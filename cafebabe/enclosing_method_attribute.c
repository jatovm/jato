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
