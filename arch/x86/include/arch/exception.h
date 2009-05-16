#ifndef _ARCH_EXCEPTION_H
#define _ARCH_EXCEPTION_H

#include <vm/vm.h>

struct compilation_unit;

/* This should be called only by JIT compiled native code */
unsigned char *throw_exception(struct compilation_unit *cu,
			       struct object *exception);
extern void unwind();

#endif
