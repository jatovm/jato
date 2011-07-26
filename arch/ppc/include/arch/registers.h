#ifndef __PPC_REGISTERS_H
#define __PPC_REGISTERS_H

#include "vm/types.h"

#include <limits.h>

enum machine_reg {
	/* Volatile registers: */

	REG_R0,

	/* First argument word; first word of function return value.  */
	REG_R3,

	/* Second argument word; second word function return value.  */
	REG_R4,

	/* 3rd - 8th argument words.  */
	REG_R5,
	REG_R6,
	REG_R7,
	REG_R8,
	REG_R9,
	REG_R10,

	/* Non-volatile registers (values are preserved across procedure
	   calls):  */
	REG_R13,
	REG_R14,
	REG_R15,
	REG_R16,
	REG_R17,
	REG_R18,
	REG_R19,
	REG_R20,
	REG_R21,
	REG_R22,
	REG_R23,
	REG_R24,
	REG_R25,
	REG_R26,
	REG_R27,
	REG_R28,
	REG_R29,
	REG_R30,
	REG_R31,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	/* The above registers are available for register allocation.  */
	NR_REGISTERS = NR_GP_REGISTERS,

	/* Stack pointer.  */
	REG_R1 = NR_REGISTERS,

	/* Table of Contents (TOC) pointer.  */
	REG_R2,

	/* Used in calls by pointer and as an environment pointer.   */
	REG_R11,

	/* Used for special exception handling and in glink code.  */
	REG_R12,

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS,

	MACH_REG_UNASSIGNED = CHAR_MAX,
};

#define GPR_VM_TYPE J_INT

static inline enum vm_type reg_default_type(enum machine_reg reg)
{
	return GPR_VM_TYPE;
}

#endif /* __PPC_REGISTERS_H */
