/*
 * Copyright (c) 2009  Pekka Enberg
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

#include "lib/pqueue.h"

#include <stdlib.h>
#include <errno.h>

struct pqueue *pqueue_alloc(pqueue_compare_fn compare)
{
	struct pqueue *pq = malloc(sizeof *pq);
	if (pq) {
		pq->compare = compare;
		pq->size = 1;
		pq->data = NULL;
	}
	return pq;
}

void pqueue_free(struct pqueue *pq)
{
	free(pq->data);
	free(pq);
}

int pqueue_insert(struct pqueue *pq, void *e)
{
	unsigned long m;
	unsigned int i;
	void *p;

	m = pq->size + 1;

	p = realloc(pq->data, m * sizeof(void *));
	if (!p)
		return -ENOMEM;
	pq->data = p;

	i = pq->size++;
	while (i > 1 && pq->compare(pq->data[i / 2], e) < 0) {
		pq->data[i] = pq->data[i / 2];
		i /= 2;
	}
	pq->data[i] = e;

	return 0;
}

void *pqueue_remove_top(struct pqueue *pq)
{
	void *tmp, *d;
	unsigned int i = 1, j;

	if (!pq || pqueue_is_empty(pq))
		return NULL;

	d = pq->data[1];
	tmp = pq->data[--pq->size];
	while (i <= pq->size / 2) {
		j = 2 * i;
		if (j < pq->size && pq->compare(pq->data[j], pq->data[j + 1]) < 0) {
			j++;
		}
		if (pq->compare(pq->data[j], tmp) < 0) {
			break;
		}
		pq->data[i] = pq->data[j];
		i = j;
	}
	pq->data[i] = tmp;

	return d;
}
