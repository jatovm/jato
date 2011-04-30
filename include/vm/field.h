#ifndef __VM_FIELD_H
#define __VM_FIELD_H

#include <stdbool.h>

#include "cafebabe/constant_pool.h"
#include "cafebabe/constant_value_attribute.h"
#include "cafebabe/field_info.h"

#include "vm/vm.h"
#include "vm/types.h"

struct vm_class;

struct vm_field {
	struct vm_class *class;
	unsigned int field_index;
	const struct cafebabe_field_info *field;

	char *name;
	char *type;

	struct vm_type_info type_info;

	unsigned int offset;

	unsigned int nr_annotations;
	struct vm_annotation **annotations;
};

int vm_field_init(struct vm_field *vmf,
	struct vm_class *vmc, unsigned int field_index);
void vm_field_init_nonstatic(struct vm_field *vmf);
int vm_field_init_static(struct vm_field *vmf);

static inline bool vm_field_is_static(const struct vm_field *vmf)
{
	return vmf->field->access_flags & CAFEBABE_FIELD_ACC_STATIC;
}

static inline bool vm_field_is_final(const struct vm_field *vmf)
{
	return vmf->field->access_flags & CAFEBABE_FIELD_ACC_FINAL;
}

static inline bool vm_field_is_public(const struct vm_field *vmf)
{
	return vmf->field->access_flags & CAFEBABE_FIELD_ACC_PUBLIC;
}

static inline enum vm_type vm_field_type(const struct vm_field *vmf)
{
	return vmf->type_info.vm_type;
}

static inline bool
vm_field_equals(const struct vm_field *f1, const struct vm_field *f2)
{
	return f1->class == f2->class && f1->field_index == f2->field_index;
}

#endif
