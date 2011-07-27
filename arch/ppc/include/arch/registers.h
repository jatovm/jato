#ifndef __PPC_REGISTERS_H
#define __PPC_REGISTERS_H

#include "vm/types.h"

#include <limits.h>

enum machine_reg {
	/* Volatile registers: */

	MACH_REG_R0,

	/* First argument word; first word of function return value.  */
	MACH_REG_R3,

	/* Second argument word; second word function return value.  */
	MACH_REG_R4,

	/* 3rd - 8th argument words.  */
	MACH_REG_R5,
	MACH_REG_R6,
	MACH_REG_R7,
	MACH_REG_R8,
	MACH_REG_R9,
	MACH_REG_R10,

	/* Non-volatile registers (values are preserved across procedure
	   calls):  */
	MACH_REG_R13,
	MACH_REG_R14,
	MACH_REG_R15,
	MACH_REG_R16,
	MACH_REG_R17,
	MACH_REG_R18,
	MACH_REG_R19,
	MACH_REG_R20,
	MACH_REG_R21,
	MACH_REG_R22,
	MACH_REG_R23,
	MACH_REG_R24,
	MACH_REG_R25,
	MACH_REG_R26,
	MACH_REG_R27,
	MACH_REG_R28,
	MACH_REG_R29,
	MACH_REG_R30,
	MACH_REG_R31,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	/* The above registers are available for register allocation.  */
	NR_REGISTERS = NR_GP_REGISTERS,

	/* Stack pointer.  */
	MACH_REG_R1 = NR_REGISTERS,

	/* Table of Contents (TOC) pointer.  */
	MACH_REG_R2,

	/* Used in calls by pointer and as an environment pointer.   */
	MACH_REG_R11,

	/* Used for special exception handling and in glink code.  */
	MACH_REG_R12,

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS,

	MACH_REG_UNASSIGNED = CHAR_MAX,
};

#define NR_CALLER_SAVE_REGS 11

extern enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS];

#define GPR_VM_TYPE J_INT

static inline enum vm_type reg_default_type(enum machine_reg reg)
{
	return GPR_VM_TYPE;
}

#endif /* __PPC_REGISTERS_H */
