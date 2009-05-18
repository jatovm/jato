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
#include <jit/compilation-unit.h>
#include <jit/compiler.h>
#include <vm/buffer.h>

struct vm_class;

struct vm_method {
	const struct cafebabe_method *method;

	struct cafebabe_code_attribute code_attribute;

	struct compilation_unit *compilation_unit;
	struct jit_trampoline *trampoline;
};

int vm_method_init(struct vm_method *vmm,
	struct vm_class *vmc, unsigned int method_index);

int vm_method_prepare_jit(struct vm_method *vmm);

static inline void *vm_method_trampoline_ptr(struct vm_method *method)
{
	return buffer_ptr(method->trampoline->objcode);
}

#endif
