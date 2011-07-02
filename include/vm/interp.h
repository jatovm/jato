#ifndef JATO__VM_INTERP_H
#define JATO__VM_INTERP_H

#include "vm/jni.h"

#include <stdarg.h>

struct vm_method;
struct vm_object;

void vm_interp_method_v(struct vm_method *method, va_list args, union jvalue *result);

static inline void vm_interp_method(struct vm_method *method, ...)
{
	union jvalue result;
	va_list args;

	va_start(args, method);
	vm_interp_method_v(method, args, &result);
	va_end(args);
}

#endif /* JATO__VM_INTERP_H */
