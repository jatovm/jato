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

static inline bool ranges_intersect(struct live_range *range1, struct live_range *range2)
{
	return range2->start < range1->end && range1->start < range2->end;
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
	struct list_head use_positions;

	/* Member of list of unhandled, active, or inactive intervals during
	   linear scan.  */
	struct list_head interval_node;

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

struct live_interval *alloc_interval(struct var_info *);
void free_interval(struct live_interval *);
struct live_interval *split_interval_at(struct live_interval *, unsigned long pos);

#endif /* __JIT_VARS_H */
