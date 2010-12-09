#ifndef __JIT_REGISTERS_H
#define __JIT_REGISTERS_H

#include <stdbool.h>
#include <limits.h>

#include "vm/types.h"

enum machine_reg {
	R0,
	R1,
	R2,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS = NR_GP_REGISTERS,

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS = NR_REGISTERS,

	MACH_REG_UNASSIGNED		/* Keep this last */
};

#define NR_CALLER_SAVE_REGS 0
extern enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS];

static inline bool reg_supports_type(enum machine_reg reg, enum vm_type type)
{
	return true;
}

#define GPR_VM_TYPE J_LONG

static inline enum vm_type reg_default_type(enum machine_reg reg)
{
	return GPR_VM_TYPE;
}

#endif /* __JIT_REGISTERS_H */
