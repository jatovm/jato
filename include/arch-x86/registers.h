#ifndef __JIT_REGISTERS_H
#define __JIT_REGISTERS_H

#define NR_REGISTERS 6	/* available for register allocator */

enum machine_reg {
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_ESI,
	REG_EDI,	/* last register available for allocation */
	REG_EBP,
	REG_ESP,
	REG_UNASSIGNED = ~0UL,
};

#endif /* __JIT_REGISTERS_H */
