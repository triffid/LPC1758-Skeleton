/* Copyright 2011 Adam Green (http://mbed.org/users/AdamGreen/)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
/* Basic unit test for C Standard I/O routines. */
//#include "mbed.h"

#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_clkpwr.h"

#include <stdio.h>

#define	GPIO_DIR_INPUT 0
#define	GPIO_DIR_OUTPUT 1

using namespace std;

class GPIO {
		uint8_t port;
		uint8_t pin;
		uint8_t io;
		uint8_t value;
	public:
		GPIO(uint8_t prt, uint8_t pn) {
			port = prt;
			pin = pn;
			io = 0;
			value = 0;

			PINSEL_CFG_Type PinCfg;
			PinCfg.Funcnum = 0;
			PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
			PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
			PinCfg.Portnum = prt;
			PinCfg.Pinnum = pn;
			PINSEL_ConfigPin(&PinCfg);
		};
		~GPIO() {};
		void set_direction(uint8_t direction) {
			FIO_SetDir(port, pin, direction);
			io = direction;
		};
		void output() {
			set_direction(1);
		};
		void input() {
			set_direction(0);
		}
		void write(uint8_t value) {
			if (value)
				FIO_SetValue(port, 1UL << pin);
			else
				FIO_ClearValue(port, 1UL << pin);
		};
		void set() {
			FIO_SetValue(port, 1UL << pin);
		};
		void clear() {
			FIO_ClearValue(port, 1UL << pin);
		};
		uint8_t get() {
			return (FIO_ReadValue(port) & (1UL << pin))?255:0;
		};
};

class UART {
		LPC_UART_TypeDef * u;
	public:
		UART(uint8_t port, uint32_t baud) {
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

			uint8_t ofdr, odlm, odll,olcr,oter;

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
			send(buf, blen);
		};
		~UART() {
			while (UART_CheckBusy(u) == SET);
			UART_DeInit(u);
		};
		void send(uint8_t *buf, uint32_t buflen) {
			UART_Send(u, buf, buflen, BLOCKING);
		};
};

GPIO X (1, 20);
GPIO Y (1, 25);
GPIO Z (1, 29);
GPIO E (0, 10);

volatile int g_LoopDummy;

void setleds(int leds) {
	X.write(leds & 1);
	Y.write(leds & 2);
	Z.write(leds & 4);
	E.write(leds & 8);
}

int main()
{
	int l = 1;
    uint8_t c[2];

	X.output();
	Y.output();
	Z.output();
	E.output();

	setleds(l);

	if (1) {
		UART * s = new UART(0, 9600);
		s->send((uint8_t *) "Start\n", 6);

		for(;;)
		{
    		for (int i = 0 ; ((i & (1 << 22)) == 0) && !g_LoopDummy ; i++)
    		{
    		}
    		c[0] = '0' + (l & 7);
    		c[1] = '\n';
			//s->send(c, 2);
			setleds(l++);
		}
	}
}
