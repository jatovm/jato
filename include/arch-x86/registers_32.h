#ifndef __X86_REGISTERS_32_H
#define __X86_REGISTERS_32_H

#include <stdbool.h>
#include <assert.h>

#define NR_REGISTERS 6	/* available for register allocator */

enum machine_reg {
	REG_EAX,
	REG_ECX,
	REG_EDX,
	REG_EBX,
	REG_ESI,
	REG_EDI,	/* last register available for allocation */
	REG_EBP,
	REG_ESP,
	REG_UNASSIGNED = ~0UL,
};

const char *reg_name(enum machine_reg reg);

static inline bool is_caller_saved_reg(enum machine_reg reg)
{
	return reg == REG_EAX || reg == REG_ECX || reg == REG_EDX;
}

#endif /* __X86_REGISTERS_32_H */
