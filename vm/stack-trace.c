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

#include <vm/class.h>
#include <vm/classloader.h>
#include <vm/object.h>
#include <vm/method.h>
#include <vm/natives.h>
#include <vm/object.h>
#include <vm/stack-trace.h>
#include <vm/java_lang.h>
#include <vm/system.h>

#include <jit/bc-offset-mapping.h>
#include <jit/cu-mapping.h>
#include <jit/exception.h>
#include <jit/compiler.h>

#include <malloc.h>
#include <stdio.h>

__thread struct native_stack_frame *bottom_stack_frame;

static struct vm_class *vmthrowable_class;
static struct vm_class *throwable_class;
static struct vm_class *ste_class;
static struct vm_class *ste_array_class;
static int backtrace_offset;
static int vmstate_offset;

typedef void (*ste_init_fn)(struct vm_object *, struct vm_object *, int,
			    struct vm_object *, struct vm_object *, int);

static ste_init_fn ste_init;

void init_stack_trace_printing(void)
{
	struct vm_field *backtrace_fld;
	struct vm_field *vmstate_fld;
	struct vm_method *ste_init_mb;

	NOT_IMPLEMENTED;
	return;

	/* XXX: Preload these. */
	vmthrowable_class = classloader_load("java/lang/VMThrowable");
	throwable_class = classloader_load("java/lang/Throwable");
	ste_class = classloader_load("java/lang/StackTraceElement");
	ste_array_class = classloader_load("[Ljava/lang/StackTraceElement;");

	if (exception_occurred())
		goto error;

	ste_init_mb = vm_class_get_method_recursive(ste_class, "<init>",
		"(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Z)V");
	if (ste_init_mb == NULL)
		goto error;

	ste_init = vm_method_trampoline_ptr(ste_init_mb);

	backtrace_fld = vm_class_get_field_recursive(vmthrowable_class,
		"backtrace", "Ljava/lang/Object;");
	vmstate_fld = vm_class_get_field_recursive(throwable_class,
		"vmState", "Ljava/lang/VMThrowable;");

	if (!backtrace_fld || !vmstate_fld)
		goto error;

	backtrace_offset = backtrace_fld->offset;
	vmstate_offset = vmstate_fld->offset;
	return;

 error:
	die("%s: initialization failed.", __func__);
}

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
int skip_frames_from_class(struct stack_trace_elem *elem, struct vm_class *class)
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

		if (!vm_class_is_assignable_from(class, cu->method->class))
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

/**
 * get_stack_trace - creates instance of VMThrowable with
 *                   backtrace filled in.
 *
 * @st_elem: top most stack trace element.
 */
struct vm_object *get_stack_trace(struct stack_trace_elem *st_elem)
{
	struct compilation_unit *cu;
	struct vm_object *vmstate;
	struct vm_object *array;
	unsigned long *data;
	int array_type;
	int depth;
	int i;

	depth = get_stack_trace_depth(st_elem);
	if (depth == 0)
		return NULL;

	vmstate = vm_object_alloc(vmthrowable_class);
	if (!vmstate)
		return NULL;

	array_type = sizeof(unsigned long) == 4 ? T_INT : T_LONG;
	array = vm_object_alloc_native_array(array_type, depth * 2);
	if (!array)
		return NULL;

	data = ARRAY_DATA(array);

	i = 0;
	do {
		unsigned long bc_offset;

		cu = get_cu_from_native_addr(st_elem->addr);
		if (!cu) {
			fprintf(stderr,
				"%s: no compilation unit mapping for %p\n",
				__func__, (void*)st_elem->addr);
			return NULL;
		}

		if (vm_method_is_native(cu->method))
			bc_offset = BC_OFFSET_UNKNOWN;
		else
			bc_offset = native_ptr_to_bytecode_offset(cu,
				(unsigned char*)st_elem->addr);

		data[i++] = (unsigned long)cu->method;

		/* We must add cu->method->code to be compatible with JamVM. */
		data[i++] = bc_offset + (unsigned long)cu->method->jit_code;
	} while (get_prev_stack_trace_elem(st_elem) == 0);

