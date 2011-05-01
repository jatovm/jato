#ifndef X86_REGISTERS_32_H
#define X86_REGISTERS_32_H

#include "vm/types.h"

#include <stdbool.h>
#include <limits.h>

enum machine_reg {
	MACH_REG_EAX,
	MACH_REG_ECX,
	MACH_REG_EDX,
	MACH_REG_EBX,
	MACH_REG_ESI,
	MACH_REG_EDI,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	MACH_REG_XMM0 = NR_GP_REGISTERS,
	MACH_REG_XMM1,
	MACH_REG_XMM2,
	MACH_REG_XMM3,
	MACH_REG_XMM4,
	MACH_REG_XMM5,
	MACH_REG_XMM6,
	MACH_REG_XMM7,

	NR_FP_REGISTERS,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS = NR_FP_REGISTERS,

	MACH_REG_EBP = NR_REGISTERS,
	MACH_REG_ESP,

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS,

	/* Either-R**-or-E** variants (for 32/64-bit common code). */
	MACH_REG_xAX = MACH_REG_EAX,
	MACH_REG_xCX = MACH_REG_ECX,
	MACH_REG_xDX = MACH_REG_EDX,
	MACH_REG_xBX = MACH_REG_EBX,
	MACH_REG_xSI = MACH_REG_ESI,
	MACH_REG_xDI = MACH_REG_EDI,
	MACH_REG_xBP = MACH_REG_EBP,
	MACH_REG_xSP = MACH_REG_ESP,

	MACH_REG_UNASSIGNED = CHAR_MAX,
};

#define GPR_VM_TYPE	J_INT

#define NR_CALLER_SAVE_REGS 11
extern enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS];

const char *reg_name(enum machine_reg reg);

const uint32_t reg_type_table[NR_REGISTERS];

static inline bool reg_supports_type(enum machine_reg reg, enum vm_type type)
{
	return reg_type_table[reg] & (1UL << type);
}

struct register_state {
	uint64_t			ip;
	union {
		unsigned long		regs[6];
		struct {
			unsigned long	eax;
			unsigned long	ebx;
			unsigned long	ecx;
			unsigned long	edi;
			unsigned long	edx;
			unsigned long	esi;
		};
	};
};

static inline enum vm_type reg_default_type(enum machine_reg reg)
{
	return GPR_VM_TYPE;
}

static inline bool is_xmm_reg(enum machine_reg reg)
{
	return reg >= NR_GP_REGISTERS && reg < NR_FP_REGISTERS;
}

#endif /* X86_REGISTERS_32_H */
