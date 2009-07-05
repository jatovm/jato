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

static const char *register_names[] = {
	[REG_RAX] = "RAX",
	[REG_RCX] = "RCX",
	[REG_RDX] = "RDX",
	[REG_RBX] = "RBX",
	[REG_RSI] = "RSI",
	[REG_RDI] = "RDI",
	[REG_R8]  = "R8",
	[REG_R9]  = "R9",
	[REG_R10] = "R10",
	[REG_R11] = "R11",
	[REG_R12] = "R12",
	[REG_R13] = "R13",
	[REG_R14] = "R14",
	[REG_R15] = "R15",
	[REG_RSP] = "RSP",
	[REG_RBP] = "RBP",
};

const char *reg_name(enum machine_reg reg)
{
	if (reg == REG_UNASSIGNED)
		return "<unassigned>";

	return register_names[reg];
}
