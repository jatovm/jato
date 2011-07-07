#ifndef ARM_ENCODE_H
#define ARM_ENCODE_H

#include "arch/registers.h"	/* for enum machine_reg */

#include <stdint.h>

struct basic_block;
struct buffer;
struct insn;

uint8_t arm_encode_reg(enum machine_reg);
void insn_encode(struct insn *, struct buffer *, struct basic_block *);
uint32_t encode_base_reg(struct insn *);

void emit_reg_imm_insn(struct insn *, struct buffer *, struct basic_block *);
void emit_reg_memlocal_insn(struct insn *, struct buffer *, struct basic_block *);

#endif /* ARM_ENCODE_H */
