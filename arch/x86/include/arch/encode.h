#ifndef X86_ENCODE_H
#define X86_ENCODE_H

#include "arch/registers.h"	/* for enum machine_reg */

#include <stdint.h>

struct basic_block;
struct buffer;
struct insn;

#define X86_CALL_INSN_SIZE	5

#define	REX		0x40
#define REX_W		(REX | 8)	/* 64-bit operands */
#define REX_R		(REX | 4)	/* ModRM reg extension */
#define REX_X		(REX | 2)	/* SIB index extension */
#define REX_B		(REX | 1)	/* ModRM r/m extension */

uint8_t x86_encode_reg(enum machine_reg reg);

static inline uint8_t x86_encode_mod_rm(uint8_t mod, uint8_t reg_opcode, uint8_t rm)
{
	return ((mod & 0x3) << 6) | ((reg_opcode & 0x7) << 3) | (rm & 0x7);
}

static inline uint8_t x86_encode_sib(uint8_t scale, uint8_t index, uint8_t base)
{
	return ((scale & 0x3) << 6) | ((index & 0x7) << 3) | (base & 0x7);
}

void insn_encode(struct insn *self, struct buffer *buffer, struct basic_block *bb);

#endif /* X86_ENCODE_H */
