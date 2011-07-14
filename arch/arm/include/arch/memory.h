#ifndef JATO__ARM_MEMORY_H
#define JATO__ARM_MEMORY_H

#include <stdint.h>
#include <assert.h>

/*
 * Memory barriers.
 */
#define mb() __sync_synchronize()
#define rmb() __sync_synchronize()
#define wmb() __sync_synchronize()

#define smp_mb() mb()
#define smp_rmb() rmb()
#define smp_wmb() wmb()

#define barrier() __asm__ __volatile__("": : :"memory")

static inline void cpu_write_u32(unsigned char *p, uint32_t val)
{
	assert(!"cpu_write_u32() not implemented");
}

#endif /* JATO__ARM_MEMORY_H */
