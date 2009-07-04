#ifndef _VM_ITABLE_H
#define _VM_ITABLE_H

#include <stdbool.h>

#include "vm/list.h"

extern bool opt_trace_itable;

/* Tunable for the interface method table. Set it too low and we will get
 * conflicts between different methods wanting the same slot in the itable.
 * Set it too high, and we waste a lot of memory per class we load. */
#define VM_ITABLE_SIZE 8

struct vm_class;
struct vm_method;

struct itable_entry {
	struct vm_method *i_method;
	struct vm_method *c_method;
	struct list_head node;
};

int vm_itable_setup(struct vm_class *vmc);
unsigned int itable_hash(struct vm_method *vmm);
void *emit_itable_resolver_stub(struct vm_class *vmc,
	struct itable_entry **sorted_table, unsigned int nr_entries);

#endif
