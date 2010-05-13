#ifndef X86_ENCODE_H
#define X86_ENCODE_H

#include "arch/registers.h"	/* for enum machine_reg */

#include <stdint.h>

uint8_t x86_encode_reg(enum machine_reg reg);

static inline uint8_t x86_encode_mod_rm(uint8_t mod, uint8_t reg_opcode, uint8_t rm)
{
	return ((mod & 0x3) << 6) | ((reg_opcode & 0x7) << 3) | (rm & 0x7);
}

static inline uint8_t x86_encode_sib(uint8_t scale, uint8_t index, uint8_t base)
{
	return ((scale & 0x3) << 6) | ((index & 0x7) << 3) | (base & 0x7);
}


#endif /* X86_ENCODE_H */
