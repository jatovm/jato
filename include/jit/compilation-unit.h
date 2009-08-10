#ifndef __JIT_COMPILATION_UNIT_H
#define __JIT_COMPILATION_UNIT_H

#include "arch/registers.h"
#include "jit/basic-block.h"
#include "lib/list.h"
#include "lib/radix-tree.h"
#include "vm/stack.h"
#include "vm/static.h"
#include "vm/types.h"

#include "arch/stack-frame.h"

#include <stdbool.h>
#include <pthread.h>

struct buffer;
struct vm_method;
struct insn;
enum machine_reg;

struct compilation_unit {
	struct vm_method *method;
	struct list_head bb_list;
	struct basic_block *entry_bb;
	struct basic_block *exit_bb;
	struct basic_block *unwind_bb;
	struct var_info *var_infos;
	unsigned long nr_vregs;
	struct var_info *fixed_var_infos[NR_FIXED_REGISTERS];
	bool is_reg_alloc_done;
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

	/* It's needed to spill exception object reference at eh entry */
	struct stack_slot *exception_spill_slot;

	/*
	 * Pointers inside exit block and unwind block after monitor
	 * unlocking code. The code is only present if method is
	 * synchronized. These pointers are used to skip unlocking
	 * when exception is thrown from that unlocking code.
	 */
	unsigned char *exit_past_unlock_ptr;
	unsigned char *unwind_past_unlock_ptr;

	struct list_head static_fixup_site_list;
	struct list_head tableswitch_list;
	struct list_head lookupswitch_list;

	/*
	 * This holds a pointer to the method's code. It's value is
	 * valid only when ->is_compiled is true.
	 * For non-native methods it's set to buffer_ptr(objcode). For
	 * native functions it points to the actuall native function's
	 * code (not trampoline).
	 */
	void *native_ptr;

	/*
	 * This maps bytecode offset to every native address
	 * inside JIT code.
	 */
	unsigned long *bc_offset_map;

	/*
	 * This maps LIR offset to instruction.
	 */
	struct radix_tree *lir_insn_map;

	/*
	 * This maps machine-code offset (of gc safepoint) to gc map
	 */
	struct radix_tree *safepoint_map;
};

struct compilation_unit *compilation_unit_alloc(struct vm_method *);
int init_stack_slots(struct compilation_unit *cu);
void free_compilation_unit(struct compilation_unit *);
void shrink_compilation_unit(struct compilation_unit *);
struct var_info *get_var(struct compilation_unit *, enum vm_type);
struct var_info *get_fixed_var(struct compilation_unit *, enum machine_reg);
struct basic_block *find_bb(struct compilation_unit *, unsigned long);
unsigned long nr_bblocks(struct compilation_unit *);
void compute_insn_positions(struct compilation_unit *);
int sort_basic_blocks(struct compilation_unit *);

#define for_each_variable(var, var_list) for (var = var_list; var != NULL; var = var->next)

#endif
