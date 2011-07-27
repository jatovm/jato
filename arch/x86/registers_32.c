/*
 * Copyright (c) 2008  Pekka Enberg
 * 
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "arch/registers.h"
#include "jit/vars.h"
#include "vm/system.h"

#include <assert.h>
#include <stdbool.h>

enum machine_reg caller_save_regs[NR_CALLER_SAVE_REGS] = {
	MACH_REG_EAX,
	MACH_REG_ECX,
	MACH_REG_EDX,

	MACH_REG_XMM0,
	MACH_REG_XMM1,
	MACH_REG_XMM2,
	MACH_REG_XMM3,
	MACH_REG_XMM4,
	MACH_REG_XMM5,
	MACH_REG_XMM6,
	MACH_REG_XMM7
};

static const char *register_names[] = {
	[MACH_REG_EAX] = "EAX",
	[MACH_REG_ECX] = "ECX",
	[MACH_REG_EDX] = "EDX",
	[MACH_REG_EBX] = "EBX",
	[MACH_REG_ESI] = "ESI",
	[MACH_REG_EDI] = "EDI",
	[MACH_REG_EBP] = "EBP",
	[MACH_REG_ESP] = "ESP",
	[MACH_REG_XMM0] = "XMM0",
	[MACH_REG_XMM1] = "XMM1",
	[MACH_REG_XMM2] = "XMM2",
	[MACH_REG_XMM3] = "XMM3",
	[MACH_REG_XMM4] = "XMM4",
	[MACH_REG_XMM5] = "XMM5",
	[MACH_REG_XMM6] = "XMM6",
	[MACH_REG_XMM7] = "XMM7",
};

const char *reg_name(enum machine_reg reg)
{
	if (reg == MACH_REG_UNASSIGNED)
		return "-";

	return register_names[reg];
}

#define GPR_32 (1UL << J_REFERENCE) | (1UL << J_INT)
#define GPR_16 (1UL << J_SHORT) | (1UL << J_CHAR)
#define GPR_8 (1UL << J_BYTE) | (1UL << J_BOOLEAN)
#define FPU (1UL << J_FLOAT) | (1UL << J_DOUBLE)

uint32_t reg_type_table[NR_REGISTERS] = {
	[MACH_REG_EAX] = GPR_32 | GPR_16 | GPR_8,
	[MACH_REG_ECX] = GPR_32 | GPR_16 | GPR_8,
	[MACH_REG_EDX] = GPR_32 | GPR_16 | GPR_8,
	[MACH_REG_EBX] = GPR_32 | GPR_16 | GPR_8,

	/* XXX: We can't access the lower nibbles of these registers,
	 * so they shouldn't have GPR_16 or GPR_8, but we need it for
	 * now to work around a reg-alloc bug. */
	[MACH_REG_ESI] = GPR_32 | GPR_16 | GPR_8,
	[MACH_REG_EDI] = GPR_32 | GPR_16 | GPR_8,

	[MACH_REG_XMM0] = FPU,
	[MACH_REG_XMM1] = FPU,
	[MACH_REG_XMM2] = FPU,
	[MACH_REG_XMM3] = FPU,
	[MACH_REG_XMM4] = FPU,
	[MACH_REG_XMM5] = FPU,
	[MACH_REG_XMM6] = FPU,
	[MACH_REG_XMM7] = FPU,
};
