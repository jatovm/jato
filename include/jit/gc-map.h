#ifndef JIT_GC_MAP_H
#define JIT_GC_MAP_H

#include <arch/registers.h>
#include <vm/system.h>

#define GC_REGISTER_MAP_SIZE	DIV_ROUND_UP(NR_GP_REGISTERS, BITS_PER_LONG)

struct gc_map {
	unsigned long		register_map[GC_REGISTER_MAP_SIZE];	/* references in registers */
};

#endif
