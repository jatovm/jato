/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdint.h>
#include <stdlib.h>

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

	if (cafebabe_stream_read_uint16(s, &m->attributes_count))
		goto out;

	m->attributes = malloc(sizeof(*m->attributes) * m->attributes_count);
	if (!m->attributes)
		goto out;

	uint16_t attributes_i;
	for (uint16_t i = 0; i < m->attributes_count; ++i) {
		if (cafebabe_attribute_info_init(&m->attributes[i], s)) {
			attributes_i = i;
			goto out_attributes_init;
		}
	}
	attributes_i = m->attributes_count;

	return 0;

out_attributes_init:
	for (uint16_t i = 0; i < attributes_i; ++i)
		cafebabe_attribute_info_deinit(&m->attributes[i]);
	free(m->attributes);
out:
	return 1;
}

void
cafebabe_method_info_deinit(struct cafebabe_method_info *m)
{
	for (uint16_t i = 0; i < m->attributes_count; ++i)
		cafebabe_attribute_info_deinit(&m->attributes[i]);
	free(m->attributes);
}
