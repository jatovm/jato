#ifndef __BYTECODE_H
#define __BYTECODE_H

#include <vm/byteorder.h>

static inline char read_s8(unsigned char *code, unsigned long offset)
{
	return (char) code[offset];
}

static inline unsigned char read_u8(unsigned char *code, unsigned long offset)
{
	return code[offset];
}

static inline short read_s16(unsigned char *code, unsigned long offset)
{
	return (short)be16_to_cpu(*(u2 *)(code + offset));
}

static inline unsigned short read_u16(unsigned char *code, unsigned long offset)
{
	return be16_to_cpu(*(u2 *)(code + offset));
}

#endif
