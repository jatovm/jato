#include "arch/inline-cache.h"
#include "arch/instruction.h"

#include "vm/method.h"
#include "vm/class.h"
#include "vm/trace.h"
#include "vm/die.h"

#include "jit/compilation-unit.h"
#include "jit/cu-mapping.h"
#include "jit/instruction.h"
#include "jit/inline-cache.h"
#include "jit/bc-offset-mapping.h"

#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

bool ic_supports_method(struct vm_method *vmm)
{
	assert(!"not implemented");
}

void *emit_ic_check(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_ic_miss_handler(struct buffer *buf, void *ic_check,
				struct vm_method *vmm)
{
	assert(!"not implemented");
}

int convert_ic_calls(struct compilation_unit *cu)
{
	assert(!"not implemented");
}

struct insn *ssa_imm_reg_insn(unsigned long imm, struct var_info *dest_reg)
{
	assert(!"not implemented");
}

int insn_uses_reg(struct insn *insn, struct use_position **reg)
{
	assert(!"not implemented");
}

int insn_use_def_src(struct insn *insn)
{
	assert(!"not implemented");
}

int insn_use_def_dst(struct insn *insn)
{
	assert(!"not implemented");
}

int insn_defs_reg(struct insn *insn, struct use_position **regs)
{
	assert(!"not implemented");
}

struct insn *ssa_reg_reg_insn(struct var_info *src, struct var_info *dest)
{
	assert(!"not implemented");
}

void free_ssa_insn(struct insn *insn)
{
	assert(!"not implemented");
}

struct insn *ssa_phi_insn(struct var_info *var, unsigned long nr_srcs)
{
	assert(!"not implemented");
}

bool insn_is_jmp_mem(struct insn *insn)
{
	assert(!"not implemented");
}

void ssa_chg_jmp_direction(struct insn *insn, struct basic_block *after_bb,
		struct basic_block *new_bb, struct basic_block *bb)
{
	assert(!"not implemented");
}

bool insn_vreg_use(struct insn *insn, struct var_info *var)
{
	assert(!"not implemented");
}

bool insn_vreg_def(struct insn *insn, struct var_info *var)
{
	assert(!"not implemented");
}
