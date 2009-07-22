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

enum machine_reg_type reg_type(enum machine_reg reg);

#define GPR_VM_TYPE J_LONG

#endif /* __JIT_REGISTERS_H */
