/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2009  Tomasz Grabiec <tgrabiec@gmail.com>
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
#include "cafebabe/source_file_attribute.h"
#include "cafebabe/stream.h"

int
cafebabe_source_file_attribute_init(struct cafebabe_source_file_attribute *a,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->sourcefile_index))
		goto out;

	if (!cafebabe_stream_eof(s)) {
		s->cafebabe_errno = CAFEBABE_ERROR_EXPECTED_EOF;
		goto out;
	}

	/* Success */
	return 0;

out:
	return 1;
}

void
cafebabe_source_file_attribute_deinit(struct cafebabe_source_file_attribute *a)
{
}
