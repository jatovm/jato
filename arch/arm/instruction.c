#include "jit/compilation-unit.h"
#include "jit/instruction.h"
#include "jit/vars.h"
#include "jit/lir-printer.h"
#include "arch/registers.h"

#include <stdlib.h>
#include <string.h>

void free_insn(struct insn *insn)
{
	assert(!"not implemented");
}

int insn_defs(struct compilation_unit *cu, struct insn *insn, struct var_info **defs)
{
	assert(!"not implemented");
}

int insn_uses(struct insn *insn, struct var_info **uses)
{
	assert(!"not implemented");
}

int insn_operand_use_kind(struct insn *insn, struct operand *operand)
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
