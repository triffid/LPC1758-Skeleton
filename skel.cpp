#include <stdlib.h>
#include <stdio.h>

#include "uart.hpp"
#include "gpio.hpp"

#include "LPC17xx.h"
#include "lpc17xx_nvic.h"

void *operator new(size_t size) throw() { return malloc(size); }

void operator delete(void *p) throw() { free(p); }

UART *dbg = 0;
GPIO **leds = 0;

void _dbg_init() {
	if (dbg == 0) {
		dbg = new UART(0, 9600);
	}
}

void _dbgled_init() {
	if (leds == 0) {
		// TODO: BOARD SPECIFIC
		#define N_LEDS 4
		leds = (GPIO **) malloc(sizeof(void *) * N_LEDS);
		leds[0] = new GPIO(1, 20);
		leds[1] = new GPIO(1, 25);
		leds[2] = new GPIO(1, 29);
		leds[3] = new GPIO(0, 10);
		for (int i = 0; i < N_LEDS; i++) {
			leds[i]->output();
		}
	}
}

uint8_t _cansend() {
	return dbg->cansend();
}

extern "C" {
	void init_nvic() {
		NVIC_DeInit();
		NVIC_SCBDeInit();
		extern void* __cs3_interrupt_vector_cortex_m;
		NVIC_SetVTOR((uint32_t) &__cs3_interrupt_vector_cortex_m);
	}

	uint8_t cansend() {
		return _cansend();
	}

	void dbgled(int l) {
		_dbgled_init();
		for (int i = 0; i < N_LEDS; i++) {
			leds[i]->write(l & 1);
			l >>= 1;
		}
	}

	int __aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle) {
		return 0;
	}

	int _write(int fd, uint8_t *buf, size_t buflen) {
		_dbg_init();
		dbg->send((uint8_t *) buf, buflen);
		return buflen;
	}

	int _close(int fd) {
		return 0;
	}

	int _lseek(int file, int ptr, int dir) {
		return ptr;
	}

	int _read(int file, char *buf, int len) {
		_dbg_init();
		return dbg->recv((uint8_t *) buf, len);
	}

	void* _sbrk(int incr) {

    	extern char _ebss; // Defined by the linker
    	static char *heap_end;
    	char *prev_heap_end;

    	if (heap_end == 0) {
        	heap_end = &_ebss;
    	}
    	prev_heap_end = heap_end;

		char * stack = (char*) __get_MSP();
    	if (heap_end + incr >  stack)
    	{
        	//_write (STDERR_FILENO, "Heap and stack collision\n", 25);
        	//errno = ENOMEM;
        	return  (void *) -1;
        	//abort ();
    	}

    	heap_end += incr;
    	return prev_heap_end;
	}

	int _fstat(int file, void *st) {
		return 0;
	}

	int _isatty(int fd) {
		if (fd <= 2)
			return 1;
		return 0;
	}
}
