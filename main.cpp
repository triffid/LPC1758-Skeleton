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

#include "uart.hpp"
#include "gpio.hpp"

using namespace std;

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
