#include <string.h>
#include <stdio.h>

#include <cafebabe/class.h>
#include <cafebabe/constant_pool.h>
#include <cafebabe/field_info.h>
#include <cafebabe/stream.h>

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

	return 0;
}

void vm_field_init_nonstatic(struct vm_field *vmf, unsigned int offset)
{
	vmf->offset = offset;
}

int vm_field_init_static(struct vm_field *vmf, unsigned int offset)
{
	vmf->offset = offset;

	const struct vm_class *vmc = vmf->class;
	const struct cafebabe_class *class = vmc->class;
	const struct cafebabe_field_info *field
		= &class->fields[vmf->field_index];

	/* XXX: Actually _use_ the ConstantValue attribute */
	vmf->class->static_values[offset] = 0;

	unsigned int constant_value_index = 0;
	if (cafebabe_attribute_array_get(&field->attributes,
		"ConstantValue", class, &constant_value_index))
	{
		return 0;
	}

	const struct cafebabe_attribute_info *attribute
		= &field->attributes.array[constant_value_index];

	struct cafebabe_constant_value_attribute constant_value_attribute;

	struct cafebabe_stream stream;
	cafebabe_stream_open_buffer(&stream,
		attribute->info, attribute->attribute_length);

	if (cafebabe_constant_value_attribute_init(
		&constant_value_attribute, &stream))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	cafebabe_stream_close_buffer(&stream);
	return 0;
}
