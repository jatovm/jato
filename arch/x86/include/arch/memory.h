#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdint.h>

/*
 * Memory barriers.
 */
#ifdef CONFIG_X86_32
#define mb() asm volatile("lock; addl $0,0(%esp)")
#define rmb() asm volatile("lock; addl $0,0(%esp)")
#define wmb() asm volatile("lock; addl $0,0(%esp)")
#else
#define mb() 	asm volatile("mfence":::"memory")
#define rmb()	asm volatile("lfence":::"memory")
#define wmb()	asm volatile("sfence" ::: "memory")
#endif

static inline void cpu_write_u32(unsigned char *p, uint32_t val)
{
	*((uint32_t*)p) = val;
}

#endif
