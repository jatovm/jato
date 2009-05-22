#ifndef __VM_FIELD_H
#define __VM_FIELD_H

#include <stdbool.h>

#include <cafebabe/field_info.h>

#include <vm/vm.h>
#include <vm/types.h>

static inline enum vm_type field_type(struct fieldblock *fb)
{
	return str_to_type(fb->type);
}

struct vm_class;

struct vm_field {
	struct vm_class *class;
	unsigned int field_index;
	const struct cafebabe_field_info *field;

	char *name;
	char *type;

	unsigned int offset;

	unsigned long long static_value;
};

int vm_field_init(struct vm_field *vmf,
	struct vm_class *vmc, unsigned int field_index);

static inline bool vm_field_is_static(struct vm_field *vmm)
{
	return vmm->field->access_flags & CAFEBABE_FIELD_ACC_STATIC;
}

static inline enum vm_type vm_field_type(struct vm_field *vmf)
{
	return str_to_type(vmf->type);
}

#endif
