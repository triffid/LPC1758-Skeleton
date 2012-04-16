#include "LPC17xx.h"


int __aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle) {
	return 0;
}

// void *malloc(size_t) { return (void *) 0; }

//void free(void *) {}

/*
sbrk
Increase program data space.
Malloc and related functions depend on this
*/

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
