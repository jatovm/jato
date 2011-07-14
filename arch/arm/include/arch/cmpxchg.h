#ifndef JATO__ARM_CMPXCHG_H
#define JATO__ARM_CMPXCHG_H

#include <stdint.h>
#include <assert.h>

static inline uint32_t cmpxchg_32(uint32_t *p, uint32_t old, uint32_t new)
{
	uint32_t oldval;

	oldval = __sync_val_compare_and_swap(p, old, new);
	return oldval;
}

static inline uint64_t cmpxchg_64(uint64_t *p, uint64_t old, uint64_t new)
{}

static inline void *cmpxchg_ptr(void *p, void *old, void *new)
{
	return (void *) cmpxchg_32((uint32_t *) p, (uint32_t) old, (uint32_t) new);
}

#endif /* JATO__ARM_CMPXCHG_H */
