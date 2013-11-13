/*
 * Copyright (C) 2006-2008  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include <assert.h>
#include <stdlib.h>

#include "jit/compilation-unit.h"
#include "jit/expression.h"
#include "jit/compiler.h"
#include "jit/args.h"
#include "jit/text.h"
#include "jit/debug.h"

#include "vm/stack-trace.h"
#include "vm/method.h"
#include "vm/vm.h"

#include "arch/stack-frame.h"

/*
 * The stack frame layout is illustrated by the following diagram:
 *
 *     :              :  ^
 *     :              :  |  Higher memory addresses
 *     +--------------+  |
 *     |    Arg n     |
 *     :     ...      :
 *     |    Arg 1     | <-- Start offset of arguments
 *     |   Old EIP    |
 *     |   Old EBP    | <-- New EBP
 *     |   Local 1    |
 *     :     ...      :
 *     |   Local m    :
 *     +--------------+
 */

#define ARGS_START_OFFSET offsetof(struct jit_stack_frame, args)

#ifdef CONFIG_X86_32
static unsigned long cu_frame_misc_size(struct compilation_unit *cu)
{
	return (opt_debug_stack ? sizeof(unsigned long) : 0);	/* canary */
}
#else
static unsigned long cu_frame_misc_size(struct compilation_unit *cu)
{
	return sizeof(unsigned long) +				/* *this */
	       (opt_debug_stack ? sizeof(unsigned long) : 0);	/* canary */
}
#endif

static unsigned long __index_to_offset(unsigned long index)
{
	return index * sizeof(unsigned long);
}

static unsigned long
index_to_offset(unsigned long idx, int size, unsigned long nr_args)
{
	if (idx < nr_args)
		return ARGS_START_OFFSET + __index_to_offset(idx);

	return 0UL - __index_to_offset(idx - nr_args + size);
}

unsigned long frame_local_offset(struct vm_method *method,
				 struct expression *local)
{
	unsigned long idx, nr_args;

	idx = local->local_index;
	nr_args = get_stack_args_count(method);

	return index_to_offset(idx, vm_type_slot_size(local->vm_type), nr_args);
}

unsigned long slot_offset(struct stack_slot *slot)
{
	struct stack_frame *frame = slot->parent;

	return index_to_offset(slot->index, 1, frame->nr_args);
}

unsigned long slot_offset_64(struct stack_slot *slot)
{
	struct stack_frame *frame = slot->parent;

	return index_to_offset(slot->index, 2, frame->nr_args);
}

unsigned long frame_locals_size(struct stack_frame *frame)
{
	unsigned long nr_locals;
	unsigned long size;

	assert(frame->nr_local_slots >= frame->nr_args);

	nr_locals = frame->nr_local_slots - frame->nr_args;

	size = __index_to_offset(nr_locals + frame->nr_spill_slots);

	return ALIGN(size, X86_STACK_ALIGN);
}

/*
 * Returns total offset to subtract from ESP to reserve space for locals.
 */
unsigned long cu_frame_locals_offset(struct compilation_unit *cu)
{
	return frame_locals_size(cu->stack_frame);
}

/*
 * Returns offset to subtract from the
 * stack pointer to reserve space for the entire stack frame.
 */
unsigned long cu_frame_total_offset(struct compilation_unit *cu)
{
	return cu_frame_locals_offset(cu) +
	       sizeof(unsigned long) * NR_CALLEE_SAVE_REGS +
	       cu_frame_misc_size(cu);
}

/*
 * Checks whether given native function was called from jit trampoline
 * code. It checks whether return address points after a relative call
 * to jit_magic_trampoline, which is typical for trampolines.
 */
bool called_from_jit_trampoline(struct native_stack_frame *frame)
{
	void *call_target;
	void **call_site;

	call_site = (void *)frame->return_address - sizeof(void *);
	if (!is_jit_text(call_site))
		return false;

	call_target = *call_site + frame->return_address;

	return call_target == &jit_magic_trampoline;
}
