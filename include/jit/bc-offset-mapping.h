#ifndef JIT_BC_OFFSET_MAPPING_H
#define JIT_BC_OFFSET_MAPPING_H

#include "jit/compilation-unit.h"
#include "jit/instruction.h"
#include "jit/tree-node.h"

#include "vm/method.h"
#include "vm/die.h"

#include "lib/string.h"
#include <limits.h>

#define BC_OFFSET_UNKNOWN ULONG_MAX

unsigned long jit_lookup_bc_offset(struct compilation_unit *cu,
				   unsigned char *native_ptr);
void print_bytecode_offset(unsigned long bc_offset, struct string *str);
void tree_patch_bc_offset(struct tree_node *node, unsigned long bc_offset);
bool all_insn_have_bytecode_offset(struct compilation_unit *cu);
int bytecode_offset_to_line_no(struct vm_method *mb, unsigned long bc_offset);
int build_bc_offset_map(struct compilation_unit *cu);

static inline void insn_set_bc_offset(struct insn *insn, unsigned long offset)
{
	if (offset != BC_OFFSET_UNKNOWN) {
		if (offset > USHRT_MAX)
			die("bytecode offset is too big");
		insn->flags	|= INSN_FLAG_KNOWN_BC_OFFSET;
		insn->bc_offset	= offset;
	} else {
		insn->flags	&= ~INSN_FLAG_KNOWN_BC_OFFSET;
	}
}

static inline unsigned long insn_get_bc_offset(struct insn *insn)
{
	if (insn->flags & INSN_FLAG_KNOWN_BC_OFFSET)
		return insn->bc_offset;

	return BC_OFFSET_UNKNOWN;
}

#endif /* JIT_BC_OFFSET_MAPPING_H */
