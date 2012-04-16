#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"

#define	GPIO_DIR_INPUT 0
#define	GPIO_DIR_OUTPUT 1

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

