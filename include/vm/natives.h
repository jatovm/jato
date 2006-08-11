#ifndef __VM_NATIVES_H
#define __VM_NATIVES_H

struct vm_method {
	const char *method_name;
	void *method_ptr;
};

struct vm_class {
	const char *class_name;
	struct vm_method *method_ptrs;
};

void *vm_lookup_native(struct vm_class *, const char *, const char *);

#endif
