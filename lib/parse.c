/*
 * Copyright (c) 2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/parse.h"

#include <strings.h>
#include <stdlib.h>

static unsigned long parse_unit(const char *start)
{
	if (*start == '\0')
		return 1;
	if (!strcasecmp(start, "k"))
		return 1024;
	if (!strcasecmp(start, "m"))
		return 1024*1024;
	if (!strcasecmp(start, "g"))
		return 1024*1024*1024;

	return 0;
}

unsigned long parse_long(const char *arg)
{
	unsigned long ret;
	char *end;

	ret	= strtoul(arg, &end, 10);

	ret	*= parse_unit(end);

	return ret;
}
