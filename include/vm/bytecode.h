#ifndef __BYTECODE_H
#define __BYTECODE_H

#include <stdint.h>

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
int32_t bytecode_read_branch_target(unsigned char opc, struct bytecode_buffer *buffer);

uint8_t read_u8(const unsigned char *p);
int16_t read_s16(const unsigned char *p);
uint16_t read_u16(const unsigned char *p);
int32_t read_s32(const unsigned char *p);
uint32_t read_u32(const unsigned char *p);

void write_u8(unsigned char *p, uint8_t value);
void write_s16(unsigned char *p, int16_t value);
void write_u16(unsigned char *p, uint16_t value);
void write_s32(unsigned char *p, int32_t value);
void write_u32(unsigned char *p, uint32_t value);

#endif
