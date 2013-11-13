/*
 * Copyright © 2007  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "vm/stdlib.h"
#include <stdlib.h>
#include <string.h>

void *zalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (p)
		memset(p, 0, size);
	return p;
}
