/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/list.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

typedef int (*qsort_cmp_fn)(const void *, const void *);

int list_sort(struct list_head *head, list_cmp_fn comparator)
{
	qsort_cmp_fn cmp = (qsort_cmp_fn) comparator;
	struct list_head **elements = NULL;
	struct list_head *node;
	int nr_elements;
	int err = 0;
	int i;

	nr_elements = 0;

	list_for_each(node, head) {
		struct list_head **p;
		size_t new_size;

		new_size = sizeof(struct list_head *) * (nr_elements + 1);
		p = realloc(elements, new_size);
		if (!p) {
			err = -ENOMEM;
			goto out;
		}
		elements = p;
		elements[nr_elements++] = node;
	}

	qsort(elements, nr_elements, sizeof(struct list_head *), cmp);

	INIT_LIST_HEAD(head);

	for (i = 0; i < nr_elements; i++)
		list_add_tail(elements[i], head);
out:
	free(elements);
	return err;
}
