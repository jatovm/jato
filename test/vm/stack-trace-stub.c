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
