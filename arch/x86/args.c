/*
 * Copyright (C) 2009 Eduard - Gabriel Munteanu <eduard.munteanu@linux360.ro>
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "arch/args.h"

#include "jit/args.h"

#include "vm/method.h"
#include "vm/types.h"

#ifdef CONFIG_X86_64

static enum machine_reg args_map_alloc_gpr(int gpr)
{
	switch (gpr) {
	case 0:
		return MACH_REG_RDI;
	case 1:
		return MACH_REG_RSI;
	case 2:
		return MACH_REG_RDX;
	case 3:
		return MACH_REG_RCX;
	case 4:
		return MACH_REG_R8;
	case 5:
		return MACH_REG_R9;
	default:
		assert(gpr > 5);
		return MACH_REG_UNASSIGNED;
	}
}

int args_map_init(struct vm_method *method)
{
	const char *type = method->type;
	enum vm_type vm_type;
	int idx, gpr_count = 0, stack_count = 0;
	struct vm_args_map *map;
	size_t size;

	/* If the method isn't static, we have a *this. */
	size = method->args_count + !vm_method_is_static(method);

	method->args_map = malloc(sizeof(*map) * size);
	if (!method->args_map)
		return -1;

	/* We know *this is a J_REFERENCE, so allocate a GPR. */
	if (!vm_method_is_static(method)) {
		map = &method->args_map[0];
		map->reg = args_map_alloc_gpr(gpr_count++);
		map->stack_index = -1;
		map->type = J_REFERENCE;
		idx = 1;
	} else
		idx = 0;

	/* Scan the real parameters and assign registers and stack slots. */
	while ((type = parse_method_args(type, &vm_type, NULL))) {
		map = &method->args_map[idx];

		switch (vm_type) {
		case J_BYTE:
		case J_CHAR:
		case J_INT:
		case J_LONG:
		case J_SHORT:
		case J_BOOLEAN:
		case J_REFERENCE:
			map->reg = args_map_alloc_gpr(gpr_count++);
			map->stack_index = -1;
			break;
		case J_FLOAT:
		case J_DOUBLE:
			/*
			 * FIXME: This is wrong, but let's us
			 * not worry about the status of FP.
			 */
			map->reg = MACH_REG_UNASSIGNED;
			map->stack_index = stack_count++;
			break;
		default:
			map->reg = MACH_REG_UNASSIGNED;
			map->stack_index = stack_count++;
			break;
		}

		map->type = vm_type;

		idx++;

		if (gpr_count == 6)
			break;
	}

	/* We're out of GPRs, so the remaining args go on the stack. */
	for (; type && idx < method->args_count; idx++) {
		map = &method->args_map[idx];
		type = parse_method_args(type, &vm_type, NULL);

		map->reg = MACH_REG_UNASSIGNED;
		map->stack_index = stack_count++;
		map->type = vm_type;
	}

	method->reg_args_count = gpr_count;

	return 0;
}

#endif /* CONFIG_X86_64 */
