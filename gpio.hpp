#ifndef _GPIO_HPP
#define	_GPIO_HPP

#include <stdint.h>

#define	GPIO_DIR_INPUT 0
#define	GPIO_DIR_OUTPUT 1

class GPIO {
		uint8_t port;
		uint8_t pin;
		uint8_t io;
		uint8_t value;
	public:
		GPIO(uint8_t prt, uint8_t pn);
		~GPIO();
		void set_direction(uint8_t direction);
		void output();
		void input();
		void write(uint8_t value);
		void set();
		void clear();
		uint8_t get();
};

#endif /* _GPIO_HPP */
