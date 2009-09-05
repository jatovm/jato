#ifndef __X86_REGISTERS_64_H
#define __X86_REGISTERS_64_H

#include <limits.h>
#include <stdbool.h>

#include "vm/types.h"

enum machine_reg {
	MACH_REG_RAX, /* R0 */
	MACH_REG_RCX, /* R1 */
	MACH_REG_RDX, /* R2 */
	MACH_REG_RBX, /* R3 */
	MACH_REG_RSI, /* R6 */
	MACH_REG_RDI, /* R7 */
	MACH_REG_R8,
	MACH_REG_R9,
	MACH_REG_R10,
	MACH_REG_R11,
	MACH_REG_R12,
	MACH_REG_R13,
	MACH_REG_R14,
	MACH_REG_R15,

	MACH_REG_XMM0,
	MACH_REG_XMM1,
	MACH_REG_XMM2,
	MACH_REG_XMM3,
	MACH_REG_XMM4,
	MACH_REG_XMM5,
	MACH_REG_XMM6,
	MACH_REG_XMM7,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS,

	MACH_REG_RSP = NR_REGISTERS, /* R4 */
	MACH_REG_RBP, /* R5 */

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS,

	/* Either-R**-or-E** variants (for 32/64-bit common code). */
	MACH_REG_xAX = MACH_REG_RAX,
	MACH_REG_xCX = MACH_REG_RCX,
	MACH_REG_xDX = MACH_REG_RDX,
	MACH_REG_xBX = MACH_REG_RBX,
	MACH_REG_xSI = MACH_REG_RSI,
	MACH_REG_xDI = MACH_REG_RDI,
	MACH_REG_xBP = MACH_REG_RBP,
	MACH_REG_xSP = MACH_REG_RSP,

	MACH_REG_UNASSIGNED = INT_MAX,
};

#define GPR_VM_TYPE	J_LONG

#define NR_CALLER_SAVE_REGS 17
extern enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS];

const char *reg_name(enum machine_reg reg);
enum machine_reg_type reg_type(enum machine_reg reg);
bool reg_supports_type(enum machine_reg reg, enum vm_type type);

#endif /* __X86_REGISTERS_64_H */
