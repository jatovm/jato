/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/exceptions_attribute.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/stream.h"

int
cafebabe_exceptions_attribute_init(struct cafebabe_exceptions_attribute *a, struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->number_of_exceptions))
		goto out;

	a->exceptions = cafebabe_stream_malloc(s, sizeof(*a->exceptions) * a->number_of_exceptions);
	if (!a->exceptions)
		goto out;

	for (uint16_t i = 0; i < a->number_of_exceptions; ++i) {
		if (cafebabe_stream_read_uint16(s, &a->exceptions[i]))
			goto out_free;
	}
	return 0;

out_free:
	free(a->exceptions);
out:
	return 1;
}

void
cafebabe_exceptions_attribute_deinit(struct cafebabe_exceptions_attribute *a)
{
	free(a->exceptions);
}

int cafebabe_read_exceptions_attribute(const struct cafebabe_class *class,
					     const struct cafebabe_attribute_array *attributes,
					     struct cafebabe_exceptions_attribute *attrib)
{
	unsigned int exceptions_index = 0;

	if (cafebabe_attribute_array_get(attributes, "Exceptions", class, &exceptions_index)) {
		attrib->number_of_exceptions = 0;
		attrib->exceptions = NULL;
		return 0;
	}

	const struct cafebabe_attribute_info *attribute = &attributes->array[exceptions_index];

	struct cafebabe_stream stream;

	cafebabe_stream_open_buffer(&stream, attribute->info, attribute->attribute_length);

	if (cafebabe_exceptions_attribute_init(attrib, &stream))
		return -1;

	cafebabe_stream_close_buffer(&stream);

	return 0;
}
