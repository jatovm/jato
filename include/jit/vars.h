#ifndef __JIT_VARS_H
#define __JIT_VARS_H

#include "lib/list.h"
#include "arch/registers.h"
#include "vm/types.h"
#include "compilation-unit.h"
#include <stdbool.h>
#include <assert.h>

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

static inline unsigned long
range_intersection_start(struct live_range *range1, struct live_range *range2)
{
	assert(ranges_intersect(range1, range2));

	if (range1->start > range2->start)
		return range1->start;

	return range2->start;
}

static inline unsigned long range_len(struct live_range *range)
{
	return range->end - range->start;
}

static inline bool range_is_empty(struct live_range *range)
{
	if (range->start == ~0UL && range->end == 0)
		return true;

	return range_len(range) == 0;
}

struct var_info;

struct live_interval {
	/* Parent variable of this interval.  */
	struct var_info *var_info;

	/* Live range of this interval.  */
	struct live_range range;

	/* Linked list of child intervals.  */
	struct live_interval *next_child, *prev_child;

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

	/* The live interval where spill happened.  */
	struct live_interval *spill_parent;

	/* Is this interval fixed? */
	bool fixed_reg;
};

static inline bool has_use_positions(struct live_interval *it)
{
	return !list_is_empty(&it->use_positions);
}

static inline void
mark_need_reload(struct live_interval *it, struct live_interval *parent)
{
	it->need_reload = true;
	it->spill_parent = parent;
}

enum machine_reg_type {
	REG_TYPE_GPR,
	REG_TYPE_FPU,
};

struct var_info {
	unsigned long		vreg;
	struct var_info		*next;
	struct live_interval	*interval;
	enum machine_reg_type	type;
	enum vm_type		vm_type;
};

struct live_interval *alloc_interval(struct var_info *);
void free_interval(struct live_interval *);
struct live_interval *split_interval_at(struct live_interval *, unsigned long pos);
unsigned long next_use_pos(struct live_interval *, unsigned long);
struct live_interval *vreg_start_interval(struct compilation_unit *, unsigned long);
struct live_interval *interval_child_at(struct live_interval *, unsigned long);
#endif /* __JIT_VARS_H */
