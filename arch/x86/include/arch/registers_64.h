#ifndef X86_REGISTERS_64_H
#define X86_REGISTERS_64_H

#include "vm/types.h"

#include <stdbool.h>
#include <limits.h>

enum machine_reg {
	MACH_REG_RAX, /* R0 */
	MACH_REG_RCX, /* R1 */
	MACH_REG_RDX, /* R2 */
	MACH_REG_RBX, /* R3 */
	MACH_REG_RSI, /* R6 */
	MACH_REG_RDI, /* R7 */
	MACH_REG_R8,
	MACH_REG_R9,
	MACH_REG_R10,
	MACH_REG_R11,
	MACH_REG_R12,
	MACH_REG_R13,
	MACH_REG_R14,
	MACH_REG_R15,

	/* Number of general purpose registers.  */
	NR_GP_REGISTERS,

	MACH_REG_XMM0 = NR_GP_REGISTERS,
	MACH_REG_XMM1,
	MACH_REG_XMM2,
	MACH_REG_XMM3,
	MACH_REG_XMM4,
	MACH_REG_XMM5,
	MACH_REG_XMM6,
	MACH_REG_XMM7,
	MACH_REG_XMM8,
	MACH_REG_XMM9,
	MACH_REG_XMM10,
	MACH_REG_XMM11,
	MACH_REG_XMM12,
	MACH_REG_XMM13,
	MACH_REG_XMM14,
	MACH_REG_XMM15,

	NR_FP_REGISTERS,

	/* The above registers are available for register allocator.  */
	NR_REGISTERS = NR_FP_REGISTERS,

	MACH_REG_RSP = NR_REGISTERS, /* R4 */
	MACH_REG_RBP, /* R5 */

	/* The above registers are available for get_fixed_var().  */
	NR_FIXED_REGISTERS,

	/* Either-R**-or-E** variants (for 32/64-bit common code). */
	MACH_REG_xAX = MACH_REG_RAX,
	MACH_REG_xCX = MACH_REG_RCX,
	MACH_REG_xDX = MACH_REG_RDX,
	MACH_REG_xBX = MACH_REG_RBX,
	MACH_REG_xSI = MACH_REG_RSI,
	MACH_REG_xDI = MACH_REG_RDI,
	MACH_REG_xBP = MACH_REG_RBP,
	MACH_REG_xSP = MACH_REG_RSP,

	MACH_REG_UNASSIGNED = CHAR_MAX,
};

#define GPR_VM_TYPE	J_LONG
#define FPU_VM_TYPE	J_DOUBLE

#define NR_CALLER_SAVE_REGS	25
extern enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS];

#define NR_CALLEE_SAVE_REGS	5
extern enum machine_reg callee_save_regs[NR_CALLEE_SAVE_REGS];

#define NR_ARG_GP_REGS		6
extern enum machine_reg	arg_gp_regs[NR_ARG_GP_REGS];

#define NR_ARG_XMM_REGS		8
extern enum machine_reg	arg_xmm_regs[NR_ARG_XMM_REGS];

static inline bool is_return_reg(enum machine_reg reg, enum vm_type type)
{
	switch (type) {
	case J_INT:
		return reg == MACH_REG_RAX;
	case J_LONG:
		return reg == MACH_REG_RAX;
	case J_FLOAT:
		return reg == MACH_REG_XMM0;
	case J_DOUBLE:
		return reg == MACH_REG_XMM0;
	default:
		break;
	}
	return false;
}

const char *reg_name(enum machine_reg reg);
enum machine_reg_type reg_type(enum machine_reg reg);
bool reg_supports_type(enum machine_reg reg, enum vm_type type);

struct register_state {
	uint64_t			ip;
	union {
		unsigned long		regs[14];
		struct {
			unsigned long	rax;
			unsigned long	rbx;
			unsigned long	rcx;
			unsigned long	rdx;
			unsigned long	rsi;
			unsigned long	rdi;
			unsigned long	r8;
			unsigned long	r9;
			unsigned long	r10;
			unsigned long	r11;
			unsigned long	r12;
			unsigned long	r13;
			unsigned long	r14;
			unsigned long	r15;
		};
	};
};

static inline enum vm_type reg_default_type(enum machine_reg reg)
{
	if (reg < NR_GP_REGISTERS)
		return GPR_VM_TYPE;
	else if (reg < NR_FP_REGISTERS)
		return FPU_VM_TYPE;
	else
		return GPR_VM_TYPE;
}

static inline bool is_xmm_reg(enum machine_reg reg)
{
	return reg >= NR_GP_REGISTERS && reg < NR_FP_REGISTERS;
}

enum machine_reg args_map_alloc_gpr(int gpr);

#endif /* X86_REGISTERS_64_H */
