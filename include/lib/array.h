/*
 * Copyright (c) 2009  Vegard Nossum
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
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

#endif
