#ifndef _ARCH_EXCEPTION_H
#define _ARCH_EXCEPTION_H

#include <vm/vm.h>

/* This should be called only by JIT compiled native code */
unsigned char *throw_exception(struct object *exception);

#endif
