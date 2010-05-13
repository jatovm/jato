#ifndef X86_ATOMIC_32_H
#define X86_ATOMIC_32_H

#include <stdint.h>

extern uint64_t atomic_cmpxchg_64(uint64_t *p, uint64_t old, uint64_t new);

static inline void *do_atomic_cmpxchg_ptr(void *p, void *old, void *new)
{
	return (void *) atomic_cmpxchg_32((uint32_t *) p, (uint32_t) old, (uint32_t) new);
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	return atomic_cmpxchg_32((uint32_t *)&v->counter, old, new);
}

static inline void *atomic_cmpxchg_ptr(atomic_t *v, void *old, void *new)
{
	return do_atomic_cmpxchg_ptr((void *)&v->counter, old, new);
}

#endif /* X86_ATOMIC_32_H */
