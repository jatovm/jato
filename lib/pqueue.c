/*
 * Copyright (c) 2009, 2012  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/pqueue.h"

#include <stdlib.h>
#include <errno.h>

struct pqueue *pqueue_alloc(void)
{
	struct pqueue *pq = malloc(sizeof *pq);
	if (pq) {
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

static inline int pqueue_compare(unsigned long a, unsigned long b)
{
	return (int)(b - a);
}

int pqueue_insert(struct pqueue *pq, unsigned long key, void *value)
{
	unsigned long m;
	unsigned int i;
	void *p;

	m = pq->size + 1;

	p = realloc(pq->data, m * sizeof(struct pqueue_node));
	if (!p)
		return -ENOMEM;
	pq->data = p;

	i = pq->size++;
	while (i > 1 && pqueue_compare(pq->data[i / 2].key, key) < 0) {
		pq->data[i] = pq->data[i / 2];
		i /= 2;
	}
	pq->data[i].key		= key;
	pq->data[i].value	= value;

	return 0;
}

void *pqueue_remove_top(struct pqueue *pq)
{
	struct pqueue_node tmp;
	void *ret;
	unsigned int i = 1, j;

	if (!pq || pqueue_is_empty(pq))
		return NULL;

	ret = pq->data[1].value;
	tmp = pq->data[--pq->size];
	while (i <= pq->size / 2) {
		j = 2 * i;
		if (j < pq->size && pqueue_compare(pq->data[j].key, pq->data[j + 1].key) < 0) {
			j++;
		}
		if (pqueue_compare(pq->data[j].key, tmp.key) < 0) {
			break;
		}
		pq->data[i] = pq->data[j];
		i = j;
	}
	pq->data[i] = tmp;

	return ret;
}
