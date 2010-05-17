#ifndef X86_ATOMIC_H
#define X86_ATOMIC_H

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
	asm volatile("lock; incl %0"
		     : "+m" (v->counter));
}

static inline void atomic_dec(atomic_t *v)
{
	asm volatile("lock; decl %0"
		     : "+m" (v->counter));
}

/* atomic_inc() and atomic_dec() imply a full barrier on x86 */
#define smp_mb__after_atomic_inc() barrier()
#define smp_mb__after_atomic_dec() barrier()
#define smp_mb__before_atomic_inc() barrier()
#define smp_mb__before_atomic_dec() barrier()

#endif /* X86_ATOMIC_H */
