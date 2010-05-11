#ifndef X86_ATOMIC_32_H
#define X86_ATOMIC_32_H

#include <stdint.h>

extern uint64_t atomic_cmpxchg_64(uint64_t *p, uint64_t old, uint64_t new);

static inline void *do_atomic_cmpxchg_ptr(void *p, void *old, void *new)
{
	return (void *) atomic_cmpxchg_32((uint32_t *) p, (uint32_t) old, (uint32_t) new);
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	return atomic_cmpxchg_32((uint32_t *)&v->counter, old, new);
}

static inline void *atomic_cmpxchg_ptr(atomic_t *v, void *old, void *new)
{
	return do_atomic_cmpxchg_ptr((void *)&v->counter, old, new);
}

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

#endif /* X86_ATOMIC_32_H */
