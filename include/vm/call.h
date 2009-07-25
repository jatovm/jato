#ifndef JATO_VM_CALL_H
#define JATO_VM_CALL_H

#include <stdarg.h>
#include <stdint.h>

#include "vm/jni.h"

struct vm_method;
struct vm_object;

unsigned long vm_call_method_v(struct vm_method *method, va_list args);

#define DECLARE_VM_CALL_METHOD(type, suffix)				\
	static inline type						\
	vm_call_method##suffix(struct vm_method *method, ...)		\
	{								\
		type result;						\
		va_list args;						\
									\
		va_start(args, method);					\
		result = (type)vm_call_method_v(method, args);		\
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

DECLARE_VM_CALL_METHOD(unsigned long, _ulong);
DECLARE_VM_CALL_METHOD(struct vm_object *, _object);
DECLARE_VM_CALL_METHOD(jint, _jint);
DECLARE_VM_CALL_METHOD(jboolean, _jboolean);

#endif
