#ifndef _UART_HPP
#define	_UART_HPP

#include <stdio.h>
#include <stdint.h>

#include "lpc17xx_uart.h"

class UART {
		LPC_UART_TypeDef * u;
	public:
		UART(uint8_t port, uint32_t baud);
		~UART();
		void send(uint8_t *buf, uint32_t buflen);
		uint32_t recv(uint8_t *buf, uint32_t buflen);
		uint8_t cansend();
};

#endif /* _UART_HPP */
