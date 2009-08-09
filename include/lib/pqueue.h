#ifndef LIB_PQUEUE_H
#define LIB_PQUEUE_H

#include <stdbool.h>

typedef int (*pqueue_compare_fn) (void *v1, void *v2);

/* A priority queue */
struct pqueue {
	pqueue_compare_fn		compare;
	unsigned long		size;
	void **			data;
};

struct pqueue *pqueue_alloc(pqueue_compare_fn compare);
void pqueue_free(struct pqueue *pq);
int pqueue_insert(struct pqueue *pq, void *e);
void *pqueue_remove_top(struct pqueue *pq);

static inline bool pqueue_is_empty(struct pqueue *pq)
{
	return pq->size == 1;
}

#endif /* LIB_PQUEUE_H */
