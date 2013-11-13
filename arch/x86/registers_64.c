/*
 * Copyright (c) 2008, 2012  Pekka Enberg
 * 
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "arch/registers.h"
#include "jit/vars.h"

#include <assert.h>

enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS] = {
	MACH_REG_RAX,
	MACH_REG_RDI,
	MACH_REG_RSI,
	MACH_REG_RDX,
	MACH_REG_RCX,
	MACH_REG_R8,
	MACH_REG_R9,
	MACH_REG_R10,
	MACH_REG_R11,

	MACH_REG_XMM0,
	MACH_REG_XMM1,
	MACH_REG_XMM2,
	MACH_REG_XMM3,
	MACH_REG_XMM4,
	MACH_REG_XMM5,
	MACH_REG_XMM6,
	MACH_REG_XMM7,
	MACH_REG_XMM8,
	MACH_REG_XMM9,
	MACH_REG_XMM10,
	MACH_REG_XMM11,
	MACH_REG_XMM12,
	MACH_REG_XMM13,
	MACH_REG_XMM14,
	MACH_REG_XMM15,
};

enum machine_reg callee_save_regs[NR_CALLEE_SAVE_REGS] = {
	MACH_REG_RBX,
	MACH_REG_R12,
	MACH_REG_R13,
	MACH_REG_R14,
	MACH_REG_R15,
};

enum machine_reg arg_gp_regs[NR_ARG_GP_REGS] = {
	MACH_REG_RDI,
	MACH_REG_RSI,
	MACH_REG_RDX,
	MACH_REG_RCX,
	MACH_REG_R8,
	MACH_REG_R9,
};

enum machine_reg arg_xmm_regs[NR_ARG_XMM_REGS] = {
	MACH_REG_XMM0,
	MACH_REG_XMM1,
	MACH_REG_XMM2,
	MACH_REG_XMM3,
	MACH_REG_XMM4,
	MACH_REG_XMM5,
	MACH_REG_XMM6,
	MACH_REG_XMM7,
};

static const char *register_names[] = {
	[MACH_REG_RAX] = "RAX",
	[MACH_REG_RCX] = "RCX",
	[MACH_REG_RDX] = "RDX",
	[MACH_REG_RBX] = "RBX",
	[MACH_REG_RSI] = "RSI",
	[MACH_REG_RDI] = "RDI",
	[MACH_REG_R8]  = "R8",
	[MACH_REG_R9]  = "R9",
	[MACH_REG_R10] = "R10",
	[MACH_REG_R11] = "R11",
	[MACH_REG_R12] = "R12",
	[MACH_REG_R13] = "R13",
	[MACH_REG_R14] = "R14",
	[MACH_REG_R15] = "R15",
	[MACH_REG_RSP] = "RSP",
	[MACH_REG_RBP] = "RBP",

	[MACH_REG_XMM0]		= "XMM0",
	[MACH_REG_XMM1]		= "XMM1",
	[MACH_REG_XMM2]		= "XMM2",
	[MACH_REG_XMM3]		= "XMM3",
	[MACH_REG_XMM4]		= "XMM4",
	[MACH_REG_XMM5]		= "XMM5",
	[MACH_REG_XMM6]		= "XMM6",
	[MACH_REG_XMM7]		= "XMM7",
	[MACH_REG_XMM8]		= "XMM8",
	[MACH_REG_XMM9]		= "XMM9",
	[MACH_REG_XMM10]	= "XMM10",
	[MACH_REG_XMM11]	= "XMM11",
	[MACH_REG_XMM12]	= "XMM12",
	[MACH_REG_XMM13]	= "XMM13",
	[MACH_REG_XMM14]	= "XMM14",
	[MACH_REG_XMM15]	= "XMM15",
};

const char *reg_name(enum machine_reg reg)
{
	if (reg == MACH_REG_UNASSIGNED)
		return "-";

	return register_names[reg];
}

#define GPR_64		((1UL << J_LONG)  | (1UL << J_REFERENCE))
#define GPR_32		((1UL << J_INT)                         )
#define GPR_16		((1UL << J_SHORT) | (1UL << J_CHAR)     )
#define GPR_8		((1UL << J_BYTE)  | (1UL << J_BOOLEAN)  )
#define FPU		((1UL << J_FLOAT) | (1UL << J_DOUBLE)   )

bool reg_supports_type(enum machine_reg reg, enum vm_type type)
{
	static const uint32_t table[NR_REGISTERS] = {
		[MACH_REG_RAX]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_RCX]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_RDX]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_RBX]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_R8 ]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_R9 ]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_R10]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_R11]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_R12]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_R13]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_R14]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_R15] 	= GPR_64 | GPR_32 | GPR_16 | GPR_8,

		/* XXX: We can't access the lower nibbles of these registers,
		 * so they shouldn't have GPR_16 or GPR_8, but we need it for
		 * now to work around a reg-alloc bug. */
		[MACH_REG_RSI]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,
		[MACH_REG_RDI]	= GPR_64 | GPR_32 | GPR_16 | GPR_8,

		[MACH_REG_XMM0]	= FPU,
		[MACH_REG_XMM1]	= FPU,
		[MACH_REG_XMM2]	= FPU,
		[MACH_REG_XMM3] = FPU,
		[MACH_REG_XMM4]	= FPU,
		[MACH_REG_XMM5]	= FPU,
		[MACH_REG_XMM6]	= FPU,
		[MACH_REG_XMM7]	= FPU,
	};

	assert(reg < NR_REGISTERS);
	assert(type < VM_TYPE_MAX);
	assert(type != J_VOID);
	assert(type != J_RETURN_ADDRESS);

	return table[reg] & (1UL << type);
}
