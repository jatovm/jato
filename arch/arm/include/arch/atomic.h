#ifndef JATO__ARM_ATOMIC_H
#define JATO__ARM_ATOMIC_H

#include "arch/cmpxchg.h"
#include "arch/memory.h"

#include <stdint.h>

typedef struct {
	int counter;
} atomic_t;

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	return cmpxchg_32((uint32_t *)&v->counter, old, new);
}

static inline int atomic_read(const atomic_t *v)
{
	return (*(volatile int *)&(v)->counter);
}

static inline void atomic_set(atomic_t *v, int i)
{
	v->counter = i;
}

static inline void atomic_inc(atomic_t *v)
{
	assert(!"atomic_inc() not implemented");
}

static inline void atomic_dec(atomic_t *v)
{
	assert(!"atomic_dec() not implemented");
}

#define smp_mb__after_atomic_inc() assert(!"smp_mb__after_atomic_inc() not implemented")
#define smp_mb__after_atomic_dec() assert(!"smp_mb__after_atomic_dec() not implemented")
#define smp_mb__before_atomic_inc() assert(!"smp_mb__before_atomic_inc() not implemented")
#define smp_mb__before_atomic_dec() assert(!"smp_mb__before_atomic_dec() not implemented")

#endif /* JATO__ARM_ATOMIC_H */
