#ifndef __VM_FIELD_H
#define __VM_FIELD_H

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
};

int vm_field_init(struct vm_field *vmf,
	struct vm_class *vmc, unsigned int field_index);

static inline enum vm_type vm_field_type(struct vm_field *vmf)
{
	return str_to_type(vmf->type);
}

#endif
