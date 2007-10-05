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

struct live_interval {
	struct live_range	range;
	enum machine_reg	reg;
	struct list_head	interval;
	struct list_head	active;
};

struct var_info {
	unsigned long vreg;
	enum machine_reg reg;
	struct var_info *next;
	struct live_interval interval;
};

static inline bool is_vreg(struct var_info *var, unsigned long vreg)
{
	return var->vreg == vreg;
}

#endif /* __JIT_VARS_H */
