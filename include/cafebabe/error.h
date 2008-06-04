#pragma once

#ifndef CAFEBABE__ERROR_H
#define CAFEBABE__ERROR_H

enum cafebabe_errno {
	/* System-call-generated errors */
	CAFEBABE_ERROR_ERRNO,

	/* Cafebabe-generated errors */
	CAFEBABE_ERROR_UNEXPECTED_EOF,
	CAFEBABE_ERROR_BAD_MAGIC_NUMBER,
	CAFEBABE_ERROR_BAD_CONSTANT_TAG,
};

const char *cafebabe_strerror(enum cafebabe_errno e);

#endif
