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
	struct arena_block		*last;
	struct arena_block		*first;
};

struct arena *arena_new(void);
void arena_delete(struct arena *self);
void *arena_expand(struct arena *arena, size_t size);

static inline void *arena_alloc(struct arena *arena, size_t size)
{
	struct arena_block *block = arena->last;
	void *p = block->free;

	if ((p + size) >= block->end)
		return arena_expand(arena, size);

	block->free	+= size;

	return p;
}

#endif /* JATO__LIB__ARENA_H */
