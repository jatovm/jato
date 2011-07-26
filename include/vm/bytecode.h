#ifndef JATO_VM_BYTECODE_H
#define JATO_VM_BYTECODE_H

#include <stdbool.h>
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

struct vm_class;

unsigned long bc_insn_size(const unsigned char *, unsigned long);
long bc_insn_size_safe(unsigned char *, unsigned long, unsigned long);
bool bc_is_invalid(unsigned char);
bool bc_is_ldc(unsigned char);
bool bc_is_branch(unsigned char);
bool bc_uses_local_var(unsigned char);
bool bc_is_goto(unsigned char);
bool bc_is_wide(unsigned char);
long bc_target_off(const unsigned char *);
bool bc_is_athrow(unsigned char);
bool bc_is_return(unsigned char);
bool bc_is_jsr(unsigned char);
bool bc_is_ret(const unsigned char *);
bool bc_is_unconditionnal_branch(const unsigned char *);
bool bc_is_astore(const unsigned char *);
unsigned long bc_get_astore_index(const unsigned char *);
unsigned long bc_get_ret_index(const unsigned char *);
void bc_set_target_off(unsigned char *, long);

void bytecode_disassemble(struct vm_class *, const unsigned char *, unsigned long);

static inline bool bc_branches_to_follower(unsigned char code)
{
	return bc_is_branch(code) && !bc_is_goto(code);
}

#define bytecode_for_each_insn(code, code_length, pc)	\
	for (pc = 0; pc < (code_length); pc += bc_insn_size(code, pc))

struct tableswitch_info {
	uint32_t low;
	uint32_t high;
	int32_t default_target;
	const unsigned char *targets;

	uint32_t count;
	unsigned long insn_size;
};

struct lookupswitch_info {
	int32_t default_target;
	const unsigned char *pairs;

	uint32_t count;
	unsigned long insn_size;
};

unsigned int get_local_var_index(const unsigned char *code, unsigned long pc);

static inline int get_tableswitch_padding(unsigned long pc)
{
	return 3 - (pc % 4);
}

void get_tableswitch_info(const unsigned char *code, unsigned long pc,
			  struct tableswitch_info *info);

void get_lookupswitch_info(const unsigned char *code, unsigned long pc,
			   struct lookupswitch_info *info);

static inline int32_t read_lookupswitch_target(struct lookupswitch_info *info,
					       unsigned int idx)
{
	return read_s32(info->pairs + idx * 8 + 4);
}

static inline int32_t read_lookupswitch_match(struct lookupswitch_info *info,
					      unsigned int idx)
{
	return read_s32(info->pairs + idx * 8);
}

static inline unsigned long bytecode_insn_count(const unsigned char *code,
						unsigned long code_length)
{
	unsigned long count;
	unsigned long pc;

	count = 0;

	bytecode_for_each_insn(code, code_length, pc) {
		count++;
	}

	return count;
}

#endif /* JATO_VM_BYTECODE_H */
