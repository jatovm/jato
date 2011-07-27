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

static const char *register_names[] = {
	[MACH_REG_R0]	= "r0",
	[MACH_REG_R1]	= "r1",
	[MACH_REG_R2]	= "r2",
	[MACH_REG_R3]	= "r3",
	[MACH_REG_R4]	= "r4",
	[MACH_REG_R5]	= "r5",
	[MACH_REG_R6]	= "r6",
	[MACH_REG_R7]	= "r7",
	[MACH_REG_R8]	= "r8",
	[MACH_REG_R9]	= "r9",
	[MACH_REG_R10]	= "r10",
	[MACH_REG_R11]	= "r11",
	[MACH_REG_R12]	= "r12",
	[MACH_REG_R13]	= "r13",
	[MACH_REG_R14]	= "r14",
	[MACH_REG_R15]	= "r15",
	[MACH_REG_R16]	= "r16",
	[MACH_REG_R17]	= "r17",
	[MACH_REG_R18]	= "r18",
	[MACH_REG_R19]	= "r19",
	[MACH_REG_R20]	= "r20",
	[MACH_REG_R21]	= "r21",
	[MACH_REG_R22]	= "r22",
	[MACH_REG_R23]	= "r23",
	[MACH_REG_R24]	= "r24",
	[MACH_REG_R25]	= "r25",
	[MACH_REG_R26]	= "r26",
	[MACH_REG_R27]	= "r27",
	[MACH_REG_R28]	= "r28",
	[MACH_REG_R29]	= "r29",
	[MACH_REG_R30]	= "r30",
	[MACH_REG_R31]	= "r31",
};

const char *reg_name(enum machine_reg reg)
{
	if (reg == MACH_REG_UNASSIGNED)
		return "-";

	return register_names[reg];
}

bool reg_supports_type(enum machine_reg reg, enum vm_type type)
{
	assert(!"not implemented");
}
