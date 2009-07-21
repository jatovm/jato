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

static enum machine_reg_type register_types[] = {
	[REG_EAX] = REG_TYPE_GPR,
	[REG_EBX] = REG_TYPE_GPR,
	[REG_ECX] = REG_TYPE_GPR,
	[REG_EDX] = REG_TYPE_GPR,
	[REG_EDI] = REG_TYPE_GPR,
	[REG_ESI] = REG_TYPE_GPR,
	[REG_ESP] = REG_TYPE_GPR,
	[REG_EBP] = REG_TYPE_GPR,

	[REG_XMM0] = REG_TYPE_FPU,
	[REG_XMM1] = REG_TYPE_FPU,
	[REG_XMM2] = REG_TYPE_FPU,
	[REG_XMM3] = REG_TYPE_FPU,
	[REG_XMM4] = REG_TYPE_FPU,
	[REG_XMM5] = REG_TYPE_FPU,
	[REG_XMM6] = REG_TYPE_FPU,
	[REG_XMM7] = REG_TYPE_FPU,
};

static const char *register_names[] = {
	[REG_EAX] = "EAX",
	[REG_ECX] = "ECX",
	[REG_EDX] = "EDX",
	[REG_EBX] = "EBX",
	[REG_ESI] = "ESI",
	[REG_EDI] = "EDI",
	[REG_EBP] = "EBP",
	[REG_ESP] = "ESP",
	[REG_XMM0] = "XMM0",
	[REG_XMM1] = "XMM1",
	[REG_XMM2] = "XMM2",
	[REG_XMM3] = "XMM3",
	[REG_XMM4] = "XMM4",
	[REG_XMM5] = "XMM5",
	[REG_XMM6] = "XMM6",
	[REG_XMM7] = "XMM7",
};

const char *reg_name(enum machine_reg reg)
{
	if (reg == REG_UNASSIGNED)
		return "<unassigned>";

	return register_names[reg];
}

enum machine_reg_type reg_type(enum machine_reg reg)
{
	return register_types[reg];
}