	INST_DATA(vmstate)[backtrace_offset] = (uintptr_t)array;

	return vmstate;
}

/**
 * new_stack_trace_element - creates new instance of
 *     java.lang.StackTraceElement for given method and bytecode
 *     offset.
 */
struct vm_object *
new_stack_trace_element(struct vm_method *mb, unsigned long bc_offset)
{
	struct vm_object *method_name;
	struct vm_object *class_name;
	struct vm_object *file_name;
	struct vm_class *cb;
	struct vm_object *ste;
	char *class_dot_name;
	bool is_native;
	int line_no;

	cb = mb->class;

	line_no = bytecode_offset_to_line_no(mb, bc_offset);
	is_native = vm_method_is_native(mb);

	NOT_IMPLEMENTED;
#if 0
	if (!is_native && cb->source_file_name)
		file_name = createString(cb->source_file_name);
	else
#endif
		file_name = NULL;

	class_dot_name = slash2dots(cb->name);
	class_name = vm_object_alloc_string_from_c(class_dot_name);
	method_name = vm_object_alloc_string_from_c(mb->name);
	free(class_dot_name);

	ste = vm_object_alloc(ste_class);
	if(!ste)
		return NULL;

	ste_init(ste, file_name, line_no, class_name, method_name, is_native);

	return ste;
}

/**
 * convert_stack_trace - returns java.lang.StackTraceElement[] array
 *     filled in using stack trace stored in given VMThrowable
 *     instance.
 *
 * @vmthrowable: instance of VMThrowable to get stack trace from.
 */
struct vm_object *convert_stack_trace(struct vm_object *vmthrowable)
{
	unsigned long *stack_trace;
	struct vm_object *ste_array;
	struct vm_object *array;
	struct vm_object **dest;
	int depth;
	int i;

	array = (struct vm_object *)INST_DATA(vmthrowable)[backtrace_offset];
	if (!array)
		return NULL;

	stack_trace = ARRAY_DATA(array);
	depth = ARRAY_LEN(array);

	ste_array = vm_object_alloc_array(ste_array_class, depth / 2);
	if (!ste_array)
		return NULL;

	dest = ARRAY_DATA(ste_array);

	for(i = 0; i < depth; dest++) {
		struct vm_method *mb;
		unsigned long bc_offset;
		struct vm_object *ste;

		mb = (struct vm_method *)stack_trace[i++];
		bc_offset = stack_trace[i++] - (unsigned long)mb->jit_code;

		ste = new_stack_trace_element(mb, bc_offset);
		if(ste == NULL || exception_occurred())
			return NULL;

		*dest = ste;
	}

	return ste_array;
}

struct vm_object * __vm_native
vm_throwable_fill_in_stack_trace(struct vm_object *throwable)
{
	struct stack_trace_elem st_elem;

	if (init_stack_trace_elem(&st_elem))
		return NULL;

	if (skip_frames_from_class(&st_elem, vmthrowable_class))
		return NULL;

	if (skip_frames_from_class(&st_elem, throwable_class))
		return NULL;

	return get_stack_trace(&st_elem);
}

struct vm_object * __vm_native
vm_throwable_get_stack_trace(struct vm_object *this, struct vm_object *throwable)
{
	struct vm_object *result;

	result = convert_stack_trace(this);

	if (exception_occurred())
		throw_from_native(sizeof(struct vm_object *) * 2);

	return result;
}

void set_throwable_vmstate(struct vm_object *throwable, struct vm_object *vmstate)
{
	INST_DATA(throwable)[vmstate_offset] = (uintptr_t)vmstate;
}

void vm_print_exception(struct vm_object *exception)
{
	struct vm_object *message_obj;

	fprintf(stderr, "%s", exception->class->name);

	message_obj = field_get_object(exception,
				       vm_java_lang_Throwable_detailMessage);
	if (message_obj) {
		char *msg_str;

		msg_str = vm_string_to_cstr(message_obj);

		fprintf(stderr, ": %s", msg_str);

		free(msg_str);
	}

	fprintf(stderr, "\n   <<No stack trace available>>\n");

	NOT_IMPLEMENTED;
}
