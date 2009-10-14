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

#include <stdint.h>
#include <stdlib.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/method_info.h"
#include "cafebabe/stream.h"

int
cafebabe_method_info_init(struct cafebabe_method_info *m,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &m->access_flags))
		goto out;

	if (cafebabe_stream_read_uint16(s, &m->name_index))
		goto out;

	if (cafebabe_stream_read_uint16(s, &m->descriptor_index))
		goto out;

	if (cafebabe_stream_read_uint16(s, &m->attributes.count))
		goto out;

	m->attributes.array = cafebabe_stream_malloc(s,
		sizeof(*m->attributes.array) * m->attributes.count);
	if (!m->attributes.array)
		goto out;

	uint16_t attributes_i;
	for (uint16_t i = 0; i < m->attributes.count; ++i) {
		if (cafebabe_attribute_info_init(&m->attributes.array[i], s)) {
			attributes_i = i;
			goto out_attributes_init;
		}
	}
	attributes_i = m->attributes.count;

	return 0;

out_attributes_init:
	for (uint16_t i = 0; i < attributes_i; ++i)
		cafebabe_attribute_info_deinit(&m->attributes.array[i]);
	free(m->attributes.array);
out:
	return 1;
}

void
cafebabe_method_info_deinit(struct cafebabe_method_info *m)
{
	for (uint16_t i = 0; i < m->attributes.count; ++i)
		cafebabe_attribute_info_deinit(&m->attributes.array[i]);
	free(m->attributes.array);
}
