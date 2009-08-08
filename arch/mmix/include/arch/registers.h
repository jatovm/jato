#ifndef __JIT_REGISTERS_H
#define __JIT_REGISTERS_H

#include <stdbool.h>

#include "vm/types.h"

enum machine_reg {
	R0,
	R1,
	R2,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS,

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS = NR_REGISTERS,

	MACH_REG_UNASSIGNED = ~0UL,
};

enum machine_reg_type reg_type(enum machine_reg reg);

static inline bool reg_supports_type(enum machine_reg reg, enum vm_type type)
{
	return true;
}

#define GPR_VM_TYPE J_LONG

#endif /* __JIT_REGISTERS_H */
