/*
 * Copyright (c) 2007, 2009 Pekka Enberg
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
 */

#include "jit/compilation-unit.h"
#include "jit/instruction.h"
#include "jit/vars.h"

enum {
	DEF_DST			= (1U <<  1),
	DEF_SRC			= (1U <<  2),
	DEF_NONE		= (1U <<  3),
	DEF_xAX			= (1U <<  4),
	DEF_xCX			= (1U <<  5),
	DEF_xDX			= (1U <<  6),
	USE_DST			= (1U <<  7),
	USE_IDX_DST		= (1U <<  8), 	/* destination operand is memindex */
	USE_IDX_SRC		= (1U <<  9), 	/* source operand is memindex */
	USE_NONE		= (1U << 10),
	USE_SRC			= (1U << 11),
	USE_FP			= (1U << 12), 	/* frame pointer */

#ifdef CONFIG_X86_32
	DEF_EAX			= DEF_xAX,
	DEF_ECX			= DEF_xCX,
	DEF_EDX			= DEF_xDX,
#else
	DEF_RAX			= DEF_xAX,
	DEF_RCX			= DEF_xCX,
	DEF_RDX			= DEF_xDX,
#endif
};

static unsigned long insn_flags[] = {
	[INSN_ADC_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_ADC_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_ADC_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_ADD_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_ADD_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_ADD_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_AND_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_AND_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_CALL_REG]				= USE_SRC | DEF_NONE,
	[INSN_CALL_REL]				= USE_NONE | DEF_NONE,
	[INSN_CLTD_REG_REG]			= USE_SRC | DEF_SRC | DEF_DST,
	[INSN_CMP_IMM_REG]			= USE_DST,
	[INSN_CMP_MEMBASE_REG]			= USE_SRC | USE_DST,
	[INSN_CMP_REG_REG]			= USE_SRC | USE_DST,
	[INSN_CONV_FPU64_TO_GPR]		= USE_SRC | DEF_DST,
	[INSN_CONV_FPU_TO_GPR]			= USE_SRC | DEF_DST,
	[INSN_CONV_GPR_TO_FPU64]		= USE_SRC | DEF_DST,
	[INSN_CONV_GPR_TO_FPU]			= USE_SRC | DEF_DST,
	[INSN_CONV_XMM64_TO_XMM]		= USE_SRC | DEF_DST,
	[INSN_CONV_XMM_TO_XMM64]		= USE_SRC | DEF_DST,
	[INSN_DIV_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST | DEF_xAX | DEF_xDX,
	[INSN_DIV_REG_REG]			= USE_SRC | USE_DST | DEF_DST | DEF_xAX | DEF_xDX,
	[INSN_FADD_64_MEMDISP_REG]		= USE_DST | DEF_DST,
	[INSN_FADD_64_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_FADD_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_FDIV_64_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_FDIV_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_FILD_64_MEMBASE]			= USE_SRC,
	[INSN_FISTP_64_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FLDCW_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FLD_64_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FLD_64_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_FLD_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FLD_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_FMUL_64_MEMDISP_REG]		= USE_DST | DEF_DST,
	[INSN_FMUL_64_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_FMUL_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_FNSTCW_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FSTP_64_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FSTP_64_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_FSTP_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FSTP_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_FSUB_64_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_FSUB_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_JE_BRANCH]			= USE_NONE | DEF_NONE,
	[INSN_JGE_BRANCH]			= USE_NONE | DEF_NONE,
	[INSN_JG_BRANCH]			= USE_NONE | DEF_NONE,
	[INSN_JLE_BRANCH]			= USE_NONE | DEF_NONE,
	[INSN_JL_BRANCH]			= USE_NONE | DEF_NONE,
	[INSN_JMP_BRANCH]			= USE_NONE | DEF_NONE,
	[INSN_JMP_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_JMP_MEMINDEX]			= USE_IDX_SRC | USE_SRC | DEF_NONE,
	[INSN_JNE_BRANCH]			= USE_NONE | DEF_NONE,
	[INSN_MOVSX_16_MEMBASE_REG]		= USE_SRC | DEF_DST,
	[INSN_MOVSX_16_REG_REG]			= USE_SRC | DEF_DST,
	[INSN_MOVSX_8_MEMBASE_REG]		= USE_SRC | DEF_DST,
	[INSN_MOVSX_8_REG_REG]			= USE_SRC | DEF_DST,
	[INSN_MOVZX_16_REG_REG]			= USE_SRC | DEF_DST,
	[INSN_MOV_64_MEMBASE_XMM]		= USE_SRC | DEF_DST,
	[INSN_MOV_64_MEMDISP_XMM]		= USE_NONE | DEF_DST,
	[INSN_MOV_64_MEMINDEX_XMM]		= USE_SRC | USE_IDX_SRC | DEF_DST,
	[INSN_MOV_64_MEMLOCAL_XMM]		= USE_FP | DEF_DST,
	[INSN_MOV_64_XMM_MEMBASE]		= USE_SRC | USE_DST,
	[INSN_MOV_64_XMM_MEMDISP]		= USE_SRC | DEF_NONE,
	[INSN_MOV_64_XMM_MEMINDEX]		= USE_SRC | USE_DST | USE_IDX_DST | DEF_NONE,
	[INSN_MOV_64_XMM_MEMLOCAL]		= USE_SRC,
	[INSN_MOV_64_XMM_XMM]			= USE_SRC | DEF_DST,
	[INSN_MOV_IMM_MEMBASE]			= USE_DST,
	[INSN_MOV_IMM_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_MOV_IMM_REG]			= DEF_DST,
	[INSN_MOV_IMM_THREAD_LOCAL_MEMBASE]	= USE_DST | DEF_NONE,
	[INSN_MOV_IP_REG]			= DEF_DST,
	[INSN_MOV_IP_THREAD_LOCAL_MEMBASE]	= USE_NONE | DEF_NONE,
	[INSN_MOV_MEMBASE_REG]			= USE_SRC | DEF_DST,
	[INSN_MOV_MEMBASE_XMM]			= USE_SRC | DEF_DST,
	[INSN_MOV_MEMDISP_REG]			= USE_NONE | DEF_DST,
	[INSN_MOV_MEMDISP_XMM]			= USE_NONE | DEF_DST,
	[INSN_MOV_MEMINDEX_REG]			= USE_SRC | USE_IDX_SRC | DEF_DST,
	[INSN_MOV_MEMINDEX_XMM]			= USE_SRC | USE_IDX_SRC | DEF_DST,
	[INSN_MOV_MEMLOCAL_REG]			= USE_FP | DEF_DST,
	[INSN_MOV_MEMLOCAL_XMM]			= USE_FP | DEF_DST,
	[INSN_MOV_REG_MEMBASE]			= USE_SRC | USE_DST,
	[INSN_MOV_REG_MEMDISP]			= USE_SRC | DEF_NONE,
	[INSN_MOV_REG_MEMINDEX]			= USE_SRC | USE_DST | USE_IDX_DST | DEF_NONE,
	[INSN_MOV_REG_MEMLOCAL]			= USE_SRC,
	[INSN_MOV_REG_REG]			= USE_SRC | DEF_DST,
	[INSN_MOV_REG_THREAD_LOCAL_MEMBASE]	= USE_SRC | USE_DST | DEF_NONE,
	[INSN_MOV_REG_THREAD_LOCAL_MEMDISP]	= USE_SRC | DEF_NONE,
	[INSN_MOV_THREAD_LOCAL_MEMDISP_REG]	= USE_NONE | DEF_DST,
	[INSN_MOV_XMM_MEMBASE]			= USE_SRC | USE_DST,
	[INSN_MOV_XMM_MEMDISP]			= USE_SRC | DEF_NONE,
	[INSN_MOV_XMM_MEMINDEX]			= USE_SRC | USE_DST | USE_IDX_DST | DEF_NONE,
	[INSN_MOV_XMM_MEMLOCAL]			= USE_SRC,
	[INSN_MOV_XMM_XMM]			= USE_SRC | DEF_DST,
	[INSN_MUL_MEMBASE_EAX]			= USE_SRC | DEF_DST | DEF_xDX | DEF_xAX,
	[INSN_MUL_REG_EAX]			= USE_SRC | USE_DST | DEF_DST | DEF_xDX | DEF_xAX,
	[INSN_MUL_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_NEG_REG]				= USE_SRC | DEF_SRC,
	[INSN_OR_IMM_MEMBASE]			= USE_DST | DEF_NONE,
	[INSN_OR_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_OR_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_POP_MEMLOCAL]			= USE_SRC | DEF_NONE,
	[INSN_POP_REG]				= USE_NONE | DEF_SRC,
	[INSN_PUSH_IMM]				= USE_NONE | DEF_NONE,
	[INSN_PUSH_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_PUSH_MEMLOCAL]			= USE_SRC | DEF_NONE,
	[INSN_PUSH_REG]				= USE_SRC | DEF_NONE,
	[INSN_RET]				= USE_NONE | DEF_NONE,
	[INSN_SAR_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_SAR_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SBB_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_SBB_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SBB_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SHL_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SHR_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SUB_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_SUB_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SUB_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_TEST_IMM_MEMDISP]			= USE_NONE | DEF_NONE,
	[INSN_TEST_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_NONE,
	[INSN_XOR_64_XMM_REG_REG]		= USE_SRC | USE_DST | DEF_DST,
	[INSN_XOR_IMM_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_XOR_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_XOR_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_XOR_XMM_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
};

void insn_sanity_check(void)
{
	assert(ARRAY_SIZE(insn_flags) == NR_INSN_TYPES);

	for (unsigned int i = 0; i < NR_INSN_TYPES; ++i) {
		if (insn_flags[i] == 0)
			die("missing insn_info for %d", i);
	}
}

struct mach_reg_def {
	enum machine_reg reg;
	int def;
};

static struct mach_reg_def checkregs[] = {
	{ MACH_REG_xAX, DEF_xAX },
	{ MACH_REG_xCX, DEF_xCX },
	{ MACH_REG_xDX, DEF_xDX },
};

int insn_defs(struct compilation_unit *cu, struct insn *insn, struct var_info **defs)
{
	unsigned long flags;
	int nr = 0;

	flags = insn_flags[insn->type];

	if (flags & DEF_SRC)
		defs[nr++] = insn->src.reg.interval->var_info;

	if (flags & DEF_DST)
		defs[nr++] = insn->dest.reg.interval->var_info;

	for (unsigned int i = 0; i < ARRAY_SIZE(checkregs); i++) {
		if (flags & checkregs[i].def)
			defs[nr++] = cu->fixed_var_infos[checkregs[i].reg];
	}
	return nr;
}

int insn_uses(struct insn *insn, struct var_info **uses)
{
	unsigned long flags;
	int nr = 0;

	flags = insn_flags[insn->type];

	if (flags & USE_SRC)
		uses[nr++] = insn->src.reg.interval->var_info;

	if (flags & USE_DST)
		uses[nr++] = insn->dest.reg.interval->var_info;

	if (flags & USE_IDX_SRC)
		uses[nr++] = insn->src.index_reg.interval->var_info;

	if (flags & USE_IDX_DST)
		uses[nr++] = insn->dest.index_reg.interval->var_info;

	return nr;
}

int insn_operand_use_kind(struct insn *insn, int idx)
{
	unsigned long flags;
	int kind_mask;
	int use_mask;
	int def_mask;

	flags = insn_flags[insn->type];

	if (idx == 0) {
		use_mask = USE_SRC;
		def_mask = DEF_SRC;
	} else {
		assert(idx == 1);
		use_mask = USE_DST;
		def_mask = DEF_DST;
	}

	kind_mask = 0;
	if (flags & use_mask)
		kind_mask |= USE_KIND_INPUT;

	if (flags & def_mask)
		kind_mask |= USE_KIND_OUTPUT;

	return kind_mask;
}
