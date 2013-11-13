/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "sys/signal.h"

#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "jit/cu-mapping.h"
#include "jit/exception.h"
#include "jit/compiler.h"

#include "vm/object.h"

#include "arch/stack-frame.h"
#include "arch/memory.h"

unsigned char *
throw_exception(struct compilation_unit *cu, struct vm_object *exception)
{
	unsigned char *native_ptr;
	struct jit_stack_frame *frame;

	native_ptr = __builtin_return_address(0) - 1;
	frame      = __builtin_frame_address(1);

	signal_exception(exception);

	return throw_from_jit(cu, frame, native_ptr);
}

void throw_from_trampoline(void *ctx, struct vm_object *exception)
{
	unsigned long return_address;
	unsigned long *stack;
	ucontext_t *uc;

	uc = ctx;

	stack = (unsigned long*)uc->uc_mcontext.gregs[REG_SP];
	return_address = stack[NR_TRAMPOLINE_LOCALS + 1];

	signal_exception(exception);

	if (is_native(return_address))
		/* Return to caller. */
		uc->uc_mcontext.gregs[REG_IP] = return_address;
	else
		/* Unwind to previous jit method. */
		uc->uc_mcontext.gregs[REG_IP] = (unsigned long)unwind;

	/* Cleanup trampoline stack and restore BP. */
	stack += NR_TRAMPOLINE_LOCALS;
	uc->uc_mcontext.gregs[REG_BP] = *stack++;
	uc->uc_mcontext.gregs[REG_SP] = (unsigned long) stack;
}

