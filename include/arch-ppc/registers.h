#ifndef __PPC_REGISTERS_H
#define __PPC_REGISTERS_H

#define NR_REGISTERS 1	/* available for register allocation */

enum machine_reg {
	GPR0,
	REG_UNASSIGNED = ~0UL,
};

#endif /* __PPC_REGISTERS_H */
