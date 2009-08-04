#ifndef __X86_REGISTERS_32_H
#define __X86_REGISTERS_32_H

#include <limits.h>
#include <stdbool.h>
#include <assert.h>
#include <vm/types.h>

enum machine_reg {
	MACH_REG_EAX,
	MACH_REG_ECX,
	MACH_REG_EDX,
	MACH_REG_EBX,
	MACH_REG_ESI,
	MACH_REG_EDI,

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

	MACH_REG_EBP = NR_REGISTERS,
	MACH_REG_ESP,

	/* Either-R**-or-E** variants (for 32/64-bit common code). */
	MACH_REG_xAX = MACH_REG_EAX,
	MACH_REG_xCX = MACH_REG_ECX,
	MACH_REG_xDX = MACH_REG_EDX,
	MACH_REG_xBX = MACH_REG_EBX,
	MACH_REG_xSI = MACH_REG_ESI,
	MACH_REG_xDI = MACH_REG_EDI,
	MACH_REG_xBP = MACH_REG_EBP,
	MACH_REG_xSP = MACH_REG_ESP,

	MACH_REG_UNASSIGNED = INT_MAX,
};

#define GPR_VM_TYPE	J_INT

const char *reg_name(enum machine_reg reg);
enum machine_reg_type reg_type(enum machine_reg reg);

#endif /* __X86_REGISTERS_32_H */
