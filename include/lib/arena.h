#ifndef JATO__LIB__ARENA_H
#define JATO__LIB__ARENA_H

#include <stddef.h>

struct arena_block {
	void				*free;
	void				*end;
	struct arena_block		*next;
	char				data[];
};

struct arena {
	/*
	 * The head block is the only block that might have free space
	 * available. Rest of the blocks are fully used.
	 */
	struct arena_block		*head;
};

struct arena *arena_new(void);
void arena_delete(struct arena *self);
void *arena_alloc_expand(struct arena *arena, size_t size);

static inline void *arena_alloc_noexpand(struct arena *arena, size_t size)
{
	struct arena_block *block = arena->head;
	void *p = block->free;

	if ((p + size) >= block->end)
		return NULL;

	block->free	+= size;

	return p;
}

static inline void *arena_alloc(struct arena *arena, size_t size)
{
	void *p;

	p = arena_alloc_noexpand(arena, size);
	if (p)
		return p;

	return arena_alloc_expand(arena, size);
}

static inline void arena_free(struct arena *arena, void *p)
{
}

#endif /* JATO__LIB__ARENA_H */
