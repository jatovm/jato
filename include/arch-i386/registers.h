#ifndef __JIT_REGISTERS_H
#define __JIT_REGISTERS_H

#define NR_REGISTERS 4	/* available for register allocator */

enum machine_reg {
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_EBP,
	REG_ESP,
	REG_UNASSIGNED = ~0UL,
};

#endif /* __JIT_REGISTERS_H */
