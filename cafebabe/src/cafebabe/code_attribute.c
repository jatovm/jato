/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
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

#include "cafebabe/attribute_info.h"
#include "cafebabe/code_attribute.h"
#include "cafebabe/stream.h"

int
cafebabe_code_attribute_init(struct cafebabe_code_attribute *a,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->max_stack))
		goto out;

	if (cafebabe_stream_read_uint16(s, &a->max_locals))
		goto out;

	if (cafebabe_stream_read_uint32(s, &a->code_length))
		goto out;

	a->code = cafebabe_stream_pointer(s);
	if (cafebabe_stream_skip(s, a->code_length))
		goto out;

	if (cafebabe_stream_read_uint16(s, &a->exception_table_length))
		goto out;

	a->exception_table = cafebabe_stream_malloc(s,
		sizeof(*a->exception_table) * a->exception_table_length);
	if (!a->exception_table)
		goto out;

	for (uint16_t i = 0; i < a->exception_table_length; ++i) {
		struct cafebabe_code_attribute_exception *e
			= &a->exception_table[i];

		if (cafebabe_stream_read_uint16(s, &e->start_pc))
			goto out_exception_table;

		if (cafebabe_stream_read_uint16(s, &e->end_pc))
			goto out_exception_table;

		if (cafebabe_stream_read_uint16(s, &e->handler_pc))
			goto out_exception_table;

		if (cafebabe_stream_read_uint16(s, &e->catch_type))
			goto out_exception_table;
	}

	if (cafebabe_stream_read_uint16(s, &a->attributes.count))
		goto out_exception_table;

	a->attributes.array = cafebabe_stream_malloc(s,
		sizeof(*a->attributes.array) * a->attributes.count);
	if (!a->attributes.array)
		goto out_exception_table;

	uint16_t attributes_i;
	for (uint16_t i = 0; i < a->attributes.count; ++i) {
		if (cafebabe_attribute_info_init(&a->attributes.array[i], s)) {
			attributes_i = i;
			goto out_attributes_init;
		}
	}
	attributes_i = a->attributes.count;

	if (!cafebabe_stream_eof(s)) {
		s->cafebabe_errno = CAFEBABE_ERROR_EXPECTED_EOF;
		goto out_attributes_init;
	}

	/* Success */
	return 0;

out_attributes_init:
	for (uint16_t i = 0; i < attributes_i; ++i)
		cafebabe_attribute_info_deinit(&a->attributes.array[i]);
	free(a->attributes.array);
out_exception_table:
	free(a->exception_table);
out:
	return 1;
}

void
cafebabe_code_attribute_deinit(struct cafebabe_code_attribute *a)
{
	free(a->exception_table);

	for (uint16_t i = 0; i < a->attributes.count; ++i)
		cafebabe_attribute_info_deinit(&a->attributes.array[i]);
	free(a->attributes.array);
}
