#ifndef JATO_VM_CALL_H
#define JATO_VM_CALL_H

#include <stdarg.h>
#include <stdint.h>

struct vm_method;
struct vm_object;

unsigned long vm_call_method(struct vm_method *method, ...);
unsigned long vm_call_method_v(struct vm_method *method, va_list args);

#endif
