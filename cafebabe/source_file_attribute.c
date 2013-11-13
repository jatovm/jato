/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2009  Tomasz Grabiec <tgrabiec@gmail.com>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
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
