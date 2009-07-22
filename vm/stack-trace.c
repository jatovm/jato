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

#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/object.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "vm/object.h"
#include "vm/stack-trace.h"
#include "vm/preload.h"
#include "vm/system.h"

#include "jit/bc-offset-mapping.h"
#include "jit/cu-mapping.h"
#include "jit/exception.h"
#include "jit/compiler.h"

#include <malloc.h>
#include <stdio.h>

__thread struct native_stack_frame *bottom_stack_frame;

typedef void (*ste_init_fn)(struct vm_object *, struct vm_object *, int,
			    struct vm_object *, struct vm_object *, int);
typedef struct vm_object *(*throwable_tostring_fn)(struct vm_object *);
typedef struct vm_object *(*throwable_stacktracestring_fn)(struct vm_object *);

static ste_init_fn ste_init;
static throwable_tostring_fn throwable_tostring;
static throwable_stacktracestring_fn throwable_stacktracestring;

void init_stack_trace_printing(void)
{
	struct vm_method *ste_init_mb;
	struct vm_method *throwable_tostring_mb;
	struct vm_method *throwable_stacktracestring_mb;

	ste_init_mb = vm_class_get_method_recursive(
		vm_java_lang_StackTraceElement,
		"<init>",
		"(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Z)V");

	throwable_tostring_mb =
		vm_class_get_method_recursive(vm_java_lang_Throwable,
			"toString", "()Ljava/lang/String;");

	throwable_stacktracestring_mb =
		vm_class_get_method_recursive(vm_java_lang_Throwable,
			"stackTraceString", "()Ljava/lang/String;");

	ste_init = vm_method_call_ptr(ste_init_mb);
	throwable_tostring = vm_method_call_ptr(throwable_tostring_mb);
	throwable_stacktracestring =
		vm_method_call_ptr(throwable_stacktracestring_mb);

	if (!ste_init_mb ||
	    !throwable_tostring ||
	    !throwable_stacktracestring)
		error("initialization failed");
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
int skip_frames_from_class(struct stack_trace_elem *elem,
			   struct vm_class *class)
{
	struct compilation_unit *cu;

	do {
		cu = jit_lookup_cu(elem->addr);
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

		cu = jit_lookup_cu(st_elem.addr);
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

	ste_init(ste, file_name, line_no, class_name, method_name, is_native);

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

struct vm_object * __vm_native
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

struct vm_object * __vm_native
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

typedef struct vm_object *(*vm_java_lang_Throwable_toString_fn)
	(struct vm_object *);
typedef struct vm_object *(*vm_java_lang_Throwable_getCause_fn)
	(struct vm_object *);
typedef struct vm_object *(*vm_java_lang_Throwable_getStackTrace_fn)
	(struct vm_object *);
typedef struct vm_object *(*vm_java_lang_StackTraceElement_getFileName_fn)
	(struct vm_object *);
typedef struct vm_object *(*vm_java_lang_StackTraceElement_getClassName_fn)
	(struct vm_object *);
typedef struct vm_object *(*vm_java_lang_StackTraceElement_getMethodName_fn)
	(struct vm_object *);
typedef int (*vm_java_lang_StackTraceElement_getLineNumber_fn)
	(struct vm_object *);
typedef bool (*vm_java_lang_StackTraceElement_isNativeMethod_fn)
	(struct vm_object *);
typedef bool (*vm_java_lang_StackTraceElement_equals_fn)
	(struct vm_object *, struct vm_object *);

static void
vm_throwable_to_string(struct vm_object *this, struct string *str)
{
	vm_java_lang_Throwable_toString_fn toString =
		vm_method_call_ptr(vm_java_lang_Throwable_toString);
	struct vm_object *string_obj;

	string_obj = toString(this);
	if (!string_obj)
		return;

	char *cstr = vm_string_to_cstr(string_obj);
	str_append(str, cstr);
	free(cstr);
}

static void vm_stack_trace_element_to_string(struct vm_object *elem,
					     struct string *str)
{
	vm_java_lang_StackTraceElement_getFileName_fn getFileName;
	vm_java_lang_StackTraceElement_getClassName_fn getClassName;
	vm_java_lang_StackTraceElement_getMethodName_fn getMethodName;
	vm_java_lang_StackTraceElement_getLineNumber_fn getLineNumber;
	vm_java_lang_StackTraceElement_isNativeMethod_fn isNativeMethod;
	struct vm_object *method_name;
	struct vm_object *file_name;
	struct vm_object *class_name;
	char *method_name_str;
	char *file_name_str;
	char *class_name_str;
	int line_number;
	bool is_native;

	getMethodName = vm_method_call_ptr(
		vm_java_lang_StackTraceElement_getMethodName);
	getFileName = vm_method_call_ptr(
		vm_java_lang_StackTraceElement_getFileName);
	getClassName = vm_method_call_ptr(
		vm_java_lang_StackTraceElement_getClassName);
	getLineNumber = vm_method_call_ptr(
		vm_java_lang_StackTraceElement_getLineNumber);
	isNativeMethod = vm_method_call_ptr(
		vm_java_lang_StackTraceElement_isNativeMethod);

	file_name = getFileName(elem);
	line_number = getLineNumber(elem);
	class_name = getClassName(elem);
	method_name = getMethodName(elem);
	is_native = isNativeMethod(elem);

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
	vm_java_lang_Throwable_getCause_fn getCause;
	vm_java_lang_Throwable_getStackTrace_fn getStackTrace;
	vm_java_lang_StackTraceElement_equals_fn ste_equals;
	struct vm_object *stack;

	getCause =
		vm_method_call_ptr(vm_java_lang_Throwable_getCause);
	getStackTrace =
		vm_method_call_ptr(vm_java_lang_Throwable_getStackTrace);
	ste_equals =
		vm_method_call_ptr(vm_java_lang_StackTraceElement_equals);

	stack = getStackTrace(this);
	vm_throwable_stack_trace(this, str, stack, 0);

	struct vm_object *cause = this;
	while ((cause = getCause(cause))) {
		struct vm_object *p_stack;

		str_append(str, "Caused by: ");

		p_stack = stack;
		stack = getStackTrace(cause);

		if (p_stack == NULL || p_stack->array_length == 0) {
			vm_throwable_stack_trace(cause, str, stack, 0);
			continue;
		}

		int equal = 0;
		int frame = stack->array_length - 1;
		int p_frame = p_stack->array_length - 1;

		while (frame > 0 && p_frame > 0) {
			if (!ste_equals(array_get_field_ptr(stack, frame),
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
	struct string *str;

	str = alloc_str();
	if (!str)
		goto error;

	/* TODO: print correct thread name */
	str_append(str, "Exception in thread \"main\" ");

	vm_throwable_print_stack_trace(exception, str);
	if (exception_occurred())
		goto error;

	fprintf(stderr, "%s", str->value);
	free_str(str);

	return;
error:
	warn("unable to print exception");
	if (str)
		free_str(str);

	vm_print_exception_description(exception);
}
