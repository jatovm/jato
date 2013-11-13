/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__ERROR_H
#define CAFEBABE__ERROR_H

enum cafebabe_errno {
	/* System-call-generated errors */
	CAFEBABE_ERROR_ERRNO,

	/* Cafebabe-generated errors */
	CAFEBABE_ERROR_EXPECTED_EOF,
	CAFEBABE_ERROR_UNEXPECTED_EOF,
	CAFEBABE_ERROR_BAD_MAGIC_NUMBER,
	CAFEBABE_ERROR_BAD_CONSTANT_TAG,
	CAFEBABE_ERROR_INVALID_STACK_FRAME_TAG,
};

const char *cafebabe_strerror(enum cafebabe_errno e);

#endif
