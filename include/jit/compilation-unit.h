#ifndef __JIT_COMPILATION_UNIT_H
#define __JIT_COMPILATION_UNIT_H

#include <jit/basic-block.h>

#include <vm/list.h>
#include <vm/stack.h>

#include <arch/stack-frame.h>
#include <arch/instruction.h>

#include <stdbool.h>
#include <pthread.h>

struct buffer;
struct vm_method;

struct compilation_unit {
	/* Jam VM */
	struct methodblock *method;

	/* Cafebabe */
	struct vm_method *vm_method;

	struct list_head bb_list;
	struct basic_block *exit_bb;
	struct var_info *var_infos;
	unsigned long nr_vregs;
	struct buffer *objcode;
	bool is_compiled;
	pthread_mutex_t mutex;

	/* The frame pointer for this method.  */
	struct var_info *frame_ptr;

	/* The stack pointer for this method.  */
	struct var_info *stack_ptr;

	/* The stack frame contains information of stack slots for stack-based
	   arguments, local variables, and spill/reload storage.  */
	struct stack_frame *stack_frame;
};

struct compilation_unit *alloc_compilation_unit(struct methodblock *);
struct compilation_unit *compilation_unit_alloc(struct vm_method *);
int init_stack_slots(struct compilation_unit *cu);
void free_compilation_unit(struct compilation_unit *);
struct var_info *get_var(struct compilation_unit *);
struct var_info *get_fixed_var(struct compilation_unit *, enum machine_reg);
struct basic_block *find_bb(struct compilation_unit *, unsigned long);
unsigned long nr_bblocks(struct compilation_unit *);
void compute_insn_positions(struct compilation_unit *);
int sort_basic_blocks(struct compilation_unit *);

#define for_each_variable(var, var_list) for (var = var_list; var != NULL; var = var->next)

#endif
