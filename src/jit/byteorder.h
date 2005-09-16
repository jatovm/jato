/*
 * Copyright (C) 2005  Pekka Enberg
 */
#ifndef __BYTEORDER_H
#define __BYTEORDER_H

static inline u2 be16_to_cpu(u2 val)
{
	return ((val & 0xFF00U) >> 8) | ((val & 0x00FFU) << 8);
}

static inline u4 swab32(u4 val)
{
	return ((val & 0xFF000000UL) >> 24) |
	       ((val & 0x00FF0000UL) >>  8) |
	       ((val & 0x0000FF00UL) <<  8) |
	       ((val & 0x000000FFUL) << 24);
}

static inline u4 be32_to_cpu(u4 val) { return swab32(val); }
static inline u4 cpu_to_be32(u4 val) { return swab32(val); }

#endif
