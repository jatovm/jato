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
#include <vm/stack-trace.h>
#include <vm/natives.h>

#include <jit/cu-mapping.h>
#include <jit/compiler.h>

__thread struct native_stack_frame *bottom_stack_frame;

/**
 * get_caller_stack_trace_elem - makes @elem to point to the stack
 *     trace element corresponding to the caller of given element.
 *
 * Returns 0 on success and -1 when bottom of stack trace reached.
 */
static int get_caller_stack_trace_elem(struct stack_trace_elem *elem)
{
	unsigned long new_addr;
	unsigned long ret_addr;
	void *new_frame;

	if (elem->is_native) {
		struct native_stack_frame *frame;

		frame = elem->frame;
		new_frame = frame->prev;
		ret_addr = frame->return_address;
	} else {
		struct jit_stack_frame *frame;

		frame = elem->frame;
		new_frame = frame->prev;
		ret_addr = frame->return_address;
	}

	/*
	 * We know only return addresses and we don't know the size of
	 * call instruction that was used. Therefore we don't know
	 * address of the call site beggining. We store address of the
	 * last byte of the call site instead which is enough
	 * information to obtain bytecode offset.
	 */
	new_addr = ret_addr - 1;

	if (new_frame == bottom_stack_frame)
		return -1;

	elem->is_trampoline = elem->is_native &&
		called_from_jit_trampoline(elem->frame);
	elem->is_native = is_native(new_addr) || elem->is_trampoline;
	elem->addr = new_addr;
	elem->frame = new_frame;

	return 0;
}

/**
 * get_prev_stack_trace_elem - makes @elem to point to the previous
 *     stack_trace element that has a compilation unit (JIT method or
 *     VM native).
 *
 * Returns 0 on success and -1 when bottom of stack trace reached.
 */
int get_prev_stack_trace_elem(struct stack_trace_elem *elem)
{
	while (get_caller_stack_trace_elem(elem) == 0)  {
		if (is_vm_native(elem->addr) ||
		    !(elem->is_trampoline || elem->is_native))
			return 0;
	}

	return -1;
}

/**
 * init_stack_trace_elem - initializes stack trace element so that it
 *     points to the nearest JIT or VM native frame.
 *
 * Returns 0 on success and -1 when bottom of stack trace reached.
 */
int init_stack_trace_elem(struct stack_trace_elem *elem)
{
	elem->is_native = true;
	elem->is_trampoline = false;
	elem->addr = (unsigned long)&init_stack_trace_elem;
	elem->frame = __builtin_frame_address(0);

	return get_prev_stack_trace_elem(elem);
}

/**
 * skip_frames_from_class - makes @elem to point to the nearest stack
 *     trace element which does not belong to any method of class
 *     which is an instance of @class.  Note that this also skips all
 *     classes that extends @class.
 *
 * Returns 0 on success and -1 when bottom of stack trace reached.
 */
int skip_frames_from_class(struct stack_trace_elem *elem, struct object *class)
{
	struct compilation_unit *cu;

	do {
		cu = get_cu_from_native_addr(elem->addr);
		if (cu == NULL) {
			fprintf(stderr,
				"%s: no compilation unit mapping for %p\n",
				__func__, (void*)elem->addr);
			return -1;
		}

		if (!isInstanceOf(class, cu->method->class))
			return 0;

	} while (get_prev_stack_trace_elem(elem) == 0);

	return -1;
}

/**
 * get_stack_trace_depth - returns number of stack trace elements,
 *     including @elem.
 */
int get_stack_trace_depth(struct stack_trace_elem *elem)
{
	struct stack_trace_elem tmp;
	int depth;

	tmp = *elem;
	depth = 1;

	while (get_prev_stack_trace_elem(&tmp) == 0)
		depth++;

	return depth;
}
