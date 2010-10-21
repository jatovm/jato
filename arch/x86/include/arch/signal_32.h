#ifndef X86_SIGNAL_32_H
#define X86_SIGNAL_32_H

#include <ucontext.h>	/* for gregset_t */

#define REG_IP REG_EIP
#define REG_SP REG_ESP
#define REG_BP REG_EBP

static inline void
save_signal_registers(struct register_state *regs, mcontext_t *mcontext)
{
	greg_t *gregs = mcontext->gregs;

	regs->ip	= (uint32_t) gregs[REG_EIP];
	regs->eax	= gregs[REG_EAX];
	regs->ebx	= gregs[REG_EBX];
	regs->ecx	= gregs[REG_ECX];
	regs->edx	= gregs[REG_EDX];
	regs->esi	= gregs[REG_ESI];
	regs->edi	= gregs[REG_EDI];
}

#endif /* X86_SIGNAL_32_H */
