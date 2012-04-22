#define POLLED_USBSERIAL TRUE


/*
	LPCUSB, an USB device driver for LPC microcontrollers
	Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
	Minimal implementation of a USB serial port, using the CDC class.
	This example application simply echoes everything it receives right back
	to the host.

	Windows:
	Extract the usbser.sys file from .cab file in C:\WINDOWS\Driver Cache\i386
	and store it somewhere (C:\temp is a good place) along with the usbser.inf
	file. Then plug in the LPC176x and direct windows to the usbser driver.
	Windows then creates an extra COMx port that you can open in a terminal
	program, like hyperterminal.

	Linux:
	The device should be recognised automatically by the cdc_acm driver,
	which creates a /dev/ttyACMx device file that acts just like a regular
	serial port.

*/

// CodeRed
// Added ref to stdio.h to pull in semihosted printf rather than using serial


#include <stdio.h>

//#include <cr_section_macros.h>
//#include <NXP/crp.h>

// Variable to store CRP value in. Will be placed automatically
// by the linker when "Enable Code Read Protect" selected.
// See crp.h header for more information
//__CRP const unsigned int CRP_WORD = CRP_NO_CRP ;

#include <string.h>			// memcpy

#include "LPC17xx.h"

#include "usbapi.h"
#include "usbdebug.h"

#include "serial_fifo.h"

#include "descriptor.h"

#include "clock.h"

#include "uip.h"
#include "uip_arp.h"

// (BUF->len[0] << 8) + BUF->len[1]
#define	ip_pkt_size(p) ((((struct uip_tcpip_hdr *) p)->len[0] << 8) + ((struct uip_tcpip_hdr *) p)->len[0])

void dbgled(int l);

// CodeRed
// Control how the character received by the board is echoed back to the host
// Set to 1 to increment character ('a' echoed as 'b'), else set to 0
#define INCREMENT_ECHO_BY 1
//#define INCREMENT_ECHO_BY 0


#define BAUD_RATE	115200

#define INT_IN_EP                0x81
#define BULK_OUT_EP              0x05
#define BULK_IN_EP               0x82

#define MAX_PACKET_SIZE          64

#define LE_WORD(x)               ((x)&0xFF),((x)>>8)

// CDC definitions
#define CS_INTERFACE             0x24
#define CS_ENDPOINT              0x25

#define	SET_LINE_CODING          0x20
#define	GET_LINE_CODING          0x21
#define	SET_CONTROL_LINE_STATE   0x22

// data structure for GET_LINE_CODING / SET_LINE_CODING class requests
typedef struct {
	U32		dwDTERate;
	U8		bCharFormat;
	U8		bParityType;
	U8		bDataBits;
} TLineCoding;

static TLineCoding LineCoding = {115200, 0, 0, 8};
//static U8 abBulkBuf[128];
static U8 abClassReqData[8];

//static U8 txdata[VCOM_FIFO_SIZE];
//static U8 rxdata[VCOM_FIFO_SIZE];

//static fifo_t txfifo;
//static fifo_t rxfifo;

static U8 usb_interrupt_bits;

static U8 __attribute__ ((align (4),section (".buffers"))) rx_packet_buffer[1536];
static U8 * rx_packet_buffer_pointer;
static volatile U8 rx_status;
#define	RX_STATUS_WAIT_FOR_PACKET 0
#define RX_STATUS_GOT_PACKET      1

static U8 __attribute__ ((align (4),section (".buffers"))) tx_packet_buffer[1536];
static U8 * tx_packet_buffer_startpointer;
static U8 * tx_packet_buffer_endpointer;
static volatile U8 tx_status;
#define	TX_STATUS_WAIT_FOR_PACKET 0
#define TX_STATUS_GOT_PACKET      1

#define v2h(v) (((v & 15) < 10)?('0' + (v & 15)):('A' + ((v & 15) - 10)))

#define MAC_0 0xAE
#define MAC_1 0xF0
#define	MAC_2 0x28
#define	MAC_3 0x5D
#define	MAC_4 0x66
#define	MAC_5 0x21

#define IP_0	192
#define	IP_1	168
#define	IP_2	10
#define	IP_3	65

static const U8 macaddress_usb[6] = { MAC_0, MAC_1, MAC_2, MAC_3, MAC_4, MAC_5 };
static const U8 macaddress_net[6] = { MAC_0, MAC_1, MAC_2, MAC_3, MAC_4, MAC_5 ^ 1 };

