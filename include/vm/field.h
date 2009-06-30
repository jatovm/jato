#ifndef __VM_FIELD_H
#define __VM_FIELD_H

#include <stdbool.h>

#include <cafebabe/constant_pool.h>
#include <cafebabe/constant_value_attribute.h>
#include <cafebabe/field_info.h>

#include <vm/vm.h>
#include <vm/types.h>

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
void vm_field_init_nonstatic(struct vm_field *vmf, unsigned int offset);
int vm_field_init_static(struct vm_field *vmf, unsigned int offset);

static inline bool vm_field_is_static(struct vm_field *vmf)
{
	return vmf->field->access_flags & CAFEBABE_FIELD_ACC_STATIC;
}

static inline bool vm_field_is_final(struct vm_field *vmf)
{
	return vmf->field->access_flags & CAFEBABE_FIELD_ACC_FINAL;
}

static inline enum vm_type vm_field_type(struct vm_field *vmf)
{
	return str_to_type(vmf->type);
}

extern void *static_guard_page;

#endif
