#ifndef ARM_ENCODE_H
#define ARM_ENCODE_H

#include "arch/registers.h"	/* for enum machine_reg */

#include <stdint.h>

struct basic_block;
struct buffer;
struct insn;

uint8_t arm_encode_reg(enum machine_reg);
void insn_encode(struct insn *, struct buffer *, struct basic_block *);
uint32_t encode_base_reg_load(struct insn *);
uint32_t encode_base_reg_load(struct insn *);
uint32_t encode_imm_offset_store(struct insn *);
uint32_t encode_imm_offset_load(struct insn *);

#endif /* ARM_ENCODE_H */
