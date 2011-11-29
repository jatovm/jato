#ifndef __JIT_COMPILATION_UNIT_H
#define __JIT_COMPILATION_UNIT_H

#include "arch/registers.h"
#include "arch/stack-frame.h"

#include "jit/basic-block.h"

#include "lib/arena.h"
#include "lib/list.h"
#include "lib/radix-tree.h"
#include "lib/stack.h"

#include "vm/static.h"
#include "vm/types.h"
#include "vm/gc.h"

#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

struct buffer;
struct vm_method;
struct insn;
enum machine_reg;

enum compilation_state {
	COMPILATION_STATE_INITIAL,
	COMPILATION_STATE_COMPILING,
	COMPILATION_STATE_COMPILED,
};

enum {
	CU_FLAG_ARRAY_OPC,
	CU_FLAG_REGALLOC_DONE,
};

struct compilation_unit {
	struct vm_method *method;
	uint32_t flags;
	unsigned long nr_bb;
	struct list_head bb_list;
	struct basic_block *entry_bb;
	struct basic_block *exit_bb;
	struct basic_block *unwind_bb;
	struct basic_block **bb_df_array;	/* depth-first postorder */
	struct basic_block **doms;
	struct var_info *var_infos;
	unsigned long nr_vregs;
	struct var_info *fixed_var_infos[NR_FIXED_REGISTERS];
	struct buffer *objcode;

	/* Mutex that protects ->state  */
	pthread_mutex_t compile_mutex;

	/* See enum compilation_state for values */
	unsigned long state;

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

	/*
	 * These are native pointers to basic blocks.
	 */
	unsigned char *exit_bb_ptr;
	unsigned char *unwind_bb_ptr;

	/*
	 * These are used by the SSA part.
	 */
	struct {
		/*
		 * ssa_var_infos represent renamed virtual registers.
		 * We need separate sets for this virtual registers to model
		 * the SSA renaming scheme.
		 * They will replace the virtual registers in var_infos
		 * after SSA deconstruction.
		 */
		struct var_info *ssa_var_infos;
		unsigned long ssa_nr_vregs;

		/*
		 * Hashmap that keeps the associations between an instruction
		 * and its third operand.
		 */
		struct hash_map *insn_add_ons;
	};

	struct list_head static_fixup_site_list;
	struct list_head call_fixup_site_list;
	struct list_head tableswitch_list;
	struct list_head lookupswitch_list;
	struct list_head ic_call_list;

	/*
	 * Entry points to the method's code. These values are
	 * valid only when ->is_compiled is true.
	 * entry_point
	 * 	points to method's objcode.
	 * ic_entry_point
	 * 	points to inline_cache_check for this method.
	 * 	To be used only when calling from a call-site
	 *	with an inline cache.
	 */
	void *entry_point;
	void *ic_entry_point;

	/*
	 * This maps bytecode offset to every native address
	 * inside JIT code.
	 */
	unsigned long *bc_offset_map;

	/*
	 * This maps LIR offset to instruction.
	 */
	struct radix_tree *lir_insn_map;

	unsigned long last_insn;

	/*
	 * Contains native pointers of exception handlers. Indices to
	 * this table are the same as for exception table in code
	 * attribute.
	 */
	void **exception_handlers;

	/*
	 * These stack slot for storing temporary results within one monoburg
	 * rule where we can not use a register.
	 */
	struct stack_slot *scratch_slot;

#ifdef CONFIG_ARGS_MAP
	struct var_info **non_fixed_args;
#endif

	struct arena *arena;

	/*
	 * This is used for ARM where we have an immediate of less than 8 bit
	 * so, to store the larger immediate we use a constant literal pool
	 */
	struct lp_entry *pool_head;
	unsigned int nr_entries_in_pool;
};

static inline unsigned long nr_bblocks(struct compilation_unit *cu)
{
	return cu->nr_bb;
}

static inline void *cu_entry_point(struct compilation_unit *cu)
{
	return cu->entry_point;
}

static inline void *cu_ic_entry_point(struct compilation_unit *cu)
{
	return cu->ic_entry_point;
}

unsigned long compilation_unit_get_state(struct compilation_unit *cu);

static inline bool compilation_unit_is_compiled(struct compilation_unit *cu)
{
	/* Optimistic unlocked check */
	if (cu->state == COMPILATION_STATE_COMPILED)
		return true;

	/* Slowpath */
	return compilation_unit_get_state(cu) == COMPILATION_STATE_COMPILED;
}

struct compilation_unit *compilation_unit_alloc(struct vm_method *);
int init_stack_slots(struct compilation_unit *cu);
void free_compilation_unit(struct compilation_unit *);
void shrink_compilation_unit(struct compilation_unit *);
void resolve_fixup_offsets(struct compilation_unit *);
struct var_info *get_var(struct compilation_unit *, enum vm_type);
struct var_info *get_fixed_var(struct compilation_unit *, enum machine_reg);
struct var_info *ssa_get_var(struct compilation_unit *, enum vm_type);
struct var_info *ssa_get_fixed_var(struct compilation_unit *, enum machine_reg);
struct basic_block *find_bb(struct compilation_unit *, unsigned long);
void compute_insn_positions(struct compilation_unit *);
struct stack_slot *get_scratch_slot(struct compilation_unit *);

#define for_each_variable(var, var_list) for (var = var_list; var != NULL; var = var->next)

#endif
