#include <stdlib.h>

void *operator new(size_t size) throw() { return malloc(size); }

void operator delete(void *p) throw() { free(p); }

extern "C" int __aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle) {
  return 0;
}


extern "C" void *malloc(size_t) {
	return (void *) 0;
}

extern "C" void free(void *) {
}

//extern "C" void _init(void) {}
