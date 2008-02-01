#ifndef __JIT_VARS_H
#define __JIT_VARS_H

#include <vm/list.h>
#include <stdbool.h>

struct live_range {
	unsigned long start, end;
};

static inline bool in_range(struct live_range *range, unsigned long offset)
{
	return (range->start <= offset) && (range->end >= offset);
}

struct var_info;

struct live_interval {
	struct var_info		*var_info;
	struct live_range	range;
	enum machine_reg	reg;
	struct list_head	interval;
	struct list_head	active;
};

struct var_info {
	unsigned long		vreg;
	struct var_info		*next;
	struct live_interval	interval;
};

struct insn;

struct register_info {
	struct insn		*insn;
	struct live_interval	*interval;
};

#define assoc_var_to_operand(var, insn, operand)	\
do {							\
	insn->operand.insn = insn;			\
	insn->operand.interval = &var->interval;	\
} while (0);

static inline void var_associate_reg(struct register_info *reg, struct var_info *var)
{
	reg->interval = &var->interval;
}

static inline enum machine_reg register_get(struct register_info *reg)
{
	return reg->interval->reg;
}

static inline struct var_info *register_get_var(struct register_info *reg)
{
	return reg->interval->var_info;
}

static inline bool is_vreg(struct register_info *reg, unsigned long vreg)
{
	return reg->interval->var_info->vreg == vreg;
}

#endif /* __JIT_VARS_H */
