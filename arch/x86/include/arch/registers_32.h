#ifndef __X86_REGISTERS_32_H
#define __X86_REGISTERS_32_H

#include <limits.h>
#include <stdbool.h>
#include <assert.h>

enum machine_reg {
	REG_EAX,
	REG_ECX,
	REG_EDX,
	REG_EBX,
	REG_ESI,
	REG_EDI,

	REG_XMM0,
	REG_XMM1,
	REG_XMM2,
	REG_XMM3,
	REG_XMM4,
	REG_XMM5,
	REG_XMM6,
	REG_XMM7,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS,

	REG_EBP = NR_REGISTERS,
	REG_ESP,

	/* Either-R**-or-E** variants (for word-size independent code). */
	REG_XAX = REG_EAX,
	REG_XCX,
	REG_XDX,
	REG_XBX,
	REG_XSI,
	REG_XDI,
	REG_XBP = REG_EBP,
	REG_XSP,

	REG_UNASSIGNED = INT_MAX,
};

const char *reg_name(enum machine_reg reg);
enum machine_reg_type reg_type(enum machine_reg reg);

#endif /* __X86_REGISTERS_32_H */
