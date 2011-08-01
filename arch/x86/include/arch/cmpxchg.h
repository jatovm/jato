#ifndef X86_CMPXCHG_H
#define X86_CMPXCHG_H

#include <stdint.h>

static inline uint32_t cmpxchg_32(uint32_t *p, uint32_t old, uint32_t new)
{
	uint32_t prev;

	asm volatile("lock; cmpxchgl %1,%2"
			: "=a"(prev)
			: "r"(new), "m"(*p), "0"(old)
			: "memory");

	return prev;
}

#ifdef CONFIG_X86_32
#  include "arch/cmpxchg_32.h"
#else
#  include "arch/cmpxchg_64.h"
#endif

#endif /* X86_CMPXCHG_H */
