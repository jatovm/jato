#ifndef X86_ATOMIC_H
#define X86_ATOMIC_H

#include <stdint.h>

#include "arch/memory.h"

typedef struct {
	volatile int counter;
} atomic_t;

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
#  include "arch/atomic_32.h"
#else
#  include "arch/atomic_64.h"
#endif

static inline void *atomic_read_ptr(atomic_t *v)
{
	return (void *)atomic_read(v);
}

static inline void atomic_set_ptr(atomic_t *v, void *ptr)
{
	atomic_set(v, (int) ptr);
}

#endif /* X86_ATOMIC_H */
