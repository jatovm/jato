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
			struct var_info *var;

			if (!insn->safepoint)
				continue;

			/* Count the number of reference-type variables */
			unsigned int nr_ref_vars = 0;
			for_each_variable(var, cu->var_infos) {
				if (var->vm_type == J_REFERENCE)
					++nr_ref_vars;
			}

			/* XXX: XMM registers can never hold references, so
			 * we should not allocate bits for them. */
			struct bitset *live_vars
				= alloc_bitset(NR_REGISTERS + nr_ref_vars);
			if (!live_vars)
				return -ENOMEM;

			unsigned int i = NR_REGISTERS;
			for_each_variable(var, cu->var_infos) {
				if (var->vm_type != J_REFERENCE)
					continue;

				set_bit(live_vars->bits, i++);
			}

			radix_tree_insert(cu->safepoint_map,
				insn->mach_offset, live_vars);
		}
	}

	return 0;
}
