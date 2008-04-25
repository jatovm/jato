#ifndef __JIT_VARS_H
#define __JIT_VARS_H

#include <vm/list.h>
#include <arch/registers.h>
#include <stdbool.h>

struct live_range {
	unsigned long start, end;	/* end is exclusive */
};

static inline bool in_range(struct live_range *range, unsigned long offset)
{
	return (offset >= range->start) && (offset < range->end);
}

static inline unsigned long range_len(struct live_range *range)
{
	return range->end - range->start;
}

static inline bool range_is_empty(struct live_range *range)
{
	return range->start == ~0UL && range->end == 0;
}

struct var_info;
struct insn;

struct live_interval {
	/* Parent variable of this interval.  */
	struct var_info *var_info;

	/* Live range of this interval.  */
	struct live_range range;

	/* Linked list of child intervals.  */
	struct live_interval *next_child;

	/* Machine register of this interval.  */
	enum machine_reg reg;

	/* List of register use positions in this interval.  */
	struct list_head registers;

	/* Member of list of intervals during linear scan.  */
	struct list_head interval;

	/* Member of list of active intervals during linear scan.  */
	struct list_head active;

	/* Array of instructions within this interval.  */
	struct insn **insn_array;

	/* Do we need to spill the register of this interval?  */
	bool need_spill;

	/* Do we need to reload the register of this interval?  */
	bool need_reload;

	/* The slot this interval is spilled to. Only set if ->need_spill is
	   true.  */
	struct stack_slot *spill_slot;
};

struct var_info {
	unsigned long		vreg;
	struct var_info		*next;
	struct live_interval	*interval;
};

struct insn;

/**
 * struct register_info - register use position
 *
 * Each instruction can operate on multiple registers. That is, every
 * instruction can have zero or more operands and each operand can use zero or
 * more registers depending on the addressing mode.
 *
 * This struct is used by code emission to look up the actual assigned machine
 * register of an operand register or base register and an optional index
 * register. We keep struct register_infos in a list in an interval so that we
 * know which operands moved to another interval when an interval is split.
 */
struct register_info {
	struct insn		*insn;
	struct live_interval	*interval;
	struct list_head	reg_list;
};

static inline void register_set_insn(struct register_info *reg, struct insn *insn)
{
	reg->insn = insn;
}

static inline void insert_register_to_interval(struct live_interval *interval,
					       struct register_info *reg)
{
	INIT_LIST_HEAD(&reg->reg_list);
	list_add(&reg->reg_list, &interval->registers);

	reg->interval = interval;
}

static inline void init_register(struct register_info *reg, struct insn *insn,
				 struct live_interval *interval)
{
	register_set_insn(reg, insn);
	insert_register_to_interval(interval, reg);
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
void free_interval(struct live_interval *);
struct live_interval *split_interval_at(struct live_interval *, unsigned long pos);

#endif /* __JIT_VARS_H */
