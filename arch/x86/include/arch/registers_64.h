#ifndef __X86_REGISTERS_64_H
#define __X86_REGISTERS_64_H

#include <limits.h>
#include <stdbool.h>

enum machine_reg {
	REG_RAX, /* R0 */
	REG_RCX, /* R1 */
	REG_RDX, /* R2 */
	REG_RBX, /* R3 */
	REG_RSI, /* R6 */
	REG_RDI, /* R7 */
	REG_R8,
	REG_R9,
	REG_R10,
	REG_R11,
	REG_R12,
	REG_R13,
	REG_R14,
	REG_R15,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS,

	REG_RSP = NR_REGISTERS, /* R4 */
	REG_RBP, /* R5 */

	/* Either-R**-or-E** variants (for 32/64-bit common code). */
	REG_xAX = REG_RAX,
	REG_xCX = REG_RCX,
	REG_xDX = REG_RDX,
	REG_xBX = REG_RBX,
	REG_xSI = REG_RSI,
	REG_xDI = REG_RDI,
	REG_xBP = REG_RBP,
	REG_xSP = REG_RSP,

	REG_UNASSIGNED = INT_MAX,
};

#define GPR_VM_TYPE	J_LONG

const char *reg_name(enum machine_reg reg);
enum machine_reg_type reg_type(enum machine_reg reg);

static inline bool is_caller_saved_reg(enum machine_reg reg)
{
	/*
	 * As per x86-64 ABI:
	 *
	 * Registers %rbp, %rbx, and %r12 through %r15 "belong" to the calling
	 * function and the called function is required to preserve their
	 * values.
	 */
	if (reg == REG_RAX || reg == REG_RCX || reg == REG_RDX)
		return true;

	if (reg >= REG_RSI && reg <= REG_R11)
		return true;

	return false;
}

#endif /* __X86_REGISTERS_64_H */
