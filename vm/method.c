#include <cafebabe/attribute_array.h>
#include <cafebabe/attribute_info.h>
#include <cafebabe/class.h>
#include <cafebabe/code_attribute.h>
#include <cafebabe/method_info.h>
#include <cafebabe/stream.h>
#include <vm/class.h>
#include <vm/method.h>

int vm_method_init(struct vm_method *vmm,
	struct vm_class *vmc, unsigned int method_index)
{
	const struct cafebabe_class *class = vmc->class;
	const struct cafebabe_method_info *method
		= &class->methods[method_index];

	unsigned int code_index = 0;
	if (cafebabe_attribute_array_get(&method->attributes,
		"Code", class, &code_index))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	{
		/* There must be only one "Code" attribute for the method! */
		unsigned int code_index2 = code_index + 1;
		if (!cafebabe_attribute_array_get(&method->attributes,
			"Code", class, &code_index2))
		{
			NOT_IMPLEMENTED;
			return -1;
		}
	}

	const struct cafebabe_attribute_info *attribute
		= &method->attributes.array[code_index];

	struct cafebabe_stream stream;
	cafebabe_stream_open_buffer(&stream,
		attribute->info, attribute->attribute_length);

	if (cafebabe_code_attribute_init(&vmm->code_attribute, &stream)) {
		NOT_IMPLEMENTED;
		return -1;
	}

	cafebabe_stream_close_buffer(&stream);

	return 0;
}
