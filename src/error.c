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
};

const char *
cafebabe_strerror(enum cafebabe_errno e)
{
	return messages[e];
}
