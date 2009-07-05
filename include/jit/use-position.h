#ifndef __JIT_USE_POSITION_H
#define __JIT_USE_POSITION_H

#include "jit/vars.h"
#include "lib/list.h"

struct insn;

/**
 * struct use_position - register use position
 *
 * Each instruction can operate on multiple registers. That is, every
 * instruction can have zero or more operands and each operand can use zero or
 * more registers depending on the addressing mode.
 *
 * This struct is used by code emission to look up the actual assigned machine
 * register of an operand register or base register and an optional index
 * register. We keep struct use_positions in a list in an interval so that we
 * know which operands moved to another interval when an interval is split.
 */
struct use_position {
	struct insn		*insn;
	struct live_interval	*interval;
	struct list_head	use_pos_list;	/* node in interval use position list */
};

static inline void register_set_insn(struct use_position *reg, struct insn *insn)
{
	reg->insn = insn;
}

static inline void insert_register_to_interval(struct live_interval *interval,
					       struct use_position *reg)
{
	INIT_LIST_HEAD(&reg->use_pos_list);
	list_add(&reg->use_pos_list, &interval->use_positions);

	reg->interval = interval;
}

static inline void init_register(struct use_position *reg, struct insn *insn,
				 struct live_interval *interval)
{
	register_set_insn(reg, insn);
	insert_register_to_interval(interval, reg);
}

static inline enum machine_reg mach_reg(struct use_position *reg)
{
	return reg->interval->reg;
}

static inline struct var_info *mach_reg_var(struct use_position *reg)
{
	return reg->interval->var_info;
}

static inline bool is_vreg(struct use_position *reg, unsigned long vreg)
{
	return reg->interval->var_info->vreg == vreg;
}

#endif /* __JIT_USE_POSITION_H */
