#ifndef __X86_FRAME_H
#define __X86_FRAME_H

#include <jit/stack-slot.h>
#include <stdbool.h>

struct methodblock;
struct expression;
struct compilation_unit;

struct jit_stack_frame {
       struct jit_stack_frame *prev;
       unsigned long old_ebx;
       unsigned long old_esi;
       unsigned long old_edi;
       unsigned long return_address;
       unsigned long args[0];
} __attribute__((packed));

unsigned long frame_local_offset(struct methodblock *, struct expression *);
unsigned long slot_offset(struct stack_slot *slot);
unsigned long frame_locals_size(struct stack_frame *frame);
bool is_jit_method(unsigned long eip);
unsigned long cu_frame_locals_offset(struct compilation_unit *cu);

#endif
