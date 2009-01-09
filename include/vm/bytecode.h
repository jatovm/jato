#ifndef __BYTECODE_H
#define __BYTECODE_H

#include <stdint.h>
#include <vm/byteorder.h>

struct bytecode_buffer {
	unsigned char	*buffer;
	unsigned long	pos;
};

int8_t bytecode_read_s8(struct bytecode_buffer *buffer);
uint8_t bytecode_read_u8(struct bytecode_buffer *buffer);
int16_t bytecode_read_s16(struct bytecode_buffer *buffer);
uint16_t bytecode_read_u16(struct bytecode_buffer *buffer);
int32_t bytecode_read_s32(struct bytecode_buffer *buffer);
uint32_t bytecode_read_u32(struct bytecode_buffer *buffer);

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
