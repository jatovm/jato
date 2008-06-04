#include "cafebabe/error.h"

static const char *messages[] = {
	[CAFEBABE_ERROR_BAD_MAGIC_NUMBER] = "Bad magic number",
};

const char *
cafebabe_strerror(enum cafebabe_errno e)
{
	return messages[e];
}
