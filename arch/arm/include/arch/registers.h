#ifndef ARM_REGISTERS_32_H
#define ARM_REGISTERS_32_H

#include "vm/types.h"

#include <stdbool.h>
#include <limits.h>

enum machine_reg {
	/*
	 * The unbanked registers R0-R10, R11 is
	 * used as Frame pointer
	 */

	MACH_REG_R0,
	MACH_REG_R1,
	MACH_REG_R2,
	MACH_REG_R3,
	MACH_REG_R4,
	MACH_REG_R5,
	MACH_REG_R6,
	MACH_REG_R7,
	MACH_REG_R8,
	MACH_REG_R9,
	MACH_REG_R10,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS = NR_GP_REGISTERS,

	MACH_REG_FP = NR_REGISTERS,
	MACH_REG_SP,

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS,
	/* These registers are special purpose registers */
	MACH_REG_IP,
	MACH_REG_LR,
	MACH_REG_PC,

	MACH_REG_UNASSIGNED = CHAR_MAX,
};

#define GPR_VM_TYPE	J_INT

#define NR_CALLER_SAVE_REGS 0
extern enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS];

const char *reg_name(enum machine_reg reg);

bool reg_supports_type(enum machine_reg reg, enum vm_type type);

struct register_state {
	uint64_t			ip;
	union {
		unsigned long		regs[12];
		struct {
			unsigned long	r0;
			unsigned long	r1;
			unsigned long	r2;
			unsigned long	r3;
			unsigned long	r4;
			unsigned long	r5;
			unsigned long	r6;
			unsigned long	r7;
			unsigned long	r8;
			unsigned long	r9;
			unsigned long	r10;
			unsigned long	fp;
			unsigned long	r12;
			unsigned long	sp;
		};
	};
};

static inline enum vm_type reg_default_type(enum machine_reg reg)
{
	return GPR_VM_TYPE;
}

#endif /* ARM_REGISTERS_32_H */
