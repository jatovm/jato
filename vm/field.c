#define _GNU_SOURCE
#include <string.h>

#include <cafebabe/class.h>
#include <cafebabe/constant_pool.h>
#include <cafebabe/field_info.h>

#include <vm/class.h>
#include <vm/field.h>
#include <vm/method.h>
#include <vm/stdlib.h>

int vm_field_init(struct vm_field *vmf,
	struct vm_class *vmc, unsigned int field_index)
{
	const struct cafebabe_class *class = vmc->class;
	const struct cafebabe_field_info *field
		= &class->fields[field_index];

	vmf->class = vmc;
	vmf->field_index = field_index;
	vmf->field = field;

	const struct cafebabe_constant_info_utf8 *name;
	if (cafebabe_class_constant_get_utf8(class, field->name_index,
		&name))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	vmf->name = strndup((char *) name->bytes, name->length);
	if (!vmf->name) {
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_utf8 *type;
	if (cafebabe_class_constant_get_utf8(class, field->descriptor_index,
		&type))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	vmf->type = strndup((char *) type->bytes, type->length);
	if (!vmf->type) {
		NOT_IMPLEMENTED;
		return -1;
	}

	NOT_IMPLEMENTED;
	vmf->offset = 0;
	/* XXX: Use ConstantValue attribute */
	vmf->static_value = 0;

	return 0;
}
