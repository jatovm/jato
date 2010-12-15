#ifndef JATO__ARM_MEMORY_H
#define JATO__ARM_MEMORY_H

#include <stdint.h>
#include <assert.h>

/*
 * Memory barriers.
 */
#define mb() assert(!"mb() not implemented")
#define rmb() assert(!"rmb() not implemented")
#define wmb() assert(!"wmb() not implemented")

#define smp_mb() assert(!"smp_mb() not implemented")
#define smp_rmb() assert(!"smp_rmb() not implemented")
#define smp_wmb() assert(!"smp_wmb() not implemented")

#define barrier() __asm__ __volatile__("": : :"memory")

static inline void cpu_write_u32(unsigned char *p, uint32_t val)
{
	assert(!"cpu_write_u32() not implemented");
}

#endif /* JATO__ARM_MEMORY_H */
