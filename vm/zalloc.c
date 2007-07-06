/*
 * Copyright © 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * For more details on the linear scan register allocation algorithm,
 * please refer to the following papers:
 */

#include <vm/stdlib.h>
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
