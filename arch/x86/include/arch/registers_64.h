#ifndef X86_REGISTERS_64_H
#define X86_REGISTERS_64_H

#include "vm/types.h"

#include <ucontext.h>  /* for gregset_t */
#include <stdbool.h>
#include <limits.h>

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

struct register_state {
	union {
		unsigned long		regs[14];
		struct {
			unsigned long	rax;
			unsigned long	rbx;
			unsigned long	rcx;
			unsigned long	rdx;
			unsigned long	rsi;
			unsigned long	rdi;
			unsigned long	r8;
			unsigned long	r9;
			unsigned long	r10;
			unsigned long	r11;
			unsigned long	r12;
			unsigned long	r13;
			unsigned long	r14;
			unsigned long	r15;
		};
	};
};

#define SAVE_REG(reg, dst)  \
	__asm__ volatile ("movq %%" reg ", %0\n\t" : "=m"(dst))

static inline void save_registers(struct register_state *regs)
{
	SAVE_REG("rax", regs->rax);
	SAVE_REG("rbx", regs->rbx);
	SAVE_REG("rcx", regs->rcx);
	SAVE_REG("rdx", regs->rdx);
	SAVE_REG("rsi", regs->rsi);
	SAVE_REG("rdi", regs->rdi);
	SAVE_REG("r8",  regs->r8);
	SAVE_REG("r9",  regs->r9);
	SAVE_REG("r10", regs->r10);
	SAVE_REG("r11", regs->r11);
	SAVE_REG("r12", regs->r12);
	SAVE_REG("r13", regs->r13);
	SAVE_REG("r14", regs->r14);
	SAVE_REG("r15", regs->r15);
}

static inline void
save_signal_registers(struct register_state *regs, gregset_t gregs)
{
        regs->rax	= gregs[REG_RAX];
        regs->rbx	= gregs[REG_RBX];
        regs->rcx	= gregs[REG_RCX];
        regs->rdx	= gregs[REG_RDX];
        regs->rsi	= gregs[REG_RSI];
        regs->rdi	= gregs[REG_RDI];
        regs->r8	= gregs[REG_R8];
        regs->r9	= gregs[REG_R9];
        regs->r10	= gregs[REG_R10];
        regs->r11	= gregs[REG_R11];
        regs->r12	= gregs[REG_R12];
        regs->r13	= gregs[REG_R13];
        regs->r14	= gregs[REG_R14];
        regs->r15	= gregs[REG_R15];
}

#endif /* X86_REGISTERS_64_H */
