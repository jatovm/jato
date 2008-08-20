#ifndef __X86_REGISTERS_32_H
#define __X86_REGISTERS_32_H

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

static inline const char *reg_name(enum machine_reg reg)
{
	switch (reg) {
	case REG_EAX:
		return "EAX";
	case REG_EBX:
		return "EBX";
	case REG_ECX:
		return "ECX";
	case REG_EDX:
		return "EDX";
	case REG_ESI:
		return "ESI";
	case REG_EDI:
		return "EDI";
	case REG_EBP:
		return "EBP";
	case REG_ESP:
		return "ESP";
	case REG_UNASSIGNED:
		return "<unassigned>";
	};
	assert(!"not reached");
}

#endif /* __X86_REGISTERS_32_H */
