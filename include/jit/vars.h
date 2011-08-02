#ifndef __JIT_VARS_H
#define __JIT_VARS_H

#include "lib/list.h"
#include "arch/registers.h"
#include "vm/types.h"
#include "compilation-unit.h"
#include <stdbool.h>
#include <assert.h>

struct live_range {
	unsigned int			start;
	unsigned int			end;	/* end is exclusive */
	struct list_head		range_list_node;
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

struct live_interval;

struct var_info {
	struct var_info			*next;
	struct live_interval		*interval;
	uint32_t			vreg;
	uint8_t				vm_type;
};

enum interval_flag_type {
	/* Do we need to spill the register of this interval?  */
	INTERVAL_FLAG_NEED_SPILL		= 1U << 0,
	/* Do we need to reload the register of this interval?  */
	INTERVAL_FLAG_NEED_RELOAD		= 1U << 1,
	/* Is this interval fixed? */
	INTERVAL_FLAG_FIXED_REG			= 1U << 2,
};

struct live_interval {
	/* Parent variable of this interval.  */
	struct var_info *var_info;

	/* Live ranges of this interval. List of not overlaping and
	   not adjacent ranges sorted in ascending order. */
	struct list_head range_list;

	/*
	 * Contains ranges which were moved from range_list to speedup
	 * some interval oprations. Intervals can have a lot of live
	 * ranges. Linear scan algorithm goes through intervals in
	 * ascending order by interval start. We can take advantage of
	 * this and don't check ranges before current position.
	 */
	struct list_head expired_range_list;

	/* Linked list of child intervals.  */
	struct live_interval *next_child, *prev_child;

	/* List of register use positions in this interval.  */
	struct list_head use_positions;

	/* Member of list of unhandled, active, or inactive intervals during
	   linear scan.  */
	struct list_head interval_node;

	/* The slot this interval is spilled to. Only set if ->need_spill is
	   true.  */
	struct stack_slot *spill_slot;

	/* This struct var_info is used during spill/reload stage to point to
	   the allocated machine register for this interval.  */
	struct var_info spill_reload_reg;

	/* The live interval where spill happened.  */
	struct live_interval *spill_parent;

	/* See enum interval_flag_type for details.  */
	uint8_t flags;

	/* Machine register of this interval.  See enum machine_reg for details.  */
	int8_t reg;
};

static inline bool has_use_positions(struct live_interval *it)
{
	return !list_is_empty(&it->use_positions);
}

static inline bool interval_needs_reload(struct live_interval *it)
{
	return it->flags & INTERVAL_FLAG_NEED_RELOAD;
}

static inline bool interval_needs_spill(struct live_interval *it)
{
	return it->flags & INTERVAL_FLAG_NEED_SPILL;
}

static inline bool interval_has_fixed_reg(struct live_interval *it)
{
	return it->flags & INTERVAL_FLAG_FIXED_REG;
}

static inline void
mark_need_reload(struct live_interval *it, struct live_interval *parent)
{
	it->flags		|= INTERVAL_FLAG_NEED_RELOAD;
	it->spill_parent	= parent;
}

static inline void mark_need_spill(struct live_interval *it)
{
	it->flags |= INTERVAL_FLAG_NEED_SPILL;
}

static inline struct live_range *node_to_range(struct list_head *node)
{
	return list_entry(node, struct live_range, range_list_node);
}

static inline struct live_range *
next_range(struct list_head *list, struct live_range *range)
{
	if (range->range_list_node.next == list)
		return NULL;

	return list_entry(range->range_list_node.next, struct live_range,
			  range_list_node);
}

static inline struct live_range *interval_first_range(struct live_interval *it)
{
	return list_first_entry(&it->range_list, struct live_range, range_list_node);
}

static inline unsigned long interval_start(struct live_interval *it)
{
	struct live_range *first = interval_first_range(it);

	return first->start;
}

static inline unsigned long interval_end(struct live_interval *it)
{
	return node_to_range(it->range_list.prev)->end;
}

static inline bool interval_is_empty(struct live_interval *it)
{
	return list_is_empty(&it->range_list);
}

struct live_interval *alloc_interval(struct compilation_unit *cu, struct var_info *);
struct live_interval *split_interval_at(struct compilation_unit *, struct live_interval *, unsigned long pos);
unsigned long next_use_pos(struct live_interval *, unsigned long);
struct live_interval *vreg_start_interval(struct compilation_unit *, unsigned long);
struct live_interval *interval_child_at(struct live_interval *, unsigned long);
bool intervals_intersect(struct live_interval *, struct live_interval *);
unsigned long interval_intersection_start(struct live_interval *, struct live_interval *);
bool interval_covers(struct live_interval *, unsigned long);
int interval_add_range(struct compilation_unit *, struct live_interval *, unsigned long, unsigned long);
struct live_range *interval_range_at(struct live_interval *, unsigned long);
void interval_expire_ranges_before(struct live_interval *, unsigned long);
void interval_restore_expired_ranges(struct live_interval *);

static inline unsigned long first_use_pos(struct live_interval *it)
{
	return next_use_pos(it, 0);
}

#endif /* __JIT_VARS_H */
