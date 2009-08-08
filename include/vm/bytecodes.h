#ifndef __JIT_BYTECODES_H
#define __JIT_BYTECODES_H

#include <stdint.h>
#include <stdbool.h>

struct vm_class;

unsigned long bc_insn_size(const unsigned char *, unsigned long);
bool bc_is_branch(unsigned char);
bool bc_is_goto(unsigned char);
bool bc_is_wide(unsigned char);
long bc_target_off(const unsigned char *);
bool bc_is_athrow(unsigned char);
bool bc_is_return(unsigned char);
bool bc_is_jsr(unsigned char);
bool bc_is_ret(const unsigned char *);
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

void get_tableswitch_info(const unsigned char *code, unsigned long pc,
			  struct tableswitch_info *info);

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

#endif
