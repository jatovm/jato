#ifndef __JIT_REGISTERS_H
#define __JIT_REGISTERS_H

enum machine_reg {
	R0,
	R1,
	R2,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS,

	REG_UNASSIGNED = ~0UL,
};

#endif /* __JIT_REGISTERS_H */
