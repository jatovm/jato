/*
 * Copyright (c) 2009  Vegard Nossum
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef ARRAY_H
#define ARRAY_H

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

struct array {
	void **ptr;

	/* Actual number of elements */
	unsigned int size;

	/* Current maximum number of elements */
	unsigned int capacity;
};

static inline void array_init(struct array *a)
{
	a->ptr = NULL;
	a->size = 0;
	a->capacity = 0;
}

static inline void array_destroy(struct array *a)
{
	free(a->ptr);
}

static inline int array_resize(struct array *a, unsigned int new_capacity)
{
	assert(new_capacity > a->capacity);

	void **new_ptr = realloc(a->ptr, new_capacity * sizeof(void *));
	if (!new_ptr)
		return -ENOMEM;

	a->ptr = new_ptr;
	a->capacity = new_capacity;
	return 0;
}

static inline int array_append(struct array *a, void *element)
{
	assert(a->size <= a->capacity);

	/* Make room if needed */
	if (a->size == a->capacity) {
		int err = array_resize(a, 1 + 2 * a->capacity);
		if (err)
			return err;
	}

	a->ptr[a->size++] = element;
	return 0;
}

static inline void array_qsort(struct array *a,
			       int (*cmp)(const void *, const void *))
{
	qsort(a->ptr, a->size, sizeof(void *), cmp);
}

/* Remove duplicate elements from the array. NOTE: The array must already
 * be sorted! */
void array_unique(struct array *a, int (*cmp)(const void *, const void *));

#endif
