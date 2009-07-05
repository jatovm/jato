/*
 * Copyright (c) 2009 Tomasz Grabiec
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
