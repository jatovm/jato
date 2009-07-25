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

#include "vm/call.h"
#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/guard-page.h"
#include "vm/jni.h"
#include "vm/object.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "vm/object.h"
#include "vm/preload.h"
#include "vm/stack-trace.h"
#include "vm/system.h"

#include "jit/bc-offset-mapping.h"
#include "jit/cu-mapping.h"
#include "jit/exception.h"
#include "jit/compiler.h"

#include <malloc.h>
#include <stdio.h>

void *vm_native_stack_offset_guard;
void *vm_native_stack_badoffset;
void *jni_stack_offset_guard;
void *jni_stack_badoffset;

__thread struct jni_stack_entry jni_stack[JNI_STACK_SIZE];
__thread unsigned long jni_stack_offset;
__thread struct vm_native_stack_entry vm_native_stack[VM_NATIVE_STACK_SIZE];
__thread unsigned long vm_native_stack_offset;

__thread struct native_stack_frame *bottom_stack_frame;

void init_stack_trace_printing(void)
{
	vm_native_stack_offset = 0;
	jni_stack_offset = 0;

	/* Initialize JNI and VM native stacks' offset guards */
	unsigned long valid_size;

	valid_size = VM_NATIVE_STACK_SIZE *
		sizeof(struct vm_native_stack_entry);
	vm_native_stack_offset_guard = alloc_offset_guard(valid_size, 1);
	vm_native_stack_badoffset =
		valid_size + vm_native_stack_offset_guard;

	valid_size = JNI_STACK_SIZE * sizeof(struct jni_stack_entry);
	jni_stack_offset_guard = alloc_offset_guard(valid_size, 1);
	jni_stack_badoffset = valid_size + jni_stack_offset_guard;
}

static bool jni_stack_is_full(void)
{
	return jni_stack_index() == JNI_STACK_SIZE;
}

static bool vm_native_stack_is_full(void)
{
	return vm_native_stack_index() == VM_NATIVE_STACK_SIZE;
}

static inline struct jni_stack_entry *new_jni_stack_entry(void)
{
	struct jni_stack_entry *tr = (void*)jni_stack + jni_stack_offset;

	jni_stack_offset += sizeof(struct jni_stack_entry);
	return tr;
}

static inline struct vm_native_stack_entry *new_vm_native_stack_entry(void)
{
	struct vm_native_stack_entry *tr = (void*)vm_native_stack +
		vm_native_stack_offset;

	vm_native_stack_offset += sizeof(struct vm_native_stack_entry);
	return tr;
}

int vm_enter_jni(void *caller_frame, unsigned long call_site_addr,
		 struct vm_method *method)
{
	if (jni_stack_is_full()) {
		struct vm_object *e = vm_alloc_stack_overflow_error();
		if (!e)
			error("failed to allocate exception");

		signal_exception(e);
		return -1;
	}

	struct jni_stack_entry *tr = new_jni_stack_entry();

	tr->caller_frame = caller_frame;
	tr->call_site_addr = call_site_addr;
	tr->method = method;
	return 0;
}

int vm_enter_vm_native(void *target, void *stack_ptr)
{
	if (vm_native_stack_is_full()) {
		struct vm_object *e = vm_alloc_stack_overflow_error();
		if (!e)
			error("failed to allocate exception");

		signal_exception(e);
		return -1;
	}

	struct vm_native_stack_entry *tr = new_vm_native_stack_entry();

	tr->stack_ptr = stack_ptr;
	tr->target = target;
	return 0;
}

void vm_leave_jni()
{
	jni_stack_offset -= sizeof(struct jni_stack_entry);
}

void vm_leave_vm_native()
{
	vm_native_stack_offset -= sizeof(struct vm_native_stack_entry);
}

/**
 * get_caller_stack_trace_elem - sets @elem to the previous element.
 *
 * Returns 0 on success and -1 when bottom of stack is reached.
 */
