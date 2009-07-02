#include <stdbool.h>

struct compilation_unit;

bool opt_trace_invoke = false;
bool opt_trace_exceptions = false;

void trace_invoke(struct compilation_unit *cu)
{
}

void trace_exception(struct compilation_unit *cu, struct jit_stack_frame *frame,
		     unsigned char *native_ptr)
{
}

void trace_exception_handler(unsigned char *addr)
{
}

void trace_exception_unwind(struct jit_stack_frame *frame)
{
}

void trace_exception_unwind_to_native(struct jit_stack_frame *frame)
{
}

