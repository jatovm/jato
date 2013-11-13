/*
 * Copyright (c) 2009  Vegard Nossum
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/array.h"

void array_unique(struct array *a, int (*cmp)(const void *, const void *))
{
	unsigned int n = a->size;
	if (!n)
		return;

	void **ptr = a->ptr;
	unsigned int target = 0;

	for (unsigned int i = target + 1; i < n; ++i) {
		assert(i > target);

		/* If they're equal, we skip it. */
		if (!cmp(&ptr[target], &ptr[i]))
			continue;

		ptr[++target] = ptr[i];
	}

	/* Update the size */
	a->size = target + 1;
}
