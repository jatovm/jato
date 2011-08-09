#include "arch/peephole.h"

#include "arch/instruction.h"

#include "jit/basic-block.h"
#include "jit/ssa.h"

int peephole_optimize(struct compilation_unit *cu)
{
#ifdef CONFIG_X86_32
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		struct insn *this, *next;

		list_for_each_entry_safe(this, next, &bb->insn_list, insn_list_node) {
			switch (this->type) {
			case INSN_MOV_REG_REG:
				if (mach_reg(&this->src.reg) == mach_reg(&this->dest.reg))
					remove_insn(this);
				break;
			default:
				break;
			}
		}
	}
#endif

	return 0;
}