static int get_caller_stack_trace_elem(struct stack_trace_elem *elem)
{
	unsigned long new_addr;
	unsigned long ret_addr;
	void *new_frame;

	/* If previous element was a JNI call then we move to the JNI
	 * caller's frame. We use the JNI stack_entry info to get the
	 * frame because we don't trust JNI methods's frame
	 * pointers. */
	if (elem->type == STACK_TRACE_ELEM_TYPE_JNI) {
		struct jni_stack_entry *tr =
			&jni_stack[elem->jni_stack_index--];

		new_frame = tr->caller_frame;
		new_addr = tr->call_site_addr;
		goto out;
	}

	/* Check if we hit the JNI interface frame */
	if (elem->jni_stack_index >= 0) {
		struct jni_stack_entry *tr =
			&jni_stack[elem->jni_stack_index];

		if (tr->vm_frame == elem->frame) {
			elem->type = STACK_TRACE_ELEM_TYPE_JNI;
			elem->is_native = false;

			elem->cu = tr->method->compilation_unit;
			elem->frame = NULL;
			return 0;
		}
	}

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

 out:
	/* Check if we hit the VM native caller frame */
	if (elem->vm_native_stack_index >= 0) {
		struct vm_native_stack_entry *tr =
			&vm_native_stack[elem->vm_native_stack_index];

		if (tr->stack_ptr - sizeof(struct native_stack_frame)
		    == new_frame)
		{
			elem->type = STACK_TRACE_ELEM_TYPE_VM_NATIVE;
			elem->is_native = true;
			new_addr = (unsigned long) tr->target;
			--elem->vm_native_stack_index;

			goto out2;
		}
	}

	/* Check if previous elemement was called from JIT trampoline. */
	if (elem->is_native && called_from_jit_trampoline(elem->frame)) {
		elem->type = STACK_TRACE_ELEM_TYPE_TRAMPOLINE;
		elem->is_native = true;
		goto out2;
	}

	elem->is_native = is_native(new_addr);

	if (elem->is_native)
		elem->type = STACK_TRACE_ELEM_TYPE_OTHER;
	else
		elem->type = STACK_TRACE_ELEM_TYPE_JIT;

 out2:
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
	while (get_caller_stack_trace_elem(elem) == 0) {
		if (elem->type < STACK_TRACE_ELEM_TYPE_OTHER)
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
	elem->type = STACK_TRACE_ELEM_TYPE_OTHER;
	elem->addr = (unsigned long)&init_stack_trace_elem;
	elem->frame = __builtin_frame_address(0);

	elem->vm_native_stack_index = vm_native_stack_index() - 1;
	elem->jni_stack_index = jni_stack_index() - 1;

	return get_prev_stack_trace_elem(elem);
}

struct compilation_unit *stack_trace_elem_get_cu(struct stack_trace_elem *elem)
{
	if (elem->type == STACK_TRACE_ELEM_TYPE_JNI)
		return elem->cu;

	return jit_lookup_cu(elem->addr);
}

/**
 * skip_frames_from_class - makes @elem to point to the nearest stack
 *     trace element which does not belong to any method of class
 *     which is an instance of @class.  Note that this also skips all
 *     classes that extends @class.
 *
 * Returns 0 on success and -1 when bottom of stack trace reached.
 */
int skip_frames_from_class(struct stack_trace_elem *elem,
			   struct vm_class *class)
{
	struct compilation_unit *cu;

	do {
		cu = stack_trace_elem_get_cu(elem);
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
 * get_intermediate_stack_trace - returns an array with intermediate
 *   java stack trace. Each stack trace element is described by two
 *   consequtive elements: a pointer to struct vm_method and bytecode
 *   offset.
 */
static struct vm_object *get_intermediate_stack_trace(void)
{
	struct stack_trace_elem st_elem;
	struct compilation_unit *cu;
	struct vm_object *array;
	int array_type;
	int depth;
	int i;

	if (init_stack_trace_elem(&st_elem))
		return NULL;

	if (skip_frames_from_class(&st_elem, vm_java_lang_VMThrowable))
		return NULL;

	if (skip_frames_from_class(&st_elem, vm_java_lang_Throwable))
		return NULL;

	depth = get_stack_trace_depth(&st_elem);
	if (depth == 0)
		return NULL;

	array_type = sizeof(unsigned long) == 4 ? T_INT : T_LONG;
	array = vm_object_alloc_native_array(array_type, depth * 2);
	if (!array)
		return NULL;

	i = 0;
	do {
		unsigned long bc_offset;

		cu = stack_trace_elem_get_cu(&st_elem);
		if (!cu) {
			fprintf(stderr,
				"%s: no compilation unit mapping for %p\n",
				__func__, (void*)st_elem.addr);
			return NULL;
		}

		if (vm_method_is_native(cu->method))
			bc_offset = BC_OFFSET_UNKNOWN;
		else
			bc_offset = native_ptr_to_bytecode_offset(cu,
						(unsigned char*)st_elem.addr);

		array_set_field_ptr(array, i++, cu->method);
		array_set_field_ptr(array, i++, (void*)bc_offset);
	} while (get_prev_stack_trace_elem(&st_elem) == 0);

	return array;
}

/**
 * new_stack_trace_element - creates new instance of
 *     java.lang.StackTraceElement for given method and bytecode
 *     offset.
 */
static struct vm_object *
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

	if (!is_native && cb->source_file_name)
		file_name = vm_object_alloc_string_from_c(cb->source_file_name);
	else
		file_name = NULL;

	class_dot_name = slash2dots(cb->name);
	class_name = vm_object_alloc_string_from_c(class_dot_name);
	method_name = vm_object_alloc_string_from_c(mb->name);
	free(class_dot_name);

	ste = vm_object_alloc(vm_java_lang_StackTraceElement);
	if(!ste)
		return NULL;

	vm_call_method(vm_java_lang_StackTraceElement_init, ste, file_name,
		       line_no, class_name, method_name, is_native);

	return ste;
}

/**
 * convert_intermediate_stack_trace - returns
 *     java.lang.StackTraceElement[] array filled in using data from
 *     given intermediate stack trace array.
 *
 * @array: intermediate stack trace array. Should have the same format
 *     as the one returned by get_intermediate_stack_trace().
 */
static struct vm_object *
convert_intermediate_stack_trace(struct vm_object *array)
{
	struct vm_object *ste_array;
	int depth;
	int i;
	int j;

	depth = array->array_length;

	ste_array = vm_object_alloc_array(
		vm_array_of_java_lang_StackTraceElement, depth / 2);
	if (!ste_array)
		return NULL;

	for(i = 0, j = 0; i < depth; j++) {
		struct vm_method *mb;
		unsigned long bc_offset;
		struct vm_object *ste;

		mb = array_get_field_ptr(array, i++);
		bc_offset = (unsigned long)array_get_field_ptr(array, i++);

		ste = new_stack_trace_element(mb, bc_offset);
		if(ste == NULL || exception_occurred())
			return NULL;

		array_set_field_ptr(ste_array, j, ste);
	}

	return ste_array;
}

/**
 * get_stack_trace - returns an array of java.lang.StackTraceElement
 *   representing the current java call stack.
 */
struct vm_object *get_stack_trace()
{
	struct vm_object *intermediate;

	intermediate = get_intermediate_stack_trace();
	if (!intermediate)
		return NULL;

	return convert_intermediate_stack_trace(intermediate);
}

struct vm_object *
native_vmthrowable_fill_in_stack_trace(struct vm_object *throwable)
{
	struct vm_object *vmstate;
	struct vm_object *array;

	vmstate = vm_object_alloc(vm_java_lang_VMThrowable);
	if (!vmstate)
		return NULL;

	array = get_intermediate_stack_trace();
	if (!array)
		return NULL;

	field_set_object(vmstate, vm_java_lang_VMThrowable_vmdata, array);

	return vmstate;
}

struct vm_object *
native_vmthrowable_get_stack_trace(struct vm_object *this,
				   struct vm_object *throwable)
{
	struct vm_object *result;
	struct vm_object *array;

	array = field_get_object(this, vm_java_lang_VMThrowable_vmdata);
	if (!array)
		return NULL;

	result = convert_intermediate_stack_trace(array);

	if (exception_occurred())
		throw_from_native(sizeof(struct vm_object *) * 2);

	return result;
}

static void
vm_throwable_to_string(struct vm_object *this, struct string *str)
{
	struct vm_object *string_obj;

	string_obj = vm_call_method_object(vm_java_lang_Throwable_toString,
					   this);
	if (!string_obj)
		return;

	char *cstr = vm_string_to_cstr(string_obj);
	str_append(str, cstr);
	free(cstr);
}

static void vm_stack_trace_element_to_string(struct vm_object *elem,
					     struct string *str)
{
	struct vm_object *method_name;
	struct vm_object *file_name;
	struct vm_object *class_name;
	char *method_name_str;
	char *file_name_str;
	char *class_name_str;
	jint line_number;
	jboolean is_native;

	file_name = vm_call_method_object(
			vm_java_lang_StackTraceElement_getFileName, elem);
	line_number = vm_call_method_jint(
			vm_java_lang_StackTraceElement_getLineNumber, elem);
	class_name = vm_call_method_object(
			vm_java_lang_StackTraceElement_getClassName, elem);
	method_name = vm_call_method_object(
			vm_java_lang_StackTraceElement_getMethodName, elem);
	is_native = vm_call_method_jboolean(
			vm_java_lang_StackTraceElement_isNativeMethod, elem);

	if (class_name) {
		class_name_str = vm_string_to_cstr(class_name);

		str_append(str, class_name_str);
		if (method_name)
			str_append(str, ".");
	}

	if (method_name) {
		method_name_str = vm_string_to_cstr(method_name);
		str_append(str, method_name_str);
	}

	str_append(str, "(");

	if (file_name) {
		file_name_str = vm_string_to_cstr(file_name);
		str_append(str, file_name_str);
	} else {
		if (is_native)
			str_append(str, "Native Method");
		else
			str_append(str, "Unknown Source");
	}

	if (line_number >= 0)
		str_append(str, ":%d", line_number);

	str_append(str, ")");
}

static void
vm_throwable_stack_trace(struct vm_object *this, struct string *str,
			 struct vm_object *stack, int equal)
{
	vm_throwable_to_string(this, str);
	str_append(str, "\n");

	if (stack == NULL || stack->array_length == 0) {
		str_append(str, "   <<No stacktrace available>>\n");
		return;
	}

	for (int i = 0; i < stack->array_length - equal; i++) {
		struct vm_object *elem = array_get_field_ptr(stack, i);

		str_append(str, "   at ");
	        if (!elem)
			str_append(str, "<<Unknown>>");
		else
			vm_stack_trace_element_to_string(elem, str);

		str_append(str, "\n");
	}

	if (equal > 0)
		str_append(str, "   ...%d more\n", equal);
}


static void
vm_throwable_print_stack_trace(struct vm_object *this, struct string *str)
{
	struct vm_object *stack;
	struct vm_object *cause;

	stack = vm_call_method_object(
				vm_java_lang_Throwable_getStackTrace, this);

	vm_throwable_stack_trace(this, str, stack, 0);

	cause = this;
	while ((cause = vm_call_method_object(vm_java_lang_Throwable_getCause,
					      cause)))
	{
		struct vm_object *p_stack;

		str_append(str, "Caused by: ");

		p_stack = stack;
		stack = vm_call_method_object(
				vm_java_lang_Throwable_getStackTrace, cause);

		if (p_stack == NULL || p_stack->array_length == 0) {
			vm_throwable_stack_trace(cause, str, stack, 0);
			continue;
		}

		int equal = 0;
		int frame = stack->array_length - 1;
		int p_frame = p_stack->array_length - 1;

		while (frame > 0 && p_frame > 0) {
			if (!vm_call_method_jboolean(
				vm_java_lang_StackTraceElement_equals,
				array_get_field_ptr(stack, frame),
				array_get_field_ptr(p_stack, p_frame)))
				break;

			equal++;
			frame--;
			p_frame--;
		}

		vm_throwable_stack_trace(cause, str, stack, equal);
	}
}

static void vm_print_exception_description(struct vm_object *exception)
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

	fprintf(stderr, "\n    <<No stacktrace available>>\n");
}

void vm_print_exception(struct vm_object *exception)
{
	struct vm_object *old_exception;
	struct string *str;

	str = alloc_str();
	if (!str)
		goto error;

	old_exception = exception_occurred();
	clear_exception();

	/* TODO: print correct thread name */
	str_append(str, "Exception in thread \"main\" ");

	vm_throwable_print_stack_trace(exception, str);
	if (exception_occurred())
		goto error;

	fprintf(stderr, "%s", str->value);
	free_str(str);

	if (old_exception)
		signal_exception(old_exception);

	return;
error:
	warn("unable to print exception");
	if (str)
		free_str(str);

	vm_print_exception_description(exception);
}

/**
 * Creates an instance of StackOverflowError. We create exception
 * object and fill stack trace in manually because throwable
 * constructor calls fillInStackTrace which can cause StackOverflowError
 * when VM native stack is full.
 */
struct vm_object *vm_alloc_stack_overflow_error(void)

{	struct vm_object *stacktrace;
	struct vm_object *obj;

	obj = vm_object_alloc(vm_java_lang_StackOverflowError);
	if (!obj) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	stacktrace = get_stack_trace();
	if (stacktrace)
		vm_call_method(vm_java_lang_Throwable_setStackTrace, obj,
			       stacktrace);

	return obj;
}
