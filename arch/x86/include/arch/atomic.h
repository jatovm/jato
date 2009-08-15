#ifndef X86_ATOMIC_H
#define X86_ATOMIC_H

#include <stdint.h>

static inline uint32_t atomic_cmpxchg_32(uint32_t *p, uint32_t old, uint32_t new)
{
	uint32_t prev;

	asm volatile("lock; cmpxchgl %1,%2"
			: "=a"(prev)
			: "r"(new), "m"(*p), "0"(old)
			: "memory");

	return prev;
}

#ifdef CONFIG_X86_32
static inline void *atomic_cmpxchg_ptr(void *p, void *old, void *new)
{
	return (void *) atomic_cmpxchg_32((uint32_t *) p, (uint32_t) old, (uint32_t) new);
}
#else
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

#endif

#endif /* X86_ATOMIC_H */
