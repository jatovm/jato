/*
 * Copyright (C) 2009 Eduard - Gabriel Munteanu <eduard.munteanu@linux360.ro>
 * 
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "jit/args.h"

#include "vm/method.h"
#include "vm/types.h"

#ifdef CONFIG_X86_64

enum machine_reg args_map_alloc_gpr(int gpr)
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

static enum machine_reg args_map_alloc_xmm(int xmm)
{
	switch (xmm) {
	case 0:
		return MACH_REG_XMM0;
	case 1:
		return MACH_REG_XMM1;
	case 2:
		return MACH_REG_XMM2;
	case 3:
		return MACH_REG_XMM3;
	case 4:
		return MACH_REG_XMM4;
	case 5:
		return MACH_REG_XMM5;
	case 6:
		return MACH_REG_XMM6;
	case 7:
		return MACH_REG_XMM7;
	default:
		assert(xmm > 7);
		return MACH_REG_UNASSIGNED;
	}
}

static void args_map_dup_slot(struct vm_args_map *orig, int *st)
{

	struct vm_args_map *new = &orig[1];

	new->reg = orig->reg;
	new->type = orig->type;
	if (orig->stack_index == -1)
		new->stack_index = -1;
	else
		new->stack_index = (*st)++;

	(*st)++;
}

static void args_map_assign(struct vm_args_map *map,
			    enum vm_type vm_type,
			    int *gpr_count,
			    int *stack_count)
{
	map->type = vm_type;
	if (*gpr_count < 6) {
		map->reg = args_map_alloc_gpr((*gpr_count)++);
		map->stack_index = -1;
	} else {
		map->reg = MACH_REG_UNASSIGNED;
		map->stack_index = (*stack_count)++;
	}
}

int args_map_init(struct vm_method *method)
{
	struct vm_method_arg *arg;
	enum vm_type vm_type;
	int idx = 0;
	int gpr_count = 0, xmm_count = 0, stack_count = 0;
	struct vm_args_map *map;
	size_t size;

	/* If the method isn't static, we have a *this. */
	size = method->args_count + !vm_method_is_static(method);

	if (vm_method_is_jni(method)) {
		if (vm_method_is_static(method))
			size++;

		size++;
	}

	method->args_map = malloc(sizeof(*map) * size);
	if (!method->args_map)
		return -1;

	if (vm_method_is_jni(method)) {
		if (vm_method_is_static(method)) {
			map = &method->args_map[idx++];
			args_map_assign(map, J_REFERENCE, &gpr_count, &stack_count);
		}
		map = &method->args_map[idx++];
		args_map_assign(map, J_REFERENCE, &gpr_count, &stack_count);
	}

	/* We know *this is a J_REFERENCE, so allocate a GPR. */
	if (!vm_method_is_static(method)) {
		map = &method->args_map[idx++];
		args_map_assign(map, J_REFERENCE, &gpr_count, &stack_count);
	}

	method->reg_args_count = 0;

	/* Scan the real parameters and assign registers and stack slots. */
	list_for_each_entry(arg, &method->args, list_node) {
		map = &method->args_map[idx];
		vm_type = arg->type_info.vm_type;

		switch (vm_type) {
		case J_BYTE:
		case J_CHAR:
		case J_INT:
		case J_LONG:
		case J_SHORT:
		case J_BOOLEAN:
		case J_REFERENCE:
			args_map_assign(map, vm_type, &gpr_count, &stack_count);
			break;
		case J_FLOAT:
		case J_DOUBLE:
			if (xmm_count < 8) {
				map->reg = args_map_alloc_xmm(xmm_count++);
				map->stack_index = -1;
			} else {
				map->reg = MACH_REG_UNASSIGNED;
				map->stack_index = stack_count++;
			}
			break;
		default:
			map->reg = MACH_REG_UNASSIGNED;
			map->stack_index = stack_count++;
			break;
		}

		map->type = vm_type;

		if (vm_type == J_LONG || vm_type == J_DOUBLE) {
			args_map_dup_slot(map, &stack_count);
			method->reg_args_count++;
			idx += 2;
		} else
			idx++;
	}

	method->reg_args_count += gpr_count + xmm_count;

	return 0;
}

#endif /* CONFIG_X86_64 */
