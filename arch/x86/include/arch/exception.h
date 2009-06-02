#ifndef _ARCH_EXCEPTION_H
#define _ARCH_EXCEPTION_H

#include <vm/vm.h>

struct compilation_unit;

/* This should be called only by JIT compiled native code */
unsigned char *throw_exception(struct compilation_unit *cu,
			       struct vm_object *exception);
void throw_exception_from_signal(void *ctx, struct vm_object *exception);
extern void unwind();

#endif
