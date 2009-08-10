#include <errno.h>

#include "arch/instruction.h"
#include "jit/compilation-unit.h"
#include "jit/gc-map.h"
#include "lib/bitset.h"

int gc_map_init(struct compilation_unit *cu)
{
	cu->safepoint_map = alloc_radix_tree(8, 8 * sizeof(unsigned long));
	if (!cu->safepoint_map)
		return -ENOMEM;

	struct basic_block *bb;
	for_each_basic_block(bb, &cu->bb_list) {
		struct insn *insn;
		for_each_insn(insn, &bb->insn_list) {
			if (!insn->safepoint)
				continue;

			/* XXX: Only allocate a map for vregs of type
			 * J_REFERENCE. */
			struct bitset *live_vars = alloc_bitset(cu->nr_vregs);
			if (!live_vars)
				return -ENOMEM;

			struct var_info *var;
			for_each_variable(var, cu->var_infos) {
				if  (var->vm_type != J_REFERENCE)
					continue;

				if (!in_range(&var->interval->range,
					insn->mach_offset))
				{
					continue;
				}

				set_bit(live_vars->bits, var->vreg);
			}

			radix_tree_insert(cu->safepoint_map,
				insn->mach_offset, live_vars);
		}
	}

	return 0;
}
