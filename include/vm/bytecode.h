#ifndef __BYTECODE_H
#define __BYTECODE_H

#include <vm/byteorder.h>

static inline char read_s8(unsigned char *p)
{
	return (char) *p;
}

static inline unsigned char read_u8(unsigned char *p)
{
	return *p;
}

static inline short read_s16(unsigned char *p)
{
	return (short)be16_to_cpu(*(u2 *) p);
}

static inline unsigned short read_u16(unsigned char *p)
{
	return be16_to_cpu(*(u2 *) p);
}

#endif
