/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "vm/fault-inject.h"
#include "vm/object.h"

struct vm_fault_entry {
	bool enabled;
	struct vm_object *arg;
};

static struct vm_fault_entry vm_fault_entries[VM_FAULT_MAX];

bool vm_fault_enabled(enum vm_fault fault)
{
	if (fault >= VM_FAULT_MAX)
		return false;

	return vm_fault_entries[fault].enabled;
}

struct vm_object *vm_fault_arg(enum vm_fault fault)
{
	if (fault >= VM_FAULT_MAX)
		return NULL;

	return vm_fault_entries[fault].arg;
}

void native_vm_enable_fault(enum vm_fault fault,
					struct vm_object *arg)
{
	if (fault >= VM_FAULT_MAX)
		return;

	vm_fault_entries[fault].enabled = true;
	vm_fault_entries[fault].arg = arg;
}

void native_vm_disable_fault(enum vm_fault fault)
{
	if (fault >= VM_FAULT_MAX)
		return;

	vm_fault_entries[fault].enabled = false;
}
