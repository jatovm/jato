#ifndef __X86_FRAME_H
#define __X86_FRAME_H

#include <jit/stack-slot.h>

struct vm_method;
struct expression;

unsigned long frame_local_offset(struct vm_method *, struct expression *);
unsigned long slot_offset(struct stack_slot *slot);
unsigned long frame_locals_size(struct stack_frame *frame);

#endif
