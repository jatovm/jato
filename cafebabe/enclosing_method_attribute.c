/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/enclosing_method_attribute.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/stream.h"
#include "cafebabe/class.h"

static int
cafebabe_enclosing_method_attribute_init(struct cafebabe_enclosing_method_attribute *a, struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->class_index))
		return -1;

	if (cafebabe_stream_read_uint16(s, &a->method_index))
		return -1;

	return 0;
}

int cafebabe_read_enclosing_method_attribute(const struct cafebabe_class *class,
					     const struct cafebabe_attribute_array *attributes,
					     struct cafebabe_enclosing_method_attribute *enclosing_method_attrib)
{
	const struct cafebabe_attribute_info *attribute;
	unsigned int enclosing_method_index = 0;
	struct cafebabe_stream stream;
	int err;

	if (cafebabe_attribute_array_get(attributes, "EnclosingMethod", class, &enclosing_method_index))
		return -1;

	attribute = &class->attributes.array[enclosing_method_index];

	cafebabe_stream_open_buffer(&stream, attribute->info, attribute->attribute_length);

	err = cafebabe_enclosing_method_attribute_init(enclosing_method_attrib, &stream);

	cafebabe_stream_close_buffer(&stream);

	return err;
}
