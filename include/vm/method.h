#ifndef __VM_METHOD_H
#define __VM_METHOD_H

#include <vm/vm.h>
#include <vm/types.h>

#include <stdbool.h>
#include <string.h>

static inline enum vm_type method_return_type(struct methodblock *method)
{
	char *return_type = method->type + (strlen(method->type) - 1);
	return str_to_type(return_type);
}

static inline bool method_is_constructor(struct methodblock *method)
{
	return strcmp(method->name,"<init>") == 0;
}

#include <cafebabe/code_attribute.h>

struct vm_class;

struct vm_method {
	const struct cafebabe_method *method;

	struct cafebabe_code_attribute code_attribute;
};

int vm_method_init(struct vm_method *vmm,
	struct vm_class *vmc, unsigned int method_index);

#endif
