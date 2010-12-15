#ifndef ARM_REGISTERS_32_H
#define ARM_REGISTERS_32_H

#include "vm/types.h"

#include <stdbool.h>
#include <limits.h>

enum machine_reg {
	/* The unbanked registers, R0-R7 */
	MACH_REG_R0,
	MACH_REG_R1,
	MACH_REG_R2,
	MACH_REG_R3,
	MACH_REG_R4,
	MACH_REG_R5,
	MACH_REG_R6,
	MACH_REG_R7,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS,

	/* The Stack Pointer */
	MACH_REG_R13 = NR_REGISTERS,

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS,

	MACH_REG_SP = MACH_REG_R13,

	MACH_REG_UNASSIGNED = INT_MAX,
};

#define GPR_VM_TYPE	J_INT

#define NR_CALLER_SAVE_REGS 0
extern enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS];

const char *reg_name(enum machine_reg reg);

bool reg_supports_type(enum machine_reg reg, enum vm_type type);

struct register_state {
	uint64_t			ip;
};

static inline enum vm_type reg_default_type(enum machine_reg reg)
{
	return GPR_VM_TYPE;
}

#endif /* ARM_REGISTERS_32_H */
