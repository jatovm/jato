#ifndef __JIT_REGISTERS_H
#define __JIT_REGISTERS_H

#define NR_REGISTERS 3	/* available for register allocation */

enum machine_reg {
	R0,
	R1,
	R2,
	REG_UNASSIGNED = ~0UL,
};

#endif /* __JIT_REGISTERS_H */
