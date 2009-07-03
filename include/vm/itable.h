#ifndef _VM_ITABLE_H
#define _VM_ITABLE_H

#include <stdbool.h>

extern bool opt_trace_itable;

/* Tunable for the interface method table. Set it too low and we will get
 * conflicts between different methods wanting the same slot in the itable.
 * Set it too high, and we waste a lot of memory per class we load. */
#define VM_ITABLE_SIZE 64

struct vm_class;
struct vm_method;

int vm_itable_setup(struct vm_class *vmc);
unsigned int itable_hash(struct vm_method *vmm);

#endif
