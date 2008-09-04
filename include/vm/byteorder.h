/*
 * Copyright (C) 2005  Pekka Enberg
 */
#ifndef __BYTEORDER_H
#define __BYTEORDER_H

#include <vm/vm.h>
#include <stdint.h>

static inline uint16_t swab16(uint16_t val)
{
	return ((val & 0xFF00U) >> 8) | ((val & 0x00FFU) << 8);
}

static inline uint16_t be16_to_cpu(uint16_t val) { return swab16(val); }
static inline uint16_t cpu_to_be16(uint16_t val) { return swab16(val); }

static inline uint32_t swab32(uint32_t val)
{
	return ((val & 0xFF000000UL) >> 24) |
	       ((val & 0x00FF0000UL) >>  8) |
	       ((val & 0x0000FF00UL) <<  8) |
	       ((val & 0x000000FFUL) << 24);
}

static inline uint32_t be32_to_cpu(uint32_t val) { return swab32(val); }
static inline uint32_t cpu_to_be32(uint32_t val) { return swab32(val); }

static inline uint64_t swab64(uint64_t val)
{
	return ((val & 0xFF00000000000000ULL) >> 56) |
	       ((val & 0x00FF000000000000ULL) >> 40) |
	       ((val & 0x0000FF0000000000ULL) >> 24) |
	       ((val & 0x000000FF00000000ULL) >>  8) |
	       ((val & 0x00000000FF000000ULL) <<  8) |
	       ((val & 0x0000000000FF0000ULL) << 24) |
	       ((val & 0x000000000000FF00ULL) << 40) |
	       ((val & 0x00000000000000FFULL) << 56);
}

static inline uint64_t be64_to_cpu(uint64_t val) { return swab64(val); }
static inline uint64_t cpu_to_be64(uint64_t val) { return swab64(val); }

#endif
