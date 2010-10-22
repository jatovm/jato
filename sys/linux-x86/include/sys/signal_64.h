#ifndef X86_SIGNAL_64_H
#define X86_SIGNAL_64_H

#include "arch/registers.h"

#include <ucontext.h>	/* for gregset_t */

#define REG_IP REG_RIP
#define REG_SP REG_RSP
#define REG_BP REG_RBP

static inline void
save_signal_registers(struct register_state *regs, mcontext_t *mcontext)
{
	greg_t *gregs = mcontext->gregs;

	regs->ip	= gregs[REG_RIP];
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

#endif /* X86_SIGNAL_64_H */
