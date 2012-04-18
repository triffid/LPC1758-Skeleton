#include "uart.hpp"

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_clkpwr.h"


UART::UART(uint8_t port, uint32_t baud) {
	UART_CFG_Type UARTConfigStruct;
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;
	PINSEL_CFG_Type PinCfg;

	if (port == 0) {
		PinCfg.Funcnum = 1;
		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;
		PinCfg.Portnum = 0;

		PinCfg.Pinnum = 2;
		PINSEL_ConfigPin(&PinCfg);

		PinCfg.Pinnum = 3;
		PINSEL_ConfigPin(&PinCfg);

		u = (LPC_UART_TypeDef *) LPC_UART0;
	}
	else if (port == 1) {
		PinCfg.Funcnum = 2;
		PinCfg.OpenDrain = 0;
		PinCfg.Pinmode = 0;
		PinCfg.Portnum = 2;

		PinCfg.Pinnum = 0;
		PINSEL_ConfigPin(&PinCfg);

		PinCfg.Pinnum = 1;
		PINSEL_ConfigPin(&PinCfg);

		u = (LPC_UART_TypeDef *) LPC_UART1;
	}
	else {
		return;
	}

	UART_ConfigStructInit(&UARTConfigStruct);

	UARTConfigStruct.Baud_rate = baud;
	//UARTConfigStruct.Parity = UART_PARITY_NONE;
	//UARTConfigStruct.Databits = UART_DATABIT_8;
	//UARTConfigStruct.Stopbits = UART_STOPBIT_1;

	UART_Init(u, &UARTConfigStruct);

	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig(u, &UARTFIFOConfigStruct);

	UART_TxCmd(u, ENABLE);

	/*uint8_t ofdr, odlm, odll,olcr,oter;

	olcr = u->LCR;
	u->LCR = 0x83;

	ofdr = u->FDR;
	odlm = u->DLM;
	odll = u->DLL;
	oter = u->TER;

	//u->FDR = 0x10;
	//u->DLM = 0;
	//u->DLL = 25000000 / 16 / baud;
	u->LCR = 0x03;

	//u->TER = 0x80;

	uint8_t buf[32]; int blen;

	blen = snprintf((char *) buf, sizeof(buf), "PCLK=%lu\n", CLKPWR_GetPCLK(CLKPWR_PCLKSEL_UART0));
	send(buf, blen);
	blen = snprintf((char *) buf, sizeof(buf), "LCR=%d\n", olcr);
	send(buf, blen);
	blen = snprintf((char *) buf, sizeof(buf), "DIVADDVAL=%d\n", ofdr & 0xF);
	send(buf, blen);
	blen = snprintf((char *) buf, sizeof(buf), "MULVAL=%d\n", (ofdr >> 4) & 0xF);
	send(buf, blen);
	blen = snprintf((char *) buf, sizeof(buf), "DLM=%d\n", odlm);
	send(buf, blen);
	blen = snprintf((char *) buf, sizeof(buf), "DLL=%d\n", odll);
	send(buf, blen);
	blen = snprintf((char *) buf, sizeof(buf), "TER=%d\n", oter);
	send(buf, blen);*/
}

UART::~UART() {
	while (UART_CheckBusy(u) == SET);
	UART_DeInit(u);
}

void UART::send(uint8_t *buf, uint32_t buflen) {
	UART_Send(u, buf, buflen, BLOCKING);
}

uint32_t UART::recv(uint8_t *buf, uint32_t buflen) {
	return UART_Receive(u, buf, buflen, NONE_BLOCKING);
}
