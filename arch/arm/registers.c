#include "arch/registers.h"
#include "jit/vars.h"
#include "vm/system.h"

#include <assert.h>
#include <stdbool.h>

static const char *register_names[] = {
	[MACH_REG_R0]	= "R0",
	[MACH_REG_R1]	= "R1",
	[MACH_REG_R2]	= "R2",
	[MACH_REG_R3]	= "R3",
	[MACH_REG_R4]	= "R4",
	[MACH_REG_R5]	= "R5",
	[MACH_REG_R6]	= "R6",
	[MACH_REG_R7]	= "R7",
	[MACH_REG_R8]	= "R8",
	[MACH_REG_R9]	= "R9",
	[MACH_REG_R10]	= "R10",
	[MACH_REG_FP]	= "FP",
	[MACH_REG_SP]	= "SP",
	[MACH_REG_IP]	= "IP",
	[MACH_REG_LR]	= "LR",
	[MACH_REG_PC]	= "PC",
};

const char *reg_name(enum machine_reg reg)
{
	if (reg == MACH_REG_UNASSIGNED)
		return "-";

	return register_names[reg];
}

#define GPR_32 ((1UL << J_REFERENCE) | (1UL << J_INT))

const uint32_t reg_type_table[NR_REGISTERS] = {
	[MACH_REG_R0]	= GPR_32,
	[MACH_REG_R1]	= GPR_32,
	[MACH_REG_R2]	= GPR_32,
	[MACH_REG_R3]	= GPR_32,
	[MACH_REG_R4]	= GPR_32,
	[MACH_REG_R5]	= GPR_32,
	[MACH_REG_R6]	= GPR_32,
	[MACH_REG_R7]	= GPR_32,
	[MACH_REG_R8]	= GPR_32,
	[MACH_REG_R9]	= GPR_32,
	[MACH_REG_R10]	= GPR_32,
};

bool reg_supports_type(enum machine_reg reg, enum vm_type type)
{
	assert(reg < NR_REGISTERS);
	assert(type < VM_TYPE_MAX);
	assert(type != J_VOID);
	assert(type != J_RETURN_ADDRESS);
	return reg_type_table[reg] & (1UL << type);
}
