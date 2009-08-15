#ifndef X86_ATOMIC_H
#define X86_ATOMIC_H

static inline uint32_t atomic_cmpxchg_32(uint32_t *p, uint32_t old, uint32_t new)
{
	uint32_t prev;

	asm volatile("lock; cmpxchgl %1,%2"
			: "=a"(prev)
			: "r"(new), "m"(*p), "0"(old)
			: "memory");

	return prev;
}

#endif /* X86_ATOMIC_H */
