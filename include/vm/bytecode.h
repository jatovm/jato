#ifndef __BYTECODE_H
#define __BYTECODE_H

#include <vm/byteorder.h>
#include <stdint.h>

static inline int8_t read_s8(unsigned char *p)
{
	return (char) *p;
}

static inline uint8_t read_u8(unsigned char *p)
{
	return *p;
}

static inline int16_t read_s16(unsigned char *p)
{
	return be16_to_cpu(*(uint16_t *) p);
}

static inline uint16_t read_u16(unsigned char *p)
{
	return be16_to_cpu(*(uint16_t *) p);
}

static inline int32_t read_s32(unsigned char *p)
{
	return be32_to_cpu(*(uint32_t *) p);
}

static inline uint32_t read_u32(unsigned char *p)
{
	return be32_to_cpu(*(uint32_t *) p);
}

#endif
