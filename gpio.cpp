#include "gpio.hpp"

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"

GPIO::GPIO(uint8_t prt, uint8_t pn) {
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
}

GPIO::~GPIO() {}

void GPIO::set_direction(uint8_t direction) {
	FIO_SetDir(port, pin, direction);
	io = direction;
}

void GPIO::output() {
	set_direction(1);
}

void GPIO::input() {
	set_direction(0);
}

void GPIO::write(uint8_t value) {
	if (value)
		FIO_SetValue(port, 1UL << pin);
	else
		FIO_ClearValue(port, 1UL << pin);
}

void GPIO::set() {
	FIO_SetValue(port, 1UL << pin);
}

void GPIO::clear() {
	FIO_ClearValue(port, 1UL << pin);
}

uint8_t GPIO::get() {
	return (FIO_ReadValue(port) & (1UL << pin))?255:0;
}
