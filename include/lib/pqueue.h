#ifndef LIB_PQUEUE_H
#define LIB_PQUEUE_H

#include <stdbool.h>

struct pqueue_node {
	unsigned long			key;
	void				*value;
};

/* A priority queue */
struct pqueue {
	unsigned long			size;
	struct pqueue_node		*data;
};

struct pqueue *pqueue_alloc(void);
void pqueue_free(struct pqueue *pq);
int pqueue_insert(struct pqueue *pq, unsigned long key, void *value);
void *pqueue_remove_top(struct pqueue *pq);

static inline bool pqueue_is_empty(struct pqueue *pq)
{
	return pq->size == 1;
}

#endif /* LIB_PQUEUE_H */
