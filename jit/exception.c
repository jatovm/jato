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

#include <jit/compilation-unit.h>
#include <jit/bc-offset-mapping.h>
#include <jit/basic-block.h>
#include <jit/exception.h>
#include <jit/compiler.h>

#include <vm/guard-page.h>
#include <vm/buffer.h>
#include <vm/class.h>
#include <vm/die.h>

#include <arch/stack-frame.h>
#include <arch/instruction.h>
#include <errno.h>

__thread void *exception_guard = NULL;
void *exceptions_guard_page;

void init_exceptions(void)
{
	exceptions_guard_page = alloc_guard_page();
	if (!exceptions_guard_page)
		die("%s: failed to allocate exceptions guard page.", __func__);

	/* TODO: Should be called from thread initialization code. */
	thread_init_exceptions();
}

/**
 * thread_init_exceptions - initializes per-thread structures.
 */
void thread_init_exceptions(void)
{
	exception_guard = &exception_guard; /* assign a safe pointer */
}

/**
 * signal_exception - used for signaling that exception has occurred
 *         in jato functions. Exception will be thrown as soon as
 *         controll is returned back to JIT method code. If another
 *         exception is already signalled no action is done.
 *
 * @exception: exception object to be thrown.
 */
void signal_exception(struct object *exception)
{
	if (getExecEnv()->exception)
		return;

	if (exception == NULL)
		die("%s: exception is NULL.", __func__);

	getExecEnv()->exception = exception;

	exception_guard = exceptions_guard_page;
}

void signal_new_exception(char *class_name, char *msg)
{
	struct object *e;

	e = new_exception(class_name, msg);
	signal_exception(e);
}

struct object *exception_occurred(void)
{
	return getExecEnv()->exception;
}

void clear_exception(void)
{
	exception_guard = &exception_guard;
	getExecEnv()->exception = NULL;
}

struct exception_table_entry *exception_find_entry(struct methodblock *method,
						   unsigned long target)
{
	int i;

	for (i = 0; i < method->exception_table_size; i++) {
		struct exception_table_entry *eh = &method->exception_table[i];

		if (eh->handler_pc == target)
			return eh;
	}

	return NULL;
}

static unsigned char *eh_native_ptr(struct compilation_unit *cu,
				    struct exception_table_entry *eh)
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
				   Class *exception_class,
				   unsigned long bc_offset)
{
	struct exception_table_entry *eh;
	struct methodblock *method = cu->method;
	int size = method->exception_table_size;
	int i;

	for (i = 0; i < size; i++) {
		eh = &method->exception_table[i];

		if (exception_covers(eh, bc_offset)) {
			Class *catch_class;

			if (eh->catch_type == 0)
				break; /* It's a finally block */

			catch_class = resolveClass(method->class,
						   eh->catch_type,
						   false);

			if (isInstanceOf(catch_class, exception_class))
				break;
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
				    struct object *exception)
{
	struct object *ee_exception;
	unsigned long bc_offset;
	unsigned char *eh_ptr;

	eh_ptr = NULL;

	ee_exception = exception_occurred();
	if (ee_exception) {
		exception = ee_exception;
		clear_exception();
	}

	bc_offset = native_ptr_to_bytecode_offset(cu, native_ptr);
	if (bc_offset != BC_OFFSET_UNKNOWN) {
		eh_ptr = find_handler(cu, exception->class, bc_offset);
		if (eh_ptr != NULL)
			return eh_ptr;
	}

	if (!is_jit_method(frame->return_address)) {
		/*
		 * No handler found within jitted method call chain.
		 * Signal exception and return to previous (not jit) method.
		 */
		signal_exception(exception);
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
