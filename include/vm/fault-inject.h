#ifndef JATO_VM_FAULT_INJECT_H
#define JATO_VM_FAULT_INJECT_H

#include <stdbool.h>

#include "vm/natives.h"

struct vm_object;

enum vm_fault {
	VM_FAULT_CLASS_INIT = 1,
	VM_FAULT_MAX
};

bool vm_fault_enabled(enum vm_fault fault);
struct vm_object *vm_fault_arg(enum vm_fault fault);

void __vm_native native_vm_enable_fault(enum vm_fault fault,
					struct vm_object *arg);

void __vm_native native_vm_disable_fault(enum vm_fault fault);

#endif
