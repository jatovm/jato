#ifndef __X86_REGISTERS_64_H
#define __X86_REGISTERS_64_H

#define NR_REGISTERS 14	/* available for register allocator */

enum machine_reg {
	REG_RAX, /* R0 */
	REG_RCX, /* R1 */
	REG_RDX, /* R2 */
	REG_RBX, /* R3 */
	REG_RSI, /* R6 */
	REG_RDI, /* R7 */
	REG_R8,
	REG_R9,
	REG_R10,
	REG_R11,
	REG_R12,
	REG_R13,
	REG_R14,
	REG_R15, /* last register available for allocation */
	REG_ESP, /* R4 */
	REG_EBP, /* R5 */
	REG_UNASSIGNED = ~0UL,
};

#endif /* __X86_REGISTERS_64_H */
