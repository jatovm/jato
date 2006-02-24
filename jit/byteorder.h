/*
 * Copyright (C) 2005  Pekka Enberg
 */
#ifndef __BYTEORDER_H
#define __BYTEORDER_H

static inline u2 swab16(u2 val)
{
	return ((val & 0xFF00U) >> 8) | ((val & 0x00FFU) << 8);
}

static inline u2 be16_to_cpu(u2 val) { return swab16(val); }
static inline u2 cpu_to_be16(u2 val) { return swab16(val); }

static inline u4 swab32(u4 val)
{
	return ((val & 0xFF000000UL) >> 24) |
	       ((val & 0x00FF0000UL) >>  8) |
	       ((val & 0x0000FF00UL) <<  8) |
	       ((val & 0x000000FFUL) << 24);
}

static inline u4 be32_to_cpu(u4 val) { return swab32(val); }
static inline u4 cpu_to_be32(u4 val) { return swab32(val); }

static inline u8 swab64(u8 val)
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

static inline u8 be64_to_cpu(u8 val) { return swab64(val); }
static inline u8 cpu_to_be64(u8 val) { return swab64(val); }

#endif
