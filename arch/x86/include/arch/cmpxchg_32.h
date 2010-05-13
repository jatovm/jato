#ifndef X86_CMPXCHG_32_H
#define X86_CMPXCHG_32_H

#include <stdint.h>

extern uint64_t cmpxchg_64(uint64_t *p, uint64_t old, uint64_t new);

static inline void *cmpxchg_ptr(void *p, void *old, void *new)
{
	return (void *) cmpxchg_32((uint32_t *) p, (uint32_t) old, (uint32_t) new);
}

#endif /* X86_CMPXCHG_32_H */
