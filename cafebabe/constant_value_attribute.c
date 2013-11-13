/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/attribute_info.h"
#include "cafebabe/constant_value_attribute.h"
#include "cafebabe/stream.h"

int
cafebabe_constant_value_attribute_init(
	struct cafebabe_constant_value_attribute *a,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->constant_value_index))
		return 1;

	if (!cafebabe_stream_eof(s)) {
		s->cafebabe_errno = CAFEBABE_ERROR_EXPECTED_EOF;
		return 1;
	}

	/* Success */
	return 0;
}

void
cafebabe_constant_value_attribute_deinit(
	struct cafebabe_constant_value_attribute *a)
{
}
