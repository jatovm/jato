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

static inline int atomic_read(const atomic_t *v)
{
	return v->counter;
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

static inline void *atomic_read_ptr(atomic_t *v)
{
	return (void *)atomic_read(v);
}

static inline void atomic_set_ptr(atomic_t *v, void *ptr)
{
	atomic_set(v, (int) ptr);
}

#endif /* X86_ATOMIC_H */
