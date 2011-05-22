#include <string.h>
#include <stdio.h>

#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/field_info.h"
#include "cafebabe/stream.h"
#include "cafebabe/annotations_attribute.h"

#include "vm/class.h"
#include "vm/field.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/stdlib.h"
#include "vm/annotation.h"

int vm_field_init(struct vm_field *vmf,
	struct vm_class *vmc, unsigned int field_index)
{
	const struct cafebabe_class *class = vmc->class;
	const struct cafebabe_field_info *field
		= &class->fields[field_index];

	vmf->class = vmc;
	vmf->field_index = field_index;
	vmf->field = field;
	vmf->annotation_initialized = false;

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
	if (!vmf->type)
		return -ENOMEM;

	if (parse_field_type(vmf)) {
		NOT_IMPLEMENTED;
		return -1;
	}

	return 0;
}

int vm_field_init_annotation(struct vm_field *vmf)
{
	const struct cafebabe_class *class = vmf->class->class;
	const struct cafebabe_field_info *field
		= &class->fields[vmf->field_index];

	struct cafebabe_annotations_attribute annotations_attribute;

	if (cafebabe_read_annotations_attribute(class, &field->attributes, &annotations_attribute))
		goto out;

	if (!annotations_attribute.num_annotations)
		goto out_deinit_annotations;

	vmf->annotations = vm_alloc(sizeof(struct vm_annotation *) * annotations_attribute.num_annotations);
	if (!vmf->annotations)
		goto out_deinit_annotations;

	for (unsigned int i = 0; i < annotations_attribute.num_annotations; i++) {
		struct cafebabe_annotation *annotation = &annotations_attribute.annotations[i];
		struct vm_annotation *vma;

		vma = vm_annotation_parse(vmf->class, annotation);
		if (!vma)
			goto error_free_annotations;
		vmf->annotations[vmf->nr_annotations++] = vma;
	}

 out_deinit_annotations:
	cafebabe_annotations_attribute_deinit(&annotations_attribute);
 out:
	vmf->annotation_initialized = true;
	return 0;

 error_free_annotations:
	for (unsigned int i = 0; i < vmf->nr_annotations; i++) {
		struct vm_annotation *vma = vmf->annotations[i];

		vm_annotation_free(vma);
	}
	vm_free(vmf->annotations);

	return -1;
}

void vm_field_init_nonstatic(struct vm_field *vmf)
{
}

int vm_field_init_static(struct vm_field *vmf)
{
	const struct vm_class *vmc = vmf->class;
	const struct cafebabe_class *class = vmc->class;
	const struct cafebabe_field_info *field
		= &class->fields[vmf->field_index];

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

	int idx = constant_value_attribute.constant_value_index;
	if (cafebabe_class_constant_index_invalid(class, idx)) {
		NOT_IMPLEMENTED;
		return -1;
	}

	struct cafebabe_constant_pool *cp;
	cp = &class->constant_pool[idx];

	/* XXX: I don't actually have any way to test that this works. We
	 * should write a test case in Jasmine or something, I guess. */
	switch (cp->tag) {
	case CAFEBABE_CONSTANT_TAG_INTEGER:
		if (mimic_stack_type(vmf->type_info.vm_type) != J_INT) {
			NOT_IMPLEMENTED;
			return -1;
		}

		static_field_set_int(vmf, cp->integer_.bytes);
		break;
	case CAFEBABE_CONSTANT_TAG_FLOAT:
		if (vmf->type_info.vm_type != J_FLOAT) {
			NOT_IMPLEMENTED;
			return -1;
		}

		static_field_set_float(vmf, cp->float_.bytes);
		break;
	case CAFEBABE_CONSTANT_TAG_STRING: {
		if (strcmp(vmf->type_info.class_name, "java/lang/String")) {
			NOT_IMPLEMENTED;
			return -1;
		}

		const struct cafebabe_constant_info_utf8 *utf8;
		if (cafebabe_class_constant_get_utf8(class,
			cp->string.string_index, &utf8))
		{
			NOT_IMPLEMENTED;
			return -1;
		}

		struct vm_object *string
			= vm_object_alloc_string_from_utf8(utf8->bytes,
				utf8->length);
		if (!string) {
			NOT_IMPLEMENTED;
			return -1;
		}

		static_field_set_object(vmf, string);
		break;
	}
	case CAFEBABE_CONSTANT_TAG_LONG:
		if (vmf->type_info.vm_type != J_LONG) {
			NOT_IMPLEMENTED;
			return -1;
		}

		static_field_set_long(vmf,
			((uint64_t) cp->long_.high_bytes << 32)
			+ (uint64_t) cp->long_.low_bytes);
		break;
	case CAFEBABE_CONSTANT_TAG_DOUBLE:
		if (vmf->type_info.vm_type != J_DOUBLE) {
			NOT_IMPLEMENTED;
			return -1;
		}

		static_field_set_double(vmf,
			uint64_to_double(cp->double_.low_bytes, cp->double_.high_bytes));
		break;
	default:
		return -1;
	}

	return 0;
}
