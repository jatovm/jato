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

#include <cafebabe/code_attribute.h>

#include <jit/exception.h>
#include <jit/compilation-unit.h>
#include <jit/bc-offset-mapping.h>
#include <jit/basic-block.h>
#include <jit/exception.h>
#include <jit/compiler.h>

#include <vm/buffer.h>
#include <vm/class.h>
#include <vm/method.h>
#include <vm/object.h>
#include <vm/thread.h>

#include <arch/stack-frame.h>
#include <arch/instruction.h>
#include <errno.h>

struct cafebabe_code_attribute_exception *
exception_find_entry(struct vm_method *method, unsigned long target)
{
	int i;

	for (i = 0; i < method->code_attribute.exception_table_length; i++) {
		struct cafebabe_code_attribute_exception *eh
			= &method->code_attribute.exception_table[i];

		if (eh->handler_pc == target)
			return eh;
	}

	return NULL;
}

static unsigned char *eh_native_ptr(struct compilation_unit *cu,
	struct cafebabe_code_attribute_exception *eh)
{
	struct basic_block *bb = find_bb(cu, eh->handler_pc);

	assert(bb != NULL);

	return bb_native_ptr(bb);
}

/**
 * find_handler - return native pointer to exception handler for given
 *                @exception_class and @bc_offset of source.
 */
static unsigned char *find_handler(struct compilation_unit *cu,
				   struct vm_class *exception_class,
				   unsigned long bc_offset)
{
	struct cafebabe_code_attribute_exception *eh;
	struct vm_method *method = cu->method;
	int size = method->code_attribute.exception_table_length;
	int i;

	for (i = 0; i < size; i++) {
		eh = &method->code_attribute.exception_table[i];

		if (exception_covers(eh, bc_offset)) {
			struct vm_class *catch_class;

			if (eh->catch_type == 0)
				break; /* It's a finally block */

			catch_class = vm_class_resolve_class(method->class,
				eh->catch_type);

			NOT_IMPLEMENTED;
#if 0
			if (isInstanceOf(catch_class, exception_class))
				break;
#endif
		}
	}

	if (i < size)
		return eh_native_ptr(cu, eh);

	return NULL;
}

/**
 * throw_exception_from - returns native pointer inside jitted method
 *                        that sould be executed to handle exception.
 *                        This can be one of the following:
 *                        1) registered exception handler (catch/finally block)
 *                        2) method's unwind block (when no handler is found)
 *                        3) method's exit block (when no handler is found and
 *                           unwind can't be done because the method's caller
 *                           is not a jitted method).
 *
 * @frame: frame pointer of method throwing exception
 * @native_ptr: pointer to instruction that caused exception
 * @exception: exception object to throw.
 */
unsigned char *throw_exception_from(struct compilation_unit *cu,
				    struct jit_stack_frame *frame,
				    unsigned char *native_ptr,
				    struct vm_object *exception)
{
	struct vm_thread *thread = vm_current_thread();
	unsigned char *eh_ptr = NULL;
	unsigned long bc_offset;

	if (thread->exception != NULL) {
		/* Looks like we've caught some asynchronous exception,
		   which must have precedence. */
		exception = thread->exception;
		thread->exception = NULL;
	}

	bc_offset = native_ptr_to_bytecode_offset(cu, native_ptr);
	if (bc_offset != BC_OFFSET_UNKNOWN) {
		eh_ptr = find_handler(cu, exception->class, bc_offset);
		if (eh_ptr != NULL)
			return eh_ptr;
	}

	if (!is_jit_method(frame->return_address)) {
		/* No handler found within jitted method call
		   chain. Set exception in execution environment and
		   return to previous (not jit) method. */
		thread->exception = exception;
		return bb_native_ptr(cu->exit_bb);
	}

	return bb_native_ptr(cu->unwind_bb);
}

int insert_exception_spill_insns(struct compilation_unit *cu)
{
	struct insn *insn;
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		if (bb->is_eh) {
			insn = exception_spill_insn(cu->exception_spill_slot);
			if (insn == NULL)
				return -ENOMEM;

			insn->bytecode_offset = bb->start;
			bb_add_insn(bb, insn);
		}
	}

	return 0;
}
