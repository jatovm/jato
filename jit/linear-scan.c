/*
 * Linear scan register allocation
 * Copyright © 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * For more details on the linear scan register allocation algorithm,
 * please refer to the following papers:
 *
 * Poletto, M. and Sarkar, V. 1999. Linear scan register
 * allocation. ACM Trans. Program. Lang. Syst. 21, 5 (Sep. 1999),
 * 895-913.
 *
 * Wimmer, C. and Mössenböck, H. 2005. Optimized interval splitting
 * in a linear scan register allocator. In Proceedings of the 1st
 * ACM/USENIX international Conference on Virtual Execution
 * Environments (Chicago, IL, USA, June 11 - 12, 2005). VEE '05. ACM
 * Press, New York, NY, 132-141.
 */

#include <jit/compiler.h>
#include <jit/vars.h>
#include <vm/bitset.h>
#include <errno.h>
#include <stdlib.h>

/* The active list contains live intervals that overlap the current
   point and have been placed in registers. The list is kept sorted by
   increasing end point.  */
static void insert_to_active(struct live_interval *interval, struct list_head *active)
{
	struct live_interval *this;

	/*
	 * If we find an existing interval, that ends _after_ the
	 * new interval, add ours before that.
	 */
	list_for_each_entry(this, active, active) {
		if (interval->range.end < this->range.end) {
			list_add_tail(&interval->active, &this->active);
			return;
		}
	}
	/*
	 * Otherwise the new interval goes to the end of the list.
	 */
	list_add_tail(&interval->active, active);
}

static int try_to_alloc_reg(struct bitset *registers)
{
	int ret;

	ret = bitset_ffs(registers);
	assert(ret < NR_REGISTERS);
	if (ret >= 0)
		clear_bit(registers->bits, ret);

	return ret;
}

static void expire_old_intervals(struct live_interval *current,
				 struct list_head *active,
				 struct bitset *registers)
{
	struct live_interval *this, *prev;

	list_for_each_entry_safe(this, prev, active, active) {
		if (this->range.end > current->range.start)
			return;

		/*
		 * Remove interval from active list...
		 */
		list_del(&this->active);

		/*
		 * ...and return register to the pool of free registers.
		 */
		set_bit(registers->bits, this->reg);
	}
}

/* The interval list contains all intervals sorted by increasing start
   point.  */
static void insert_to_intervals(struct live_interval *interval,
				struct list_head *intervals)
{
	struct live_interval *this;

	/*
	 * If we find an existing interval, that starts _after_ the
	 * new interval, add ours before that.
	 */
	list_for_each_entry(this, intervals, interval) {
		if (interval->range.start < this->range.start) {
			list_add_tail(&interval->interval, &this->interval);
			return;
		}
	}
	/*
	 * Otherwise the new interval goes to the end of the list.
	 */
	list_add_tail(&interval->interval, intervals);
}

int allocate_registers(struct compilation_unit *cu)
{
	struct list_head intervals = LIST_HEAD_INIT(intervals);
	struct list_head active = LIST_HEAD_INIT(active);
	struct live_interval *this;
	struct bitset *registers;
	struct var_info *var;

	registers = alloc_bitset(NR_REGISTERS);
	if (!registers)
		return -ENOMEM;

	/*
	 * Mark all registers available in the pool of free registers.
	 */
	bitset_set_all(registers);

	/*
	 * Sort intervals in the order of increasing start point. We
	 * add intervals with register constraints first, so that that
	 * we when a fixed and non-fixed intervals overlap, we always
	 * process the fixed interval first.
	 */
	for_each_variable(var, cu->var_infos) {
		if (var->interval->reg != REG_UNASSIGNED)
			insert_to_intervals(var->interval, &intervals);
	}

	for_each_variable(var, cu->var_infos) {
		if (var->interval->reg == REG_UNASSIGNED)
			insert_to_intervals(var->interval, &intervals);
	}

	list_for_each_entry(this, &intervals, interval) {
		expire_old_intervals(this, &active, registers);

		if (this->reg == REG_UNASSIGNED) {
			int reg = try_to_alloc_reg(registers);
			if (reg < 0)
				assert(!"oops: need to spill!");

			this->reg = reg;
		} else {
			/*
			 * Remove the register of this fixed interval
			 * from the pool of free registers. As we
			 * process fixed intervals first, this ensures
			 * that we don't assign the same register to
			 * overlapping non-fixed intervals.
			 */
			clear_bit(registers->bits, this->reg);
		}
		insert_to_active(this, &active);
	}
	free(registers);

	return 0;
}
