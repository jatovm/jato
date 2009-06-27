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
#include <vm/die.h>
#include <vm/guard-page.h>
#include <vm/method.h>
#include <vm/object.h>
#include <vm/thread.h>

#include <arch/stack-frame.h>
#include <arch/instruction.h>
#include <errno.h>

__thread struct vm_object *exception_holder = NULL;
__thread void *exception_guard = NULL;
__thread void *trampoline_exception_guard = NULL;

void *exceptions_guard_page;
void *trampoline_exceptions_guard_page;

void init_exceptions(void)
{
	exceptions_guard_page = alloc_guard_page();
	trampoline_exceptions_guard_page = alloc_guard_page();

	if (!exceptions_guard_page || !trampoline_exceptions_guard_page)
		die("%s: failed to allocate exceptions guard page.", __func__);

	/* TODO: Should be called from thread initialization code. */
	thread_init_exceptions();
}

/**
 * thread_init_exceptions - initializes per-thread structures.
 */
void thread_init_exceptions(void)
{
	/* Assign safe pointers. */
	exception_guard = &exception_guard;
	trampoline_exception_guard = &trampoline_exception_guard;
}

/**
 * signal_exception - used for signaling that exception has occurred
 *         in jato functions. Exception will be thrown when controll
 *         is returned to JIT code.
 *
 * @exception: exception object to be thrown.
 */
void signal_exception(struct vm_object *exception)
{
	if (exception_holder)
		return;

	if (exception == NULL)
		die("%s: exception is NULL.", __func__);

	trampoline_exception_guard = trampoline_exceptions_guard_page;
	exception_guard  = exceptions_guard_page;
	exception_holder = exception;
}

void signal_new_exception(const char *class_name, const char *msg)
{
	struct vm_object *e;

	e = new_exception(class_name, msg);
	signal_exception(e);
}

void clear_exception(void)
{
	trampoline_exception_guard = &trampoline_exception_guard;
	exception_guard  = &exception_guard;
	exception_holder = NULL;
}

struct cafebabe_code_attribute_exception *
lookup_eh_entry(struct vm_method *method, unsigned long target)
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
	struct basic_block *bb;

	bb = find_bb(cu, eh->handler_pc);
	assert(bb != NULL);

	return bb_native_ptr(bb);
}

/**
 * find_handler - return native pointer to exception handler for given
 *                @exception_class and @bc_offset of source.
 */
static unsigned char *find_handler(struct compilation_unit *cu,
	struct vm_class *exception_class, unsigned long bc_offset)
{
	struct cafebabe_code_attribute_exception *eh;
	struct vm_method *method;
	int size;
	int i;

	method = cu->method;
	size = method->code_attribute.exception_table_length;

	for (i = 0; i < size; i++) {
		struct vm_class *catch_class;

		eh = &method->code_attribute.exception_table[i];
		if (!exception_covers(eh, bc_offset))
			continue;

		/* This matches to everything. */
		if (eh->catch_type == 0)
			break;

		catch_class = vm_class_resolve_class(method->class,
			eh->catch_type);

		if (vm_class_is_assignable_from(exception_class, catch_class))
			break;
	}

	if (i < size)
		return eh_native_ptr(cu, eh);

	return NULL;
}

static bool
is_inside_exit_unlock(struct compilation_unit *cu, unsigned char *ptr)
{
	return ptr >= bb_native_ptr(cu->exit_bb) &&
		ptr < cu->exit_past_unlock_ptr;
}

static bool
is_inside_unwind_unlock(struct compilation_unit *cu, unsigned char *ptr)
{
	return ptr >= bb_native_ptr(cu->unwind_bb) &&
		ptr < cu->unwind_past_unlock_ptr;
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
 * @cu: compilation unit
 * @frame: frame pointer of method throwing exception
 * @native_ptr: pointer to instruction that caused exception
 */
unsigned char *
throw_exception_from(struct compilation_unit *cu, struct jit_stack_frame *frame,
		     unsigned char *native_ptr)
{
	struct vm_object *exception;
	unsigned long bc_offset;
	unsigned char *eh_ptr;

	eh_ptr = NULL;

	exception = exception_occurred();
	assert(exception != NULL);

	clear_exception();

	bc_offset = native_ptr_to_bytecode_offset(cu, native_ptr);
	if (bc_offset != BC_OFFSET_UNKNOWN) {
		eh_ptr = find_handler(cu, exception->class, bc_offset);
		if (eh_ptr != NULL) {
			signal_exception(exception);
			return eh_ptr;
		}
	}

	signal_exception(exception);

	if (is_native(frame->return_address)) {
		/*
		 * No handler found within jitted method call chain.
		 * Return to previous (not jit) method.
		 */
		if (is_inside_exit_unlock(cu, native_ptr))
			return cu->exit_past_unlock_ptr;

		return bb_native_ptr(cu->exit_bb);
	}

	if (is_inside_unwind_unlock(cu, native_ptr))
		return cu->unwind_past_unlock_ptr;

	return bb_native_ptr(cu->unwind_bb);
}
