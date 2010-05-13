#ifndef X86_CMPXCHG_64_H
#define X86_CMPXCHG_64_H

#include <stdint.h>

static inline uint64_t cmpxchg_64(uint64_t *p, uint64_t old, uint64_t new)
{
	uint64_t prev;

	asm volatile("lock; cmpxchgq %1,%2"
			: "=a"(prev)
			: "r"(new), "m"(*p), "0"(old)
			: "memory");

	return prev;
}

static inline void *cmpxchg_ptr(void *p, void *old, void *new)
{
	return (void *) cmpxchg_64((uint64_t *) p, (uint64_t) old, (uint64_t) new);
}

#endif /* X86_CMPXCHG_64_H */
