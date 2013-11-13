/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "cafebabe/error.h"

static const char *messages[] = {
	[CAFEBABE_ERROR_EXPECTED_EOF]
		= "Expected end-of-file",
	[CAFEBABE_ERROR_UNEXPECTED_EOF]
		= "Unexpected end-of-file",
	[CAFEBABE_ERROR_BAD_MAGIC_NUMBER]
		= "Bad magic number",
	[CAFEBABE_ERROR_BAD_CONSTANT_TAG]
		= "Bad constant tag",
	[CAFEBABE_ERROR_INVALID_STACK_FRAME_TAG]
		= "Invalid stack frame tag",
};

const char *
cafebabe_strerror(enum cafebabe_errno e)
{
	return messages[e];
}
