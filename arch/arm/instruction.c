#include "jit/compilation-unit.h"
#include "jit/instruction.h"
#include "jit/vars.h"
#include "jit/lir-printer.h"
#include "arch/registers.h"

#include <stdlib.h>
#include <string.h>

enum {
	DEF_DST			= (1U << 1),
	DEF_SRC			= (1U << 2),
	DEF_NONE		= (1U << 3),
	USE_DST			= (1U << 4),
	USE_NONE		= (1U << 5),
	USE_SRC			= (1U << 6),
	USE_FP			= (1U << 7),	/* frame pointer */

};

static unsigned long insn_flags[] = {

};

int insn_defs(struct compilation_unit *cu, struct insn *insn, struct var_info **defs)
{
	assert(!"not implemented");
}

int insn_uses(struct insn *insn, struct var_info **uses)
{
	assert(!"not implemented");
}

bool insn_is_branch(struct insn *insn)
{
	assert(!"not implemented");
}

bool insn_is_call(struct insn *insn)
{
	assert(!"not implemented");
}

void insn_sanity_check(void)
{
	assert(!"not implemented");
}

struct insn *insn(enum insn_type insn_type)
{
	assert(!"not implemented");
}

int insert_copy_slot_32_insns(struct stack_slot *stack_slot1, struct stack_slot *stack_slot2, struct list_head *list_head, unsigned long a)
{
	assert(!"not implemented");
}

int insert_copy_slot_64_insns(struct stack_slot *stack_slot1, struct stack_slot *stack_slot2, struct list_head *list_head, unsigned long a)
{
	assert(!"not implemented");
}

struct insn *spill_insn(struct var_info *var, struct stack_slot *slot)
{
	assert(!"not implemented");
}

struct insn *reload_insn(struct stack_slot *slot, struct var_info *var)
{
	assert(!"not implemented");
}

struct insn *jump_insn(struct basic_block *bb)
{
	assert(!"not implemented");
}

const char *reg_name(enum machine_reg reg)
{
	assert(!"not implemented");
}

bool reg_supports_type(enum machine_reg reg, enum vm_type type)
{
	assert(!"not implemented");
}

struct insn *alloc_insn(enum insn_type type)
{
	struct insn *insn = malloc(sizeof *insn);
	if (insn) {
		memset(insn, 0, sizeof *insn);
		INIT_LIST_HEAD(&insn->insn_list_node);
		INIT_LIST_HEAD(&insn->branch_list_node);
		insn->type = type;
	}
	return insn;
}

int insn_operand_use_kind(struct insn *insn, struct operand *operand)
{
	unsigned long flags;
	int kind_mask;
	int use_mask;
	int def_mask;

	flags = insn_flags[insn->type];

	if (operand == &insn->src) {
		use_mask = USE_SRC;
		def_mask = DEF_SRC;
	} else {
		assert(operand == &insn->dest);
		use_mask = USE_DST;
		def_mask = DEF_DST;
	}

	kind_mask = 0;
	if (flags & use_mask)
		kind_mask |= USE_KIND_INPUT;

	if (flags & def_mask)
		kind_mask |= USE_KIND_OUTPUT;

	return kind_mask;
}

static bool operand_uses_reg(struct operand *operand)
{
	return false;
}

static void release_operand(struct operand *operand)
{
	if (operand_uses_reg(operand))
		list_del(&operand->reg.use_pos_list);

}

void free_insn(struct insn *insn)
{
	release_operand(&insn->src);
	release_operand(&insn->dest);

	free(insn);
}
