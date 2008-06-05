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

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/attribute_info.h"
#include "cafebabe/stream.h"

int
cafebabe_attribute_info_init(struct cafebabe_attribute_info *a,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->attribute_name_index))
		goto out;

	if (cafebabe_stream_read_uint32(s, &a->attribute_length))
		goto out;

	a->info = malloc(a->attribute_length);
	if (!a->info)
		goto out;

	for (uint16_t i = 0; i < a->attribute_length; ++i) {
		if (cafebabe_stream_read_uint8(s, &a->info[i]))
			goto out_info;
	}

	return 0;

out_info:
	free(a->info);
out:
	return 1;
}

void
cafebabe_attribute_info_deinit(struct cafebabe_attribute_info *a)
{
	free(a->info);
}
