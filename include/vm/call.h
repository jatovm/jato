#ifndef JATO_VM_CALL_H
#define JATO_VM_CALL_H

#include <stdarg.h>
#include <stdint.h>

#include "vm/jni.h"

struct vm_method;
struct vm_object;

unsigned long vm_call_method_v(struct vm_method *method, va_list args);
unsigned long vm_call_method_this_v(struct vm_method *method,
				    struct vm_object *this,
				    va_list args);
unsigned long vm_call_method_this_a(struct vm_method *method,
				    struct vm_object *this,
				    unsigned long *args);
unsigned long vm_call_method_a(struct vm_method *method, unsigned long *args);

#define DECLARE_VM_CALL_METHOD(type)					\
	static inline j ## type						\
	vm_call_method_ ## type(struct vm_method *method, ...)		\
	{								\
		j ## type result;					\
		va_list args;						\
									\
		va_start(args, method);					\
		result = (j ## type)vm_call_method_v(method, args);	\
		va_end(args);						\
									\
		return result;						\
	}

static inline void vm_call_method(struct vm_method *method, ...)
{
	va_list args;

	va_start(args, method);
	vm_call_method_v(method, args);
	va_end(args);
}

DECLARE_VM_CALL_METHOD(byte);
DECLARE_VM_CALL_METHOD(boolean);
DECLARE_VM_CALL_METHOD(double);
DECLARE_VM_CALL_METHOD(float);
DECLARE_VM_CALL_METHOD(long);
DECLARE_VM_CALL_METHOD(short);
DECLARE_VM_CALL_METHOD(object);
DECLARE_VM_CALL_METHOD(int);

extern unsigned long native_call(struct vm_method *method,
				 const void *target,
				 unsigned long *args);
extern unsigned long vm_native_call(struct vm_method *method,
				    const void *target,
				    unsigned long *args);

#endif
