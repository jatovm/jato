#ifndef X86_EXCEPTION_H
#define X86_EXCEPTION_H

#include "vm/vm.h"

struct compilation_unit;

/* This should be called only by JIT compiled native code */
unsigned char *throw_exception(struct compilation_unit *cu, struct vm_object *exception);
void unwind(void);

#endif /* X86_EXCEPTION_H */
