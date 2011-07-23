#ifndef ARM_ENCODE_H
#define ARM_ENCODE_H

#include "arch/registers.h"	/* for enum machine_reg */

#include <stdint.h>

#define PC_RELATIVE_OFFSET	8
struct basic_block;
struct buffer;
struct insn;

uint8_t arm_encode_reg(enum machine_reg);
void insn_encode(struct insn *, struct buffer *, struct basic_block *);
uint32_t encode_base_reg_load(struct insn *);
uint32_t encode_base_reg_store(struct insn *);
uint32_t encode_imm_offset_store(struct insn *);
uint32_t encode_imm_offset_load(struct insn *);
long emit_branch(struct insn *, struct basic_block *);
long branch_rel_addr(struct insn *, unsigned long);
static inline void write_imm24(struct buffer *, struct insn *, unsigned long, unsigned long);
void encode_stm(struct buffer *, uint16_t);
void encode_setup_fp(struct buffer *, unsigned long);
void encode_sub_sp(struct buffer *, unsigned long);
#endif /* ARM_ENCODE_H */
