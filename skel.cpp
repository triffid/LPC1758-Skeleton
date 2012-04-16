#include <stdlib.h>
#include <stdio.h>

void *operator new(size_t size) throw() { return malloc(size); }

void operator delete(void *p) throw() { free(p); }
