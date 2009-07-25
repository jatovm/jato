/*
 * Copyright (c) 2009 Tomasz Grabiec
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

#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "jit/cu-mapping.h"
#include "jit/exception.h"
#include "jit/compiler.h"

#include "vm/object.h"

#include "arch/stack-frame.h"
#include "arch/memory.h"
#include "arch/signal.h"

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
	return_address = stack[1];

	signal_exception(exception);

	if (is_native(return_address))
		/* Return to caller. */
		uc->uc_mcontext.gregs[REG_IP] = return_address;
	else
		/* Unwind to previous jit method. */
		uc->uc_mcontext.gregs[REG_IP] = (unsigned long)unwind;

	/* pop EBP from stack */
	stack = (unsigned long*)uc->uc_mcontext.gregs[REG_SP];
	uc->uc_mcontext.gregs[REG_BP] = *stack;
	uc->uc_mcontext.gregs[REG_SP] += sizeof(unsigned long);
}
