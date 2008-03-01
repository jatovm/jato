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
	/* Parent variable of this interval.  */
	struct var_info *var_info;

	/* Live range of this interval.  */
	struct live_range range;

	/* Machine register of this interval.  */
	enum machine_reg reg;

	/* List of use positions of this interval.  */
	struct list_head registers;

	/* Member of list of intervals during linear scan.  */
	struct list_head interval;

	/* Member of list of active intervals during linear scan.  */
	struct list_head active;
};

struct var_info {
	unsigned long		vreg;
	struct var_info		*next;
	struct live_interval	*interval;
};

struct insn;

struct register_info {
	struct insn		*insn;
	struct live_interval	*interval;
	struct list_head	reg_list;
};

static inline void __assoc_var_to_operand(struct var_info *var,
					  struct insn *insn,
					  struct register_info *reg_info)
{
	reg_info->insn = insn;
	reg_info->interval = var->interval;
	INIT_LIST_HEAD(&reg_info->reg_list);
	list_add(&reg_info->reg_list, &var->interval->registers);
}

#define assoc_var_to_operand(_var, _insn, _operand) \
		__assoc_var_to_operand(_var, _insn, &((_insn)->_operand));
	
static inline void var_associate_reg(struct register_info *reg, struct var_info *var)
{
	reg->interval = var->interval;
}

static inline enum machine_reg mach_reg(struct register_info *reg)
{
	return reg->interval->reg;
}

static inline struct var_info *mach_reg_var(struct register_info *reg)
{
	return reg->interval->var_info;
}

static inline bool is_vreg(struct register_info *reg, unsigned long vreg)
{
	return reg->interval->var_info->vreg == vreg;
}

struct live_interval *alloc_interval(struct var_info *);
struct live_interval *split_interval_at(struct live_interval *, unsigned long pos);

#endif /* __JIT_VARS_H */
