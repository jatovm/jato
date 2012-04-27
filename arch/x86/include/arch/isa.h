#ifndef JATO_X86_ISA_H
#define JATO_X86_ISA_H

#define X86_CALL_INSN_SIZE		5
#define X86_CALL_DISP_OFFSET		1
#define X86_CALL_OPC 			0xe8

static inline unsigned long x86_call_disp(void *callsite, void *target)
{
	return (unsigned long) target - (unsigned long) callsite - X86_CALL_INSN_SIZE;
}

#endif /* JATO_X86_ISA_H */
