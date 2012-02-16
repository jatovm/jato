#ifndef __JIT_REGISTERS_H
#define __JIT_REGISTERS_H

#include <stdbool.h>
#include <limits.h>

#include "vm/types.h"

enum machine_reg {
	MACH_REG_R0,
	MACH_REG_R1,
	MACH_REG_R2,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS = NR_GP_REGISTERS,

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS = NR_REGISTERS,

	MACH_REG_UNASSIGNED = CHAR_MAX,
};

struct register_state {
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

static inline enum machine_reg args_map_alloc_gpr(int gpr)
{
	return gpr;
}

#endif /* __JIT_REGISTERS_H */
