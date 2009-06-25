#ifndef JATO_VM_STACK_TRACE_H
#define JATO_VM_STACK_TRACE_H

#include <arch/stack-frame.h>

#include <vm/stack-trace.h>
#include <vm/natives.h>
#include <vm/vm.h>

#include <stdbool.h>

/*
 * Points to a native stack frame that is considered as bottom-most
 * for given thread.
 */
extern __thread struct native_stack_frame *bottom_stack_frame;

struct stack_trace_elem {
	/* Holds instruction address of this stack trace element. */
	unsigned long addr;

	/*
	 * If true then @frame has format of struct native_stack_frame
	 * and struct jit_stack_frame otherwise.
	 */
	bool is_native;

	/* If true then frame belongs to a trampoline */
	bool is_trampoline;

	/* Points to a stack frame of this stack trace element. */
	void *frame;
};

void init_stack_trace_printing(void);
int init_stack_trace_elem(struct stack_trace_elem *elem);
int get_prev_stack_trace_elem(struct stack_trace_elem *elem);
int skip_frames_from_class(struct stack_trace_elem *elem, struct vm_class *class);
int get_stack_trace_depth(struct stack_trace_elem *elem);
struct vm_object *get_stack_trace(struct stack_trace_elem *);
struct vm_object *get_stack_trace_from_ctx(void *ctx);
struct vm_object *convert_stack_trace(struct vm_object *vmthrowable);
struct vm_object *new_stack_trace_element(struct vm_method *, unsigned long);
struct vm_object * __vm_native vm_throwable_fill_in_stack_trace(struct vm_object *);
struct vm_object * __vm_native vm_throwable_get_stack_trace(struct vm_object *, struct vm_object *);
void set_throwable_vmstate(struct vm_object *throwable, struct vm_object *vmstate);

bool called_from_jit_trampoline(struct native_stack_frame *frame);
void vm_print_exception(struct vm_object *exception);

#endif /* JATO_VM_STACK_TRACE_H */
