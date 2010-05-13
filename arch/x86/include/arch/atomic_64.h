#ifndef X86_ATOMIC_64_H
#define X86_ATOMIC_64_H

#include <stdint.h>

static inline atomic_t atomic_cmpxchg(atomic_t *p, atomic_t old, atomic_t new)
{
	return atomic_cmpxchg_64(p, old, new);
}

static inline uint64_t atomic_cmpxchg_64(uint64_t *p, uint64_t old, uint64_t new)
{
	uint64_t prev;

	asm volatile("lock; cmpxchgq %1,%2"
			: "=a"(prev)
			: "r"(new), "m"(*p), "0"(old)
			: "memory");

	return prev;
}

static inline void *atomic_cmpxchg_ptr(void *p, void *old, void *new)
{
	return (void *) atomic_cmpxchg_64((uint64_t *) p, (uint64_t) old, (uint64_t) new);
}

#endif /* X86_ATOMIC_64_H */
