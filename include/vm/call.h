#ifndef JATO_VM_CALL_H
#define JATO_VM_CALL_H

#include <stdarg.h>
#include <stdint.h>

#include "vm/jni.h"

struct vm_method;
struct vm_object;

void vm_call_method_v(struct vm_method *method, va_list args, union jvalue *result);
void vm_call_method_this_v(struct vm_method *method,
			   struct vm_object *this,
			   va_list args,
			   union jvalue *result);
void vm_call_method_this_a(struct vm_method *method,
			   struct vm_object *this,
			   unsigned long *args,
			   union jvalue *result);
void vm_call_method_a(struct vm_method *method,
		      unsigned long *args,
		      union jvalue *result);

#define DECLARE_VM_CALL_METHOD(type, symbol)				\
	static inline j ## type						\
	vm_call_method_ ## type(struct vm_method *method, ...)		\
	{								\
		union jvalue result;					\
		va_list args;						\
									\
		va_start(args, method);					\
		vm_call_method_v(method, args, &result);		\
		va_end(args);						\
									\
		return result.symbol;					\
	}

#define DECLARE_VM_CALL_METHOD_THIS(type, symbol)			\
	static inline j ## type						\
	vm_call_method_this_ ## type(struct vm_method *method,		\
				     struct vm_object *this, ...)	\
	{								\
		union jvalue result;					\
		va_list args;						\
									\
		va_start(args, this);					\
		vm_call_method_this_v(method, this, args, &result);	\
		va_end(args);						\
									\
		return result.symbol;					\
	}

static inline void vm_call_method(struct vm_method *method, ...)
{
	union jvalue result;
	va_list args;

	va_start(args, method);
	vm_call_method_v(method, args, &result);
	va_end(args);
}

DECLARE_VM_CALL_METHOD(byte, b);
DECLARE_VM_CALL_METHOD(boolean, z);
DECLARE_VM_CALL_METHOD(double, d);
DECLARE_VM_CALL_METHOD(float, f);
DECLARE_VM_CALL_METHOD(long, j);
DECLARE_VM_CALL_METHOD(short, s);
DECLARE_VM_CALL_METHOD(object, l);
DECLARE_VM_CALL_METHOD(int, i);

DECLARE_VM_CALL_METHOD_THIS(byte, b);
DECLARE_VM_CALL_METHOD_THIS(boolean, z);
DECLARE_VM_CALL_METHOD_THIS(double, d);
DECLARE_VM_CALL_METHOD_THIS(float, f);
DECLARE_VM_CALL_METHOD_THIS(long, j);
DECLARE_VM_CALL_METHOD_THIS(short, s);
DECLARE_VM_CALL_METHOD_THIS(object, l);
DECLARE_VM_CALL_METHOD_THIS(int, i);

extern void native_call(struct vm_method *method,
			void *target,
			unsigned long *args,
			union jvalue *result);

#endif