void uip_log(char *msg) {
	printf(msg);
}

// forward declaration of interrupt handler
void USBIntHandler(void);

void dbgledscroll() {
	static int i;
	dbgled((++i) >> 5);
}

static const struct {
	usbdesc_device				device;
	usbdesc_configuration	config0;
	usbdesc_interface			if0;
	usbcdc_header					fd_header;
	usbcdc_union					fd_union;
	usbcdc_ether					fd_ether;
	usbdesc_endpoint			ep_notify;
	usbdesc_interface			if_nop;
	usbdesc_interface			if_data;
	usbdesc_endpoint			ep_bulkout;
	usbdesc_endpoint			ep_bulkin;
	usbdesc_language			st_language;
	usbdesc_string_l(6)		st_Manufacturer;
	usbdesc_string_l(9)		st_Product;
	usbdesc_string_l(8)		st_Serial;
	usbdesc_string_l(12)	st_MAC;
	U8										end;

} abDescriptors = {
	.device = {
		DL_DEVICE,
		DT_DEVICE,
		.bcdUSB							= USB_VERSION_1_1,
		.bDeviceClass				= UC_COMM,
		.bDeviceSubClass		= 0,
		.bDeviceProtocol		= 0,
		.bMaxPacketSize			= MAX_PACKET_SIZE0,
		.idVendor						= 0xFFFF,
		.idProduct					= 0x0005,
		.bcdDevice					= 0x0100,
		.iManufacturer			= 0x01,
		.iProduct						= 0x02,
		.iSerialNumber			= 0x03,
		.bNumConfigurations	= 1,
	},
	.config0 = {
		DL_CONFIGURATION,
		DT_CONFIGURATION,
		.wTotalLength				= sizeof(usbdesc_configuration)
												+ sizeof(usbdesc_interface)
												+ sizeof(usbcdc_header)
												+ sizeof(usbcdc_union)
												+ sizeof(usbcdc_ether)
												+ sizeof(usbdesc_endpoint)
												+ sizeof(usbdesc_interface)
												+ sizeof(usbdesc_interface)
												+ sizeof(usbdesc_endpoint)
												+ sizeof(usbdesc_endpoint)
												,
		.bNumInterfaces			= 2,
		.bConfigurationValue = 1,
		.iConfiguration			= 0,
		.bmAttributes				= CA_BUSPOWERED,
		.bMaxPower					= 100 mA,
	},
	.if0 = {
		DL_INTERFACE,
		DT_INTERFACE,
		.bInterfaceNumber		= 0,
		.bAlternateSetting	= 0,
		.bNumEndPoints			= 1,
		.bInterfaceClass		= UC_COMM,
		.bInterfaceSubClass	= USB_CDC_SUBCLASS_ETHERNET,
		.bInterfaceProtocol	= 0, // linux requires value of 1 for the cdc_acm module
		.iInterface					= 0,
	},
	.fd_header = {
		USB_CDC_LENGTH_HEADER,
		DT_CDC_DESCRIPTOR,
		USB_CDC_SUBTYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.fd_union = {
		USB_CDC_LENGTH_UNION,
		DT_CDC_DESCRIPTOR,
		USB_CDC_SUBTYPE_UNION,
		.bMasterInterface = 0,
		.bSlaveInterface0 = 1,
	},
	.fd_ether = {
		USB_CDC_LENGTH_ETHER,
		DT_CDC_DESCRIPTOR,
		USB_CDC_SUBTYPE_ETHERNET,
		.iMacAddress = 4,
		.bmEthernetStatistics = 0,
		.wMaxSegmentSize = 1514,
		.wNumberMCFilters = 0,
		.bNumberPowerFilters = 0,
	},
	.ep_notify = {
		DL_ENDPOINT,
		DT_ENDPOINT,
		.bEndpointAddress = INT_IN_EP,
		.bmAttributes = EA_INTERRUPT,
		.wMaxPacketSize = 8,
		.bInterval = 10,
	},
	.if_nop = {
		DL_INTERFACE,
		DT_INTERFACE,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndPoints = 0,
		.bInterfaceClass = UC_CDC_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	.if_data = {
		DL_INTERFACE,
		DT_INTERFACE,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 1,
		.bNumEndPoints = 2,
		.bInterfaceClass = UC_CDC_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	.ep_bulkout = {
		DL_ENDPOINT,
		DT_ENDPOINT,
		.bEndpointAddress = BULK_OUT_EP,
		.bmAttributes = EA_BULK,
		.wMaxPacketSize = MAX_PACKET_SIZE,
		.bInterval = 0,
	},
	.ep_bulkin = {
		DL_ENDPOINT,
		DT_ENDPOINT,
		.bEndpointAddress = BULK_IN_EP,
		.bmAttributes = EA_BULK,
		.wMaxPacketSize = MAX_PACKET_SIZE,
		.bInterval = 0,
	},
	.st_language = {
		.bLength = 4,
		DT_STRING,
		{ SL_USENGLISH, },
	},
	.st_Manufacturer = {
		14,
		DT_STRING,
		{ 'L','P','C','U','S','B', },
	},
	.st_Product = {
		20,
		DT_STRING,
		{ 'U','S','B','S','e','r','i','a','l', },
	},
	.st_Serial = {
		18,
		DT_STRING,
		{ 'D','E','A','D','B','E','E','F', },
	},
	.st_MAC = {
		26,
		DT_STRING,
		{
			v2h(MAC_0 >> 4),
			v2h(MAC_0 & 15),
			v2h(MAC_1 >> 4),
			v2h(MAC_1 & 15),
			v2h(MAC_2 >> 4),
			v2h(MAC_2 & 15),
			v2h(MAC_3 >> 4),
			v2h(MAC_3 & 15),
			v2h(MAC_4 >> 4),
			v2h(MAC_4 & 15),
			v2h(MAC_5 >> 4),
			v2h(MAC_5 & 15),
		},
	},
	0,
};

U8 isValidPacket(U8 *buf, U16 *len, U16 *pType) {
	// first check dest mac
	U8 *dst = buf;
	//U8 *src = buf + 6;
	U8 broadcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	if ((memcmp(macaddress_net, dst, 6) == 0) || (memcmp(broadcast, dst, 6) == 0)) {
		U16 type = buf[12] << 8 | buf[13];
		if (pType)
			*pType = type;
		U8 *iphdr = buf + 14;
		if (type == UIP_ETHTYPE_IP) {
			U16 length = (buf[16] << 8 | buf[17]) + (iphdr - buf);
			if (len)
				*len = length;
			return 1;
		}
		if (type == UIP_ETHTYPE_ARP) {
			if (len)
				*len = 42;
			return 1;
		}
	}
	if (len)
		*len = 0;
	return 0;
}

/**
	Local function to handle incoming bulk data

	@param [in] bEP
	@param [in] bEPStatus
 */
static void BulkOut(U8 bEP, U8 bEPStatus)
{
	U16 iLen, pLen;

	printf("OUT EP [%02X:%02X] ", bEP, bEPStatus);

	if (rx_status != RX_STATUS_WAIT_FOR_PACKET) {
		printf(" NACK!\n");
		USBHwNakIntEnable(usb_interrupt_bits &= ~INACK_BO);
		return;
	}

// 	if (fifo_free(&rxfifo) < MAX_PACKET_SIZE) {
// 		// may not fit into fifo
// 		printf(" full\n");
// 		return;
// 	}

	// get data from USB into intermediate buffer
// 	iLen = USBHwEPRead(bEP, abBulkBuf, sizeof(abBulkBuf));
// 	i = sizeof(rx_packet_buffer);
// 	if (rx_packet_buffer_pointer != rx_packet_buffer) {
// 		if (isValidPacket(rx_packet_buffer, &pLen)) {
// 			i = pLen - (rx_packet_buffer_pointer - rx_packet_buffer);
// 		}
// 		else {
// 			rx_packet_buffer_pointer = rx_packet_buffer;
// 		}
// 	}
//
// 	if (rx_packet_buffer_pointer < rx_packet_buffer || rx_packet_buffer_pointer >= rx_packet_buffer + sizeof(rx_packet_buffer)) {
// 		rx_packet_buffer_pointer = rx_packet_buffer;
// 	}

	// now i (hopefully) holds the amount of bytes until the end of the ethernet frame, or MAX if the buffer is empty

// 	printf("->%d", rx_packet_buffer_pointer - rx_packet_buffer);

	iLen = USBHwEPRead(bEP, rx_packet_buffer_pointer, MAX_PACKET_SIZE);

	printf("%d:", iLen);

	for (int j = 0; j < iLen; j++) {
		printf(",0x%02x", rx_packet_buffer_pointer[j]);
	}

// 	i -= iLen;

	if (isValidPacket(rx_packet_buffer, &pLen, NULL)) {
		rx_packet_buffer_pointer += iLen;

		printf(" got %d, frame bytes %d of %d\n", iLen, rx_packet_buffer_pointer - rx_packet_buffer, pLen);

		if (rx_packet_buffer_pointer >= rx_packet_buffer + pLen) {
			rx_status = RX_STATUS_GOT_PACKET;
			USBHwNakIntEnable(usb_interrupt_bits &= ~INACK_BO);
			//USBHwEPStall(bEP, 1);
		}
	}
	else {
		printf(" INVALID, dropping\n");
		rx_packet_buffer_pointer = rx_packet_buffer;
	}
//
// 	for (i = 0; i < iLen; i++) {
// 		// put into FIFO
// 		if (!fifo_put(&rxfifo, abBulkBuf[i])) {
// 			// overflow... :(
// 			ASSERT(FALSE);
// 			break;
// 		}
// 	}
}


/**
	Local function to handle outgoing bulk data

	@param [in] bEP
	@param [in] bEPStatus
 */
static void BulkIn(U8 bEP, U8 bEPStatus)
{
	int iLen;

	printf("IN EP [%02X:%02X]", bEP, bEPStatus);

// 	if ((bEPStatus & EP_STATUS_NACKED) == 0)
// 		return;

	if (tx_packet_buffer_startpointer < tx_packet_buffer_endpointer) {
		iLen = tx_packet_buffer_endpointer - tx_packet_buffer_startpointer;

		if (iLen > 64)
			iLen = 64;

		printf(" Sending %d:", iLen);
		for (int i = 0; i < iLen; i++)
			printf("_%02X", tx_packet_buffer_startpointer[i]);

		if (USBHwEPWrite(bEP, tx_packet_buffer_startpointer, iLen))
			tx_packet_buffer_startpointer += iLen;
	}

	if (tx_packet_buffer_startpointer >= tx_packet_buffer_endpointer) {
		USBHwNakIntEnable(usb_interrupt_bits &= ~INACK_BI);
		tx_status = TX_STATUS_WAIT_FOR_PACKET;
		printf(" FIN\n");
		//USBHwEPStall(bEP, 1);
	}
	else {
		printf("...cont\n");
	}


// 	if (fifo_avail(&txfifo) == 0) {
// 		// no more data, disable further NAK interrupts until next USB frame
// 		USBHwNakIntEnable(usb_interrupt_bits &= ~INACK_BI);
// 		printf(" empty\n");
// 		return;
// 	}
//
// 	// get bytes from transmit FIFO into intermediate buffer
// 	for (i = 0; i < MAX_PACKET_SIZE; i++) {
// 		if (!fifo_get(&txfifo, &abBulkBuf[i])) {
// 			break;
// 		}
// 	}
// 	iLen = i;
//
// 	printf(" sent %d\n", iLen);
//
// 	// send over USB
// 	if (iLen > 0) {
// 		USBHwEPWrite(bEP, abBulkBuf, iLen);
// 	}
}


/**
	Local function to handle the USB-CDC class requests

	@param [in] pSetup
	@param [out] piLen
	@param [out] ppbData
 */
static BOOL HandleClassRequest(TSetupPacket *pSetup, int *piLen, U8 **ppbData)
{
	printf("handle %x %x %d %d %d\n", pSetup->bmRequestType, pSetup->bRequest, pSetup->wValue, pSetup->wIndex, pSetup->wLength);
	switch (pSetup->bRequest) {

	// set line coding
	case SET_LINE_CODING:
		DBG("SET_LINE_CODING\n");
		memcpy((U8 *)&LineCoding, *ppbData, 7);
		*piLen = 7;
		DBG("dwDTERate=%u, bCharFormat=%u, bParityType=%u, bDataBits=%u\n",
		LineCoding.dwDTERate,
		LineCoding.bCharFormat,
		LineCoding.bParityType,
		LineCoding.bDataBits);
		break;

	// get line coding
	case GET_LINE_CODING:
		DBG("GET_LINE_CODING\n");
		*ppbData = (U8 *)&LineCoding;
		*piLen = 7;
		break;

	// set control line state
	case SET_CONTROL_LINE_STATE:
		// bit0 = DTR, bit = RTS
		DBG("SET_CONTROL_LINE_STATE %X\n", pSetup->wValue);
		break;

	default:
		printf("Unknown CLASS Request: %d\n", pSetup->bRequest);
		return FALSE;
	}
	return TRUE;
}


/**
	Initialises the VCOM port.
	Call this function before using VCOM_putchar or VCOM_getchar
 */
// void VCOM_init(void)
// {
// 	fifo_init(&txfifo, txdata);
// 	fifo_init(&rxfifo, rxdata);
// }


/**
	Writes one character to VCOM port

	@param [in] c character to write
	@returns character written, or EOF if character could not be written
 */
// int VCOM_putchar(int c)
// {
// 	return fifo_put(&txfifo, c) ? c : EOF;
// }


/**
	Reads one character from VCOM port

	@returns character read, or EOF if character could not be read
 */
// int VCOM_getchar(void)
// {
// 	U8 c;
//
// 	return fifo_get(&rxfifo, &c) ? c : EOF;
// }


/**
	Interrupt handler

	Simply calls the USB ISR
 */
//void USBIntHandler(void)
void USB_IRQHandler(void)
{
	//dbgledscroll();
	USBHwISR();
//	dbgled(0);
}

typedef struct {
	U32 timeout;
	volatile U32 trigger_time;
	volatile U8 flag;
} timer;

timer timers[] = {
	{
		50,
		50,
		0,
	},
	{
		500,
		500,
		0,
	},
	{ 0, 0, 0, },
};

timer *timer_1_2s = &timers[0];
timer *timer_10s = &timers[1];

/**
 * SysTick IRQ
 */
U32 time;
void SysTick_Handler(void) {
	time++;
	//dbgled((time >> 5) & 0xFF);
	for (int i = 0; timers[i].timeout != 0; i++) {
		if (time == timers[i].trigger_time) {
			if (timers[i].flag != 255)
				timers[i].flag++;
			timers[i].trigger_time += timers[i].timeout;
		}
	}
}

static void USBFrameHandler(U16 wFrame)
{
	//if (fifo_avail(&txfifo) > 0) {
		// data available, enable NAK interrupt on bulk in

	//TODO: work out why enabling this makes things lock up
	//if (tx_status == TX_STATUS_GOT_PACKET)
		//USBHwNakIntEnable(usb_interrupt_bits = INACK_BI);

	//}
}

void txpacket(void *packet, int len) {
	while (tx_status == TX_STATUS_GOT_PACKET);
	memcpy(tx_packet_buffer, packet, len);
	tx_packet_buffer_startpointer = tx_packet_buffer;
	tx_packet_buffer_endpointer = tx_packet_buffer + len;
	tx_status = TX_STATUS_GOT_PACKET;
	//USBHwNakIntEnable(usb_interrupt_bits = INACK_BI);
}

//void enable_USB_interrupts(void);

/*************************************************************************
	main
	====
**************************************************************************/
int main(void)
{
	//int c;
	uip_ipaddr_t ipaddr;	/* local IP address */

	dbgled(0);

	clock_init();

	dbgled(1);

	printf("Initialising uIP\n");

	uip_init();

	uip_arp_init();

	printf("Setting MAC Address\n");
	struct uip_eth_addr mac;
	memcpy(&mac, macaddress_net, 6);
	uip_setethaddr(mac);

	printf("Setting Netmask\n");

	uip_ipaddr(ipaddr, 255,255,255,0);
	uip_setnetmask(ipaddr);	/* mask */

	//uip_ipaddr(ipaddr, 192,168,10,1);
	//uip_setdraddr(ipaddr);	/* router IP address */

	printf("Setting IP address\n");

	uip_ipaddr(ipaddr, IP_0,IP_1,IP_2,IP_3);
	uip_sethostaddr(ipaddr);	/* host IP address */

	printf("Starting HTTPd\n");

	httpd_init();

	printf("Initialising USB stack\n");

	// initialise stack
	USBInit();

	// register descriptors
	USBRegisterDescriptors((uint8_t *) &abDescriptors);

	// register class request handler
	USBRegisterRequestHandler(REQTYPE_TYPE_CLASS, HandleClassRequest, abClassReqData);

	// register endpoint handlers
	USBHwRegisterEPIntHandler(INT_IN_EP, NULL);
	USBHwRegisterEPIntHandler(BULK_IN_EP, NULL);
	USBHwRegisterEPIntHandler(BULK_OUT_EP, NULL);

	// register frame handler
	USBHwRegisterFrameHandler(USBFrameHandler);

	// enable bulk-in interrupts on NAKs
// 	USBHwNakIntEnable(usb_interrupt_bits |= INACK_BI);

	dbgled(2);

	// initialise VCOM
	//VCOM_init();
	printf("Starting USB communication\n");

	dbgled(3);

#ifndef POLLED_USBSERIAL
	//enable_USB_interrupts();
	NVIC_EnableIRQ(USB_IRQn);
	dbgled(4);
	LPC_SC->USBIntSt |= 0x80000000;
	dbgled(5);
#endif

	dbgled(6);
	// connect to bus

	printf("Connecting to USB bus\n");
	dbgled(7);
	USBHwConnect(TRUE);

	dbgled(8);

// 	volatile static int i = 0 ;

	U32 watchdog = 0;
	U8 connflag[UIP_CONNS];
	U8 connflag_any = 0;

	for (int i = 0; i < UIP_CONNS; i++)
		connflag[i] = 0;

	// echo any character received (do USB stuff in interrupt)
	while (1) {

// 		i++ ;

		// CodeRed - add option to use polling rather than interrupt
#ifdef POLLED_USBSERIAL

		USBHwISR();

#endif

#define EPSTAT_FULL 1

		//dbgledscroll();

		dbgled(1);

		if (tx_status == TX_STATUS_WAIT_FOR_PACKET) {
			if (rx_status == RX_STATUS_WAIT_FOR_PACKET) {
				U8 status = USBHwEPGetStatus(BULK_OUT_EP);
				if ((status & EPSTAT_FULL) == EPSTAT_FULL) {
					BulkOut(BULK_OUT_EP, status);
				}
			}
			dbgled(2);
			if (rx_status == RX_STATUS_GOT_PACKET) {
				U16 pLen = rx_packet_buffer_pointer - rx_packet_buffer;
				U16 pType;
				printf("packet drain %d bytes\n", pLen);
				if (isValidPacket(rx_packet_buffer, NULL, &pType)) {
					if (pType == UIP_ETHTYPE_ARP) {
						memcpy(uip_buf, rx_packet_buffer, pLen);
						uip_len = pLen;
						uip_arp_arpin();

						if (uip_len > 0) {
							txpacket(uip_buf, uip_len);
						}
					}
					else {
						memcpy(uip_buf, rx_packet_buffer, pLen);
						uip_len = pLen;
						uip_arp_ipin();
						uip_input();
						if (uip_len > 0) {
							uip_arp_out();
							txpacket(uip_buf, uip_len);
						}
					}
				}
				rx_packet_buffer_pointer = rx_packet_buffer;
				rx_status = RX_STATUS_WAIT_FOR_PACKET;
	// 			USBHwEPStall(BULK_OUT_EP, 0);
			}
		}
		dbgled(3);
		if (tx_status != TX_STATUS_GOT_PACKET) {
			if (timer_1_2s->flag) {
				timer_1_2s->flag = 0;
				connflag_any = 1;
				for(int i = 0; i < UIP_CONNS; i++)
				{
					connflag[i] = 1;
				}
			}
			dbgled(4);
			if (connflag_any) {
				int i;
				for (i = 0; i < UIP_CONNS; i++) {
					if (connflag[i]) {
						connflag[i] = 0;
						uip_periodic(i);
						/* If the above function invocation resulted in data that
							*    should be sent out on the network, the global variable
							*    uip_len is set to a value > 0. */
						if(uip_len > 0) {
							uip_arp_out();
							txpacket(uip_buf, uip_len);
							//USBHwEPStall(BULK_IN_EP, 0);
							break;
						}
					}
				}
				if (i == UIP_CONNS) {
					connflag_any = 0;
				}
			}
		}
		dbgled(5);
		if (tx_status == TX_STATUS_GOT_PACKET) {
			U8 status = USBHwEPGetStatus(BULK_IN_EP);
			if ((status & EPSTAT_FULL) == 0) {
				BulkIn(BULK_IN_EP, status);
			}
		}
		dbgled(6);
		if (timer_10s->flag) {
			timer_10s->flag = 0;
			printf("timer 10s TX%d RX%d INT%d BIN:0x%02X BOUT:0x%02X\n", tx_status, rx_status, usb_interrupt_bits, USBHwEPGetStatus(BULK_IN_EP), USBHwEPGetStatus(BULK_OUT_EP));
			uip_arp_timer();
			watchdog = 0;
		}
		dbgled(7);
		watchdog++;
		//dbgled(watchdog >> 13);
	}

	return 0;
}

