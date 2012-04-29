#ifndef __JIT_STACK_SLOT_H
#define __JIT_STACK_SLOT_H

#include "vm/types.h"

struct stack_frame;
enum machine_reg;

struct stack_slot {
	struct stack_frame *parent;

	/* Linked list of stack slots.  */
	struct stack_slot *next;

	/* In the class file, a 32-bit local variable occupies one index and a
	   64-bit variable occupies two which gives us a natural mapping for
	   stack slots.  */
	unsigned long index;
};

struct stack_frame {
	/* Number of arguments in this stack frame.  */
	unsigned long nr_args;

	/* Number of stack slots for local variables in this stack frame
	   (including arguments on the stack).  */

	unsigned long nr_local_slots;

	/* Table of stack slots for local variables by index.  Note: 64-bit
	   variables occupy two stack slots.  */
	struct stack_slot *local_slots;

	/* Number of stack slots for spill/reload storage in this stack
	   frame.  */
	unsigned long nr_spill_slots;

	/* Table of stack slots for spill/reload storage.  */
	struct stack_slot *spill_slots;
};

struct stack_frame *alloc_stack_frame(unsigned long nr_args, unsigned long nr_local_slots);
void free_stack_frame(struct stack_frame *frame);

struct stack_slot *get_local_slot(struct stack_frame *frame, unsigned long index);
struct stack_slot *get_spill_slot_32(struct stack_frame *frame);
struct stack_slot *get_spill_slot_64(struct stack_frame *frame);
struct stack_slot *get_spill_slot(struct stack_frame *frame, enum vm_type type);
struct stack_slot *get_next_slot(struct stack_slot *slot);

#endif /* __JIT_STACK_SLOT_H */
