#include "lib/arena.h"

#include <stdlib.h>
#include <assert.h>

#define ARENA_BLOCK_MIN_LEN		256

static struct arena_block *arena_block_new(size_t len)
{
	struct arena_block *self;

	self		= malloc(sizeof *self + len);
	if (!self)
		return NULL;

	self->free	= self->data;
	self->end	= self->data + len;
	self->next	= NULL;

	return self;
}

static void arena_block_delete(struct arena_block *self)
{
	free(self);
}

struct arena *arena_new(void)
{
	struct arena_block *block;
	struct arena *self;

	self		= calloc(1, sizeof *self);
	if (!self)
		return NULL;

	block		= arena_block_new(ARENA_BLOCK_MIN_LEN);
	if (!block) {
		free(self);
		return NULL;
	}

	self->first	= block;
	self->last	= block;

	return self;
}

void arena_delete(struct arena *self)
{
	struct arena_block *block = self->first;

	while (block) {
		struct arena_block *next = block->next;

		arena_block_delete(block);

		block		= next;
	}

	free(self);
}

static inline void *arena_block_alloc(struct arena_block *self, size_t size)
{
	void *p = self->free;

	if (p >= self->end)
		return NULL;

	self->free	+= size;

	return p;
}

void *arena_expand(struct arena *arena, size_t size)
{
	struct arena_block *block;

	assert(size < ARENA_BLOCK_MIN_LEN);

	block		= arena_block_new(ARENA_BLOCK_MIN_LEN);

	arena->last->next	= block;

	arena->last		= block;

	return arena_block_alloc(block, size);
}
