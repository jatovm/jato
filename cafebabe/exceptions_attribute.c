/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
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
