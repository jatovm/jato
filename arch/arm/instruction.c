#include "jit/compilation-unit.h"
#include "jit/instruction.h"
#include "jit/vars.h"

#include <stdlib.h>
#include <string.h>

void free_insn(struct insn *insn)
{
	assert(!"not implemented");
}

int insn_defs(struct compilation_unit *cu, struct insn *insn, struct var_info **defs)
{
	assert(!"not implemented");
}

int insn_uses(struct insn *insn, struct var_info **uses)
{
	assert(!"not implemented");
}

int insn_operand_use_kind(struct insn *insn, struct operand *operand)
{
	assert(!"not implemented");
}

bool insn_is_branch(struct insn *insn)
{
	assert(!"not implemented");
}

bool insn_is_call(struct insn *insn)
{
	assert(!"not implemented");
}
