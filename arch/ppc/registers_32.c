#include "arch/registers.h"

enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS] = {
	MACH_REG_R0,
	MACH_REG_R3,
	MACH_REG_R4,
	MACH_REG_R5,
	MACH_REG_R6,
	MACH_REG_R7,
	MACH_REG_R8,
	MACH_REG_R9,
	MACH_REG_R10,
	MACH_REG_R11,
	MACH_REG_R12,
};
