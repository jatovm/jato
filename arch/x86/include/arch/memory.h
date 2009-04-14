#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdint.h>

static inline void cpu_write_u32(unsigned char *p, uint32_t val)
{
	*((uint32_t*)p) = val;
}

#endif
