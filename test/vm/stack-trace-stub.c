#include <execinfo.h>
#include <stdio.h>

#include "vm/backtrace.h"
#include "vm/stack-trace.h"

int vm_enter_jni(void *caller_frame, unsigned long call_site_addr,
		 struct vm_method *method)
{
	return 0;
}

void vm_leave_jni(void)
{
}

void init_stack_trace_elem(struct stack_trace_elem *elem, unsigned long addr,
			   void *frame)
{
}

void init_stack_trace_elem_current(struct stack_trace_elem *elem)
{
}

int stack_trace_elem_next(struct stack_trace_elem *elem)
{
	return -1;
}

int stack_trace_elem_next_java(struct stack_trace_elem *elem)
{
	return -1;
}

void print_java_stack_trace_elem(struct stack_trace_elem *elem)
{
}

const char *stack_trace_elem_type_name(enum stack_trace_elem_type type)
{
	return NULL;
}

/* Must be inline so this does not change the backtrace. */
static inline void __show_stack_trace(unsigned long start, unsigned long caller)
{
	void *array[10];
	size_t size;
	size_t i;

	size = backtrace(array, 10);

	if (caller)
		array[1] = (void *) caller;

	trace_printf("Native stack trace:\n");
	for (i = start; i < size; i++) {
		trace_printf(" [<%08lx>] ", (unsigned long) array[i]);
		show_function(array[i]);
	}

	trace_flush();
}

void print_trace(void)
{
	__show_stack_trace(1, 0);
}

void print_trace_from(unsigned long eip, void *frame)
{
	__show_stack_trace(1, eip);
}
