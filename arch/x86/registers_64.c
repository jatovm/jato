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

#include <assert.h>

static enum machine_reg_type register_types[] = {
	[MACH_REG_RAX] = REG_TYPE_GPR,
	[MACH_REG_RBX] = REG_TYPE_GPR,
	[MACH_REG_RCX] = REG_TYPE_GPR,
	[MACH_REG_RDX] = REG_TYPE_GPR,
	[MACH_REG_RDI] = REG_TYPE_GPR,
	[MACH_REG_RSI] = REG_TYPE_GPR,
	[MACH_REG_R8] = REG_TYPE_GPR,
	[MACH_REG_R9] = REG_TYPE_GPR,
	[MACH_REG_R10] = REG_TYPE_GPR,
	[MACH_REG_R11] = REG_TYPE_GPR,
	[MACH_REG_R12] = REG_TYPE_GPR,
	[MACH_REG_R13] = REG_TYPE_GPR,
	[MACH_REG_R14] = REG_TYPE_GPR,
	[MACH_REG_R15] = REG_TYPE_GPR,
};

static const char *register_names[] = {
	[MACH_REG_RAX] = "RAX",
	[MACH_REG_RCX] = "RCX",
	[MACH_REG_RDX] = "RDX",
	[MACH_REG_RBX] = "RBX",
	[MACH_REG_RSI] = "RSI",
	[MACH_REG_RDI] = "RDI",
	[MACH_REG_R8]  = "R8",
	[MACH_REG_R9]  = "R9",
	[MACH_REG_R10] = "R10",
	[MACH_REG_R11] = "R11",
	[MACH_REG_R12] = "R12",
	[MACH_REG_R13] = "R13",
	[MACH_REG_R14] = "R14",
	[MACH_REG_R15] = "R15",
	[MACH_REG_RSP] = "RSP",
	[MACH_REG_RBP] = "RBP",
};

const char *reg_name(enum machine_reg reg)
{
	if (reg == MACH_REG_UNASSIGNED)
		return "-";

	return register_names[reg];
}

enum machine_reg_type reg_type(enum machine_reg reg)
{
	assert(reg != MACH_REG_UNASSIGNED);

	return register_types[reg];
}
