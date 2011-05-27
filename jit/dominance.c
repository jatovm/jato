/*
 * Dominance frontier computation for SSA form
 * Copyright (c) 2010  Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 *
 * For more details on the algorithm used here, please refer to the following
 * paper:
 *
 *   Keith D. Cooper and Timothy J. Harvey and Ken Kennedy. A Simple, Fast
 *   Dominance Algorithm. 2001.
 */

#include "jit/compiler.h"

#include "lib/bitset.h"

#include "vm/stdlib.h"

static void do_compute_dfns(struct basic_block *bb, struct basic_block **df_array, int *dfn, struct basic_block *entry_bb)
{
	unsigned int i;

	for (i = 0; i < bb->nr_successors; i++) {
		struct basic_block *succ = bb->successors[i];

		if (succ->dfn || succ == entry_bb)
			continue;

		(*dfn)++;

		succ->dfn		= *dfn;

		df_array[succ->dfn]	= succ;

		do_compute_dfns(succ, df_array, dfn, entry_bb);
	}
}

int compute_dfns(struct compilation_unit *cu)
{
	int dfn = 0;

	cu->bb_df_array	= zalloc(sizeof(struct basic_block *) * nr_bblocks(cu));
	if (!cu->bb_df_array)
		return -ENOMEM;

	cu->bb_df_array[0] = cu->entry_bb;

	do_compute_dfns(cu->entry_bb, cu->bb_df_array, &dfn, cu->entry_bb);

	return 0;
}

static struct basic_block *intersect(struct basic_block *b1, struct basic_block *b2, struct basic_block **doms)
{
	struct basic_block *finger1, *finger2;

	finger1		= b1;
	finger2		= b2;

	while (finger1 != finger2) {
		if (finger1->dfn < finger2->dfn)
			finger2 = doms[finger2->dfn];
		else
			finger1 = doms[finger1->dfn];
	}

	return finger1;
}

int compute_dom(struct compilation_unit *cu)
{
	struct basic_block *start;
	bool changed;

	start	= cu->entry_bb;

	cu->doms = zalloc(sizeof(struct basic_block *) * nr_bblocks(cu));
	if (!cu->doms)
		return -ENOMEM;

	cu->doms[start->dfn]	= start;

	changed = true;
	while (changed) {
		struct basic_block *b;
		unsigned int ndx;

		changed = false;

		for (ndx = 0; ndx < nr_bblocks(cu); ndx++) {
			struct basic_block *new_idom;
			unsigned int i;

			b = cu->bb_df_array[ndx];

			if (!b || !b->dfn)
				continue;

			new_idom	= NULL;
			for (i = 0; i < b->nr_predecessors; i++) {
				struct basic_block *p = b->predecessors[i];

				if (!p->dfn && p != start)
					continue;

				if ((p != b) && cu->doms[p->dfn]) {
					new_idom	= p;
					break;
				}
			}

			if (b != start)
				assert(new_idom != NULL);

			while (i < b->nr_predecessors) {
				struct basic_block *p = b->predecessors[i++];

				if (!p->dfn && p != start)
					continue;

				if (cu->doms[p->dfn] != NULL) {
					new_idom	= intersect(p, new_idom, cu->doms);
				}
			}

			if (cu->doms[b->dfn] != new_idom) {
				if (b == start)
					cu->doms[b->dfn]	= b;
				else {
					cu->doms[b->dfn]	= new_idom;
					changed		= true;
				}
			}
		}
	}

	return 0;
}

int compute_dom_frontier(struct compilation_unit *cu)
{
	struct basic_block *b, *start;

	start	= cu->entry_bb;

	for_each_basic_block(b, &cu->bb_list) {
		b->dom_frontier	= alloc_bitset(nr_bblocks(cu));
		if (!b->dom_frontier)
			return -ENOMEM;
	}

	for_each_basic_block(b, &cu->bb_list) {
		if (b->nr_predecessors >= 2) {
			unsigned int i;

			for (i = 0; i < b->nr_predecessors; i++) {
				struct basic_block *p = b->predecessors[i];
				struct basic_block *runner;

				if (!p->dfn && p != start)
					continue;

				runner = p;

				while (runner != cu->doms[b->dfn]) {
					set_bit(runner->dom_frontier->bits, b->dfn);

					runner = cu->doms[runner->dfn];
				}
			}
		}
	}

	return 0;
}
