#ifndef X86_REGISTERS_32_H
#define X86_REGISTERS_32_H

#include "vm/types.h"

#include <ucontext.h>	/* for gregset_t */
#include <stdbool.h>
#include <limits.h>

enum machine_reg {
	MACH_REG_EAX,
	MACH_REG_ECX,
	MACH_REG_EDX,
	MACH_REG_EBX,
	MACH_REG_ESI,
	MACH_REG_EDI,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	MACH_REG_XMM0 = NR_GP_REGISTERS,
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

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS,

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

#define NR_CALLER_SAVE_REGS 11
extern enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS];

const char *reg_name(enum machine_reg reg);

bool reg_supports_type(enum machine_reg reg, enum vm_type type);

struct register_state {
	uint64_t			ip;
	union {
		unsigned long		regs[6];
		struct {
			unsigned long	eax;
			unsigned long	ebx;
			unsigned long	ecx;
			unsigned long	edi;
			unsigned long	edx;
			unsigned long	esi;
		};
	};
};

static inline void
save_signal_registers(struct register_state *regs, gregset_t gregs)
{
	regs->ip	= (uint32_t) gregs[REG_EIP];
	regs->eax	= gregs[REG_EAX];
	regs->ebx	= gregs[REG_EBX];
	regs->ecx	= gregs[REG_ECX];
	regs->edx	= gregs[REG_EDX];
	regs->esi	= gregs[REG_ESI];
	regs->edi	= gregs[REG_EDI];
}

static inline enum vm_type reg_default_type(enum machine_reg reg)
{
	return GPR_VM_TYPE;
}

#endif /* X86_REGISTERS_32_H */
