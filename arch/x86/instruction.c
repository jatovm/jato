/*
 * Copyright (c) 2006-2009  Pekka Enberg
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

#include "jit/bc-offset-mapping.h"
#include "jit/compilation-unit.h"
#include "jit/instruction.h"
#include "jit/vars.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct insn *alloc_insn(enum insn_type type)
{
	struct insn *insn = malloc(sizeof *insn);
	if (insn) {
		memset(insn, 0, sizeof *insn);
		INIT_LIST_HEAD(&insn->insn_list_node);
		INIT_LIST_HEAD(&insn->branch_list_node);
		insn->type = type;
	}
	return insn;
}

static bool operand_uses_reg(struct operand *operand)
{
	return operand->type == OPERAND_REG
		|| operand->type == OPERAND_MEMBASE
		|| operand->type == OPERAND_MEMINDEX;
}

static bool operand_uses_index_reg(struct operand *operand)
{
	return operand->type == OPERAND_MEMINDEX;
}

static void release_operand(struct operand *operand)
{
	if (operand_uses_reg(operand))
		list_del(&operand->reg.use_pos_list);

	if (operand_uses_index_reg(operand))
		list_del(&operand->index_reg.use_pos_list);
}

void free_insn(struct insn *insn)
{
	release_operand(&insn->src);
	release_operand(&insn->dest);

	free(insn);
}

void free_ssa_insn(struct insn *insn)
{
	unsigned long ndx;
	release_operand(&insn->ssa_dest);

	for (ndx = 0; ndx < insn->nr_srcs; ndx++)
		release_operand(&insn->ssa_srcs[ndx]);

	free(insn);
}

static void init_membase_operand(struct insn *insn, struct operand *operand,
				 struct var_info *base_reg, unsigned long disp)
{
	operand->type = OPERAND_MEMBASE;
	operand->disp = disp;

	init_register(&operand->base_reg, insn, base_reg->interval);
	operand->base_reg.kind = USE_KIND_INPUT;
}

static void init_memindex_operand(struct insn *insn, struct operand *operand,
				  struct var_info *base_reg,
				  struct var_info *index_reg, unsigned long shift)
{
	operand->type = OPERAND_MEMINDEX;
	operand->shift = shift;

	init_register(&operand->base_reg, insn, base_reg->interval);
	init_register(&operand->index_reg, insn, index_reg->interval);

	operand->base_reg.kind  = USE_KIND_INPUT;
	operand->index_reg.kind = USE_KIND_INPUT;
}

static void init_reg_operand(struct insn *insn, struct operand *operand, struct var_info *reg)
{
	operand->type = OPERAND_REG;

	init_register(&operand->reg, insn, reg->interval);
	operand->reg.kind = insn_operand_use_kind(insn, operand);
}

static void init_phi_reg_operand(struct insn *insn, struct operand *operand, struct var_info *reg)
{
	operand->type = OPERAND_REG;

	init_register(&operand->reg, insn, reg->interval);
	if (operand == &insn->ssa_dest)
		operand->reg.kind = USE_KIND_OUTPUT;
	else operand->reg.kind = USE_KIND_INPUT;
}

struct insn *insn(enum insn_type insn_type)
{
	return alloc_insn(insn_type);
}

struct insn *memlocal_reg_insn(enum insn_type insn_type,
			    struct stack_slot *src_slot,
			    struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->src	= (struct operand) {
			.type		= OPERAND_MEMLOCAL,
			{
				.slot		= src_slot,
			}
		};
		init_reg_operand(insn, &insn->dest, dest_reg);
	}
	return insn;
}

struct insn *membase_reg_insn(enum insn_type insn_type, struct var_info *src_base_reg,
			      long src_disp, struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		init_membase_operand(insn, &insn->src, src_base_reg, src_disp);
		init_reg_operand(insn, &insn->dest, dest_reg);
	}
	return insn;
}

struct insn *memindex_insn(enum insn_type insn_type, struct var_info *src_base_reg, struct var_info *src_index_reg, unsigned char src_shift)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn)
		init_memindex_operand(insn, &insn->operand, src_base_reg, src_index_reg, src_shift);

	return insn;
}

struct insn *reverse_memindex_insn(enum insn_type insn_type, struct var_info *dst_base_reg, struct var_info *dst_index_reg, unsigned char dst_shift)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn)
		init_memindex_operand(insn, &insn->dest, dst_base_reg, dst_index_reg, dst_shift);

	return insn;
}

struct insn *memindex_reg_insn(enum insn_type insn_type,
			       struct var_info *src_base_reg, struct var_info *src_index_reg,
			       unsigned char src_shift, struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		init_memindex_operand(insn, &insn->src, src_base_reg, src_index_reg, src_shift);
		init_reg_operand(insn, &insn->dest, dest_reg);
	}
	return insn;
}

struct insn *
reg_membase_insn(enum insn_type insn_type, struct var_info *src_reg,
		 struct var_info *dest_base_reg, long dest_disp)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		init_reg_operand(insn, &insn->src, src_reg);
		init_membase_operand(insn, &insn->dest, dest_base_reg, dest_disp);
	}
	return insn;
}

struct insn *
reg_memlocal_insn(enum insn_type insn_type, struct var_info *src_reg,
		  struct stack_slot *dest_slot)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		init_reg_operand(insn, &insn->src, src_reg);
		insn->dest	= (struct operand) {
			.type		= OPERAND_MEMLOCAL,
			{
				.slot		= dest_slot,
			}
		};
	}
	return insn;
}

struct insn *reg_memindex_insn(enum insn_type insn_type,
			       struct var_info *src_reg,
			       struct var_info *dest_base_reg,
			       struct var_info *dest_index_reg,
			       unsigned char dest_shift)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		init_reg_operand(insn, &insn->src, src_reg);
		init_memindex_operand(insn, &insn->dest, dest_base_reg, dest_index_reg, dest_shift);
	}
	return insn;
}

struct insn *imm_reg_insn(enum insn_type insn_type, unsigned long imm,
			  struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->src	= (struct operand) {
			.type		= OPERAND_IMM,
			{
				.imm		= imm,
			}
		};
		init_reg_operand(insn, &insn->dest, dest_reg);
	}
	return insn;
}

struct insn *memdisp_reg_insn(enum insn_type insn_type, unsigned long imm,
			      struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->src	= (struct operand) {
			.type		= OPERAND_IMM,
			{
				.imm		= imm,
			}
		};
		init_reg_operand(insn, &insn->dest, dest_reg);
	}
	return insn;
}

struct insn *reg_memdisp_insn(enum insn_type insn_type,
			      struct var_info *src_reg, unsigned long imm)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		init_reg_operand(insn, &insn->src, src_reg);
		insn->dest	= (struct operand) {
			.type		= OPERAND_IMM,
			{
				.imm		= imm,
			}
		};
	}
	return insn;
}

struct insn *imm_memdisp_insn(enum insn_type insn_type, long imm, long disp)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->src	= (struct operand) {
			.type		= OPERAND_IMM,
			{
				.imm		= imm,
			}
		};
		insn->dest	= (struct operand) {
			.type		= OPERAND_MEMDISP,
		};
		insn->dest.disp		= disp;
	}
	return insn;
}

struct insn *imm_membase_insn(enum insn_type insn_type, unsigned long imm,
			      struct var_info *base_reg, long disp)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->src	= (struct operand) {
			.type		= OPERAND_IMM,
			{
				.imm		= imm,
			}
		};
		init_membase_operand(insn, &insn->dest, base_reg, disp);
	}
	return insn;
}

struct insn *reg_insn(enum insn_type insn_type, struct var_info *reg)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn)
		init_reg_operand(insn, &insn->src, reg);

	return insn;
}

struct insn *reverse_reg_insn(enum insn_type insn_type, struct var_info *reg)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn)
		init_reg_operand(insn, &insn->dest, reg);

	return insn;
}

struct insn *reg_reg_insn(enum insn_type insn_type, struct var_info *src, struct var_info *dest)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		init_reg_operand(insn, &insn->src, src);
		init_reg_operand(insn, &insn->dest, dest);
	}
	return insn;
}

struct insn *imm_insn(enum insn_type insn_type, unsigned long imm)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->operand	= (struct operand) {
			.type		= OPERAND_IMM,
			{
				.imm		= imm,
			}
		};
	}
	return insn;
}

struct insn *rel_insn(enum insn_type insn_type, unsigned long rel)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->operand	= (struct operand) {
			.type		= OPERAND_REL,
			{
				.rel		= rel,
			}
		};
	}
	return insn;
}

struct insn *phi_insn(enum insn_type insn_type, struct var_info *var, unsigned long nr_srcs)
{
	struct insn *insn = alloc_insn(insn_type);
	unsigned long ndx;

	if (insn) {
		init_phi_reg_operand(insn, &insn->ssa_dest, var);

		insn->ssa_srcs = malloc(nr_srcs * sizeof(struct operand));
		for (ndx = 0; ndx < nr_srcs; ndx++)
			init_phi_reg_operand(insn, &insn->ssa_srcs[ndx], var);

		insn->nr_srcs = nr_srcs;
	}
	return insn;
}

struct insn *branch_insn(enum insn_type insn_type, struct basic_block *if_true)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->operand	= (struct operand) {
			.type		= OPERAND_BRANCH,
			{
				.branch_target	= if_true,
			}
		};
	}
	return insn;
}

struct insn *memlocal_insn(enum insn_type insn_type, struct stack_slot *slot)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->src	= (struct operand) {
			.type		= OPERAND_MEMLOCAL,
			{
				.slot		= slot,
			}
		};
	}

	return insn;
}

struct insn *membase_insn(enum insn_type insn_type, struct var_info *src_base_reg, long src_disp)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn)
		init_membase_operand(insn, &insn->operand, src_base_reg, src_disp);

	return insn;
}

struct insn *reverse_membase_insn(enum insn_type insn_type, struct var_info *dst_base_reg, long dst_disp)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn)
		init_membase_operand(insn, &insn->dest, dst_base_reg, dst_disp);

	return insn;
}

struct insn *imm_memlocal_insn(enum insn_type insn_type,
			       unsigned long imm,
			       struct stack_slot *dst_slot)
{
	struct insn *insn = alloc_insn(insn_type);

	if (insn) {
		insn->src	= (struct operand) {
			.type		= OPERAND_IMM,
			{
				.imm		= imm,
			}
		};
		insn->dest	= (struct operand) {
			.type		= OPERAND_MEMLOCAL,
			{
				.slot		= dst_slot,
			}
		};
	}
	return insn;
}

struct insn *ic_call_insn(struct var_info *this, unsigned long imm)
{
	struct insn *insn = alloc_insn(INSN_IC_CALL);

	if (insn) {
		init_reg_operand(insn, &insn->src, this);
		assert(insn_operand_use_kind(insn, &insn->src) == USE_KIND_INPUT);
		insn->dest	= (struct operand) {
			.type		= OPERAND_IMM,
			{
				.imm		= imm,
			}
		};
	}

	return insn;
}

int insert_copy_slot_32_insns(struct stack_slot *from, struct stack_slot *to,
			      struct list_head *add_before, unsigned long bc_offset)
{
	struct insn *push;
	struct insn *pop;

	assert(from);
	assert(to);

	push = memlocal_insn(INSN_PUSH_MEMLOCAL, from);
	if (!push)
		return -1;

	pop = memlocal_insn(INSN_POP_MEMLOCAL, to);
	if (!pop) {
		free_insn(push);
		return -1;
	}

	insn_set_bc_offset(push, bc_offset);
	insn_set_bc_offset(pop, bc_offset);

	list_add_tail(&push->insn_list_node, add_before);
	list_add(&pop->insn_list_node, &push->insn_list_node);
	return 0;
}

#ifdef CONFIG_X86_32

int insert_copy_slot_64_insns(struct stack_slot *from, struct stack_slot *to,
			      struct list_head *add_before, unsigned long bc_offset)
{
	struct insn *push_lo, *push_hi;
	struct insn *pop_lo, *pop_hi;

	assert(from);
	assert(to);

	push_hi = memlocal_insn(INSN_PUSH_MEMLOCAL, from);
	if (!push_hi)
		goto fail_push_hi;

	push_lo = memlocal_insn(INSN_PUSH_MEMLOCAL, from->next);
	if (!push_lo)
		goto fail_push_lo;

	pop_hi = memlocal_insn(INSN_POP_MEMLOCAL, to);
	if (!pop_hi)
		goto fail_pop_hi;

	pop_lo = memlocal_insn(INSN_POP_MEMLOCAL, to->next);
	if (!pop_lo)
		goto fail_pop_lo;

	insn_set_bc_offset(push_lo, bc_offset);
	insn_set_bc_offset(push_hi, bc_offset);
	insn_set_bc_offset(pop_lo, bc_offset);
	insn_set_bc_offset(pop_hi, bc_offset);

	list_add_tail(&push_lo->insn_list_node, add_before);
	list_add(&push_hi->insn_list_node, &push_lo->insn_list_node);
	list_add(&pop_hi->insn_list_node, &push_hi->insn_list_node);
	list_add(&pop_lo->insn_list_node, &pop_hi->insn_list_node);

	return 0;

 fail_pop_lo:
	free_insn(pop_hi);
 fail_pop_hi:
	free_insn(push_lo);
 fail_push_lo:
	free_insn(push_hi);
 fail_push_hi:
	return -1;
}

#else

int insert_copy_slot_64_insns(struct stack_slot *from, struct stack_slot *to,
			      struct list_head *add_before, unsigned long bc_offset)
{
	return insert_copy_slot_32_insns(from, to, add_before, bc_offset);
}

#endif  /* CONFIG_X86_32 */

struct insn *ssa_reg_reg_insn(struct var_info *src, struct var_info *dest)
{
	struct insn *insn;

	switch(src->vm_type) {
		case J_FLOAT:
			insn = reg_reg_insn(INSN_MOVSS_XMM_XMM, src, dest);
			break;
		case J_DOUBLE:
			insn = reg_reg_insn(INSN_MOVSD_XMM_XMM, src, dest);
			break;
		default:
			insn = reg_reg_insn(INSN_MOV_REG_REG, src, dest);
	}

	return insn;
}

struct insn *ssa_imm_reg_insn(unsigned long imm,
			struct var_info *dest_reg,
			struct var_info *gpr,
			struct insn **insn_conv)
{
	struct insn *insn;

	switch(dest_reg->vm_type) {
		case J_FLOAT:
			insn = imm_reg_insn(INSN_MOV_IMM_REG, imm, gpr);
			*insn_conv = reg_reg_insn(INSN_CONV_GPR_TO_FPU, gpr, dest_reg);
			break;
		case J_DOUBLE:
			insn = imm_reg_insn(INSN_MOV_IMM_REG, imm, gpr);
			*insn_conv = reg_reg_insn(INSN_CONV_GPR_TO_FPU64, gpr, dest_reg);
			break;
		default:
			insn = imm_reg_insn(INSN_MOV_IMM_REG, imm, dest_reg);
			*insn_conv = NULL;
	}

	return insn;
}

struct insn *ssa_phi_insn(struct var_info *var, unsigned long nr_srcs)
{
	return phi_insn(INSN_PHI, var, nr_srcs);
}

struct insn *spill_insn(struct var_info *var, struct stack_slot *slot)
{
	enum insn_type insn_type;

	assert(slot != NULL);

	if (var->vm_type == J_FLOAT) {
		insn_type = INSN_MOVSS_XMM_MEMLOCAL;
	} else if (var->vm_type == J_DOUBLE) {
		if (cpu_has(X86_FEATURE_SSE2))
			insn_type = INSN_MOVSD_XMM_MEMLOCAL;
		else
			error("not implemented");
	} else {
		insn_type = INSN_MOV_REG_MEMLOCAL;
	}

	return reg_memlocal_insn(insn_type, var, slot);
}

struct insn *reload_insn(struct stack_slot *slot, struct var_info *var)
{
	enum insn_type insn_type;

	assert(slot != NULL);

	if (var->vm_type == J_FLOAT) {
		insn_type = INSN_MOVSS_MEMLOCAL_XMM;
	} else if (var->vm_type == J_DOUBLE) {
		if (cpu_has(X86_FEATURE_SSE2))
			insn_type = INSN_MOVSD_MEMLOCAL_XMM;
		else
			error("not implemented");
	} else {
		insn_type = INSN_MOV_MEMLOCAL_REG;
	}

	return memlocal_reg_insn(insn_type, slot, var);
}

struct insn *jump_insn(struct basic_block *bb)
{
	return branch_insn(INSN_JMP_BRANCH, bb);
}

void ssa_chg_jmp_direction(struct insn *insn, struct basic_block *after_bb,
		struct basic_block *new_bb, struct basic_block *bb)
{
	switch(insn->type) {
		case INSN_JE_BRANCH:
			if (insn->operand.branch_target == bb) {
				insn->operand.branch_target = after_bb;
				insn->type = INSN_JNE_BRANCH;
			}
			break;
		case INSN_JNE_BRANCH:
			if (insn->operand.branch_target == bb) {
				insn->operand.branch_target = after_bb;
				insn->type = INSN_JE_BRANCH;
			}
			break;
		case INSN_JGE_BRANCH:
			if (insn->operand.branch_target == bb) {
				insn->operand.branch_target = after_bb;
				insn->type = INSN_JL_BRANCH;
			}
			break;
		case INSN_JL_BRANCH:
			if (insn->operand.branch_target == bb) {
				insn->operand.branch_target = after_bb;
				insn->type = INSN_JGE_BRANCH;
			}
			break;
		case INSN_JG_BRANCH:
			if (insn->operand.branch_target == bb) {
				insn->operand.branch_target = after_bb;
				insn->type = INSN_JLE_BRANCH;
			}
			break;
		case INSN_JLE_BRANCH:
			if (insn->operand.branch_target == bb) {
				insn->operand.branch_target = after_bb;
				insn->type = INSN_JG_BRANCH;
			}
			break;
		case INSN_JMP_BRANCH:
			if (insn->operand.branch_target == bb)
				insn->operand.branch_target = new_bb;
			break;
	}
}

/*
 *	Instruction flags
 */
enum {
	DEF_DST			= (1U <<  1),
	DEF_SRC			= (1U <<  2),
	DEF_NONE		= (1U <<  3),
	DEF_xAX			= (1U <<  4),
	DEF_xCX			= (1U <<  5),
	DEF_xDX			= (1U <<  6),
	USE_DST			= (1U <<  7),
	USE_IDX_DST		= (1U <<  8),	/* destination operand is memindex */
	USE_IDX_SRC		= (1U <<  9),	/* source operand is memindex */
	USE_NONE		= (1U << 10),
	USE_SRC			= (1U << 11),
	USE_FP			= (1U << 12),	/* frame pointer */
	TYPE_BRANCH		= (1U << 13),
	TYPE_CALL		= (1U << 14),

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
	[INSN_ADDSD_MEMDISP_XMM]		= USE_DST | DEF_DST,
	[INSN_ADDSD_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_ADDSS_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_ADD_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_ADD_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_ADD_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_AND_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_AND_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_CALL_REG]				= USE_DST | DEF_NONE | TYPE_CALL,
	[INSN_CALL_REL]				= USE_NONE | DEF_NONE | TYPE_CALL,
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
	[INSN_DIVSD_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_DIVSS_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_DIV_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST | DEF_xAX | DEF_xDX,
	[INSN_DIV_REG_REG]			= USE_SRC | USE_DST | DEF_DST | DEF_xAX | DEF_xDX,
	[INSN_FILD_64_MEMBASE]			= USE_SRC,
	[INSN_FISTP_64_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FLDCW_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FLD_64_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FLD_64_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_FLD_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FLD_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_FNSTCW_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FSTP_64_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FSTP_64_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_FSTP_MEMBASE]			= USE_SRC | DEF_NONE,
	[INSN_FSTP_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_IC_CALL]				= USE_SRC | DEF_xAX | DEF_xCX | TYPE_CALL,
	[INSN_JE_BRANCH]			= USE_NONE | DEF_NONE | TYPE_BRANCH,
	[INSN_JGE_BRANCH]			= USE_NONE | DEF_NONE | TYPE_BRANCH,
	[INSN_JG_BRANCH]			= USE_NONE | DEF_NONE | TYPE_BRANCH,
	[INSN_JLE_BRANCH]			= USE_NONE | DEF_NONE | TYPE_BRANCH,
	[INSN_JL_BRANCH]			= USE_NONE | DEF_NONE | TYPE_BRANCH,
	[INSN_JMP_BRANCH]			= USE_NONE | DEF_NONE | TYPE_BRANCH,
	[INSN_JMP_MEMBASE]			= USE_DST | DEF_NONE | TYPE_BRANCH,
	[INSN_JMP_MEMINDEX]			= USE_IDX_DST | USE_DST | DEF_NONE | TYPE_BRANCH,
	[INSN_JNE_BRANCH]			= USE_NONE | DEF_NONE | TYPE_BRANCH,
	[INSN_MOVSD_MEMBASE_XMM]		= USE_SRC | DEF_DST,
	[INSN_MOVSD_MEMDISP_XMM]		= USE_NONE | DEF_DST,
	[INSN_MOVSD_MEMINDEX_XMM]		= USE_SRC | USE_IDX_SRC | DEF_DST,
	[INSN_MOVSD_MEMLOCAL_XMM]		= USE_FP | DEF_DST,
	[INSN_MOVSD_XMM_MEMBASE]		= USE_SRC | USE_DST,
	[INSN_MOVSD_XMM_MEMDISP]		= USE_SRC | DEF_NONE,
	[INSN_MOVSD_XMM_MEMINDEX]		= USE_SRC | USE_DST | USE_IDX_DST | DEF_NONE,
	[INSN_MOVSD_XMM_MEMLOCAL]		= USE_SRC,
	[INSN_MOVSD_XMM_XMM]			= USE_SRC | DEF_DST,
	[INSN_MOVSS_MEMBASE_XMM]		= USE_SRC | DEF_DST,
	[INSN_MOVSS_MEMDISP_XMM]		= USE_NONE | DEF_DST,
	[INSN_MOVSS_MEMINDEX_XMM]		= USE_SRC | USE_IDX_SRC | DEF_DST,
	[INSN_MOVSS_MEMLOCAL_XMM]		= USE_FP | DEF_DST,
	[INSN_MOVSS_XMM_MEMBASE]		= USE_SRC | USE_DST,
	[INSN_MOVSS_XMM_MEMDISP]		= USE_SRC | DEF_NONE,
	[INSN_MOVSS_XMM_MEMINDEX]		= USE_SRC | USE_DST | USE_IDX_DST | DEF_NONE,
	[INSN_MOVSS_XMM_MEMLOCAL]		= USE_SRC,
	[INSN_MOVSS_XMM_XMM]			= USE_SRC | DEF_DST,
	[INSN_MOVSX_16_MEMBASE_REG]		= USE_SRC | DEF_DST,
	[INSN_MOVSX_16_REG_REG]			= USE_SRC | DEF_DST,
	[INSN_MOVSX_8_MEMBASE_REG]		= USE_SRC | DEF_DST,
	[INSN_MOVSX_8_REG_REG]			= USE_SRC | DEF_DST,
	[INSN_MOVZX_16_REG_REG]			= USE_SRC | DEF_DST,
	[INSN_MOV_IMM_MEMBASE]			= USE_DST,
	[INSN_MOV_IMM_MEMLOCAL]			= USE_FP | DEF_NONE,
	[INSN_MOV_IMM_REG]			= DEF_DST,
	[INSN_MOV_IMM_THREAD_LOCAL_MEMBASE]	= USE_DST | DEF_NONE,
	[INSN_MOV_MEMBASE_REG]			= USE_SRC | DEF_DST,
	[INSN_MOV_MEMDISP_REG]			= USE_NONE | DEF_DST,
	[INSN_MOV_MEMINDEX_REG]			= USE_SRC | USE_IDX_SRC | DEF_DST,
	[INSN_MOV_MEMLOCAL_REG]			= USE_FP | DEF_DST,
	[INSN_MOV_REG_MEMBASE]			= USE_SRC | USE_DST,
	[INSN_MOV_REG_MEMDISP]			= USE_SRC | DEF_NONE,
	[INSN_MOV_REG_MEMINDEX]			= USE_SRC | USE_DST | USE_IDX_DST | DEF_NONE,
	[INSN_MOV_REG_MEMLOCAL]			= USE_SRC,
	[INSN_MOV_REG_REG]			= USE_SRC | DEF_DST,
	[INSN_MOV_REG_THREAD_LOCAL_MEMBASE]	= USE_SRC | USE_DST | DEF_NONE,
	[INSN_MOV_REG_THREAD_LOCAL_MEMDISP]	= USE_SRC | DEF_NONE,
	[INSN_MOV_THREAD_LOCAL_MEMDISP_REG]	= USE_NONE | DEF_DST,
	[INSN_MULSD_MEMDISP_XMM]		= USE_DST | DEF_DST,
	[INSN_MULSD_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_MULSS_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_MUL_MEMBASE_EAX]			= USE_SRC | DEF_DST | DEF_xDX | DEF_xAX,
	[INSN_MUL_REG_EAX]			= USE_SRC | USE_DST | DEF_DST | DEF_xDX | DEF_xAX,
	[INSN_MUL_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_NEG_REG]				= USE_DST | DEF_DST,
	[INSN_OR_IMM_MEMBASE]			= USE_DST | DEF_NONE,
	[INSN_OR_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_OR_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_PHI]				= USE_SRC | DEF_DST,
	[INSN_POP_MEMLOCAL]			= USE_SRC | DEF_NONE,
	[INSN_POP_REG]				= USE_NONE | DEF_DST,
	[INSN_PUSH_IMM]				= USE_NONE | DEF_NONE,
	[INSN_PUSH_MEMLOCAL]			= USE_SRC | DEF_NONE,
	[INSN_PUSH_REG]				= USE_SRC | DEF_NONE,
	[INSN_RET]				= USE_NONE | DEF_NONE | TYPE_BRANCH,
	[INSN_SAR_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_SAR_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SBB_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_SBB_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SBB_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SHL_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SHR_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SUBSD_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SUBSS_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SUB_IMM_REG]			= USE_DST | DEF_DST,
	[INSN_SUB_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_SUB_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_TEST_IMM_MEMDISP]			= USE_NONE | DEF_NONE,
	[INSN_TEST_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_NONE,
	[INSN_XORPD_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_XOR_MEMBASE_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_XOR_REG_REG]			= USE_SRC | USE_DST | DEF_DST,
	[INSN_XORPS_XMM_XMM]			= USE_SRC | USE_DST | DEF_DST,
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

int insn_use_def(struct insn *insn)
{
	if (insn_use_def_dst(insn))
		return 1;

	if (insn_use_def_src(insn))
		return 1;

	return 0;
}

int insn_use_def_dst(struct insn *insn)
{
	unsigned long flags;

	flags = insn_flags[insn->type];

	if ((flags & DEF_DST) && (flags & USE_DST))
		return 1;

	return 0;
}

int insn_use_def_src(struct insn *insn)
{
	unsigned long flags;

	flags = insn_flags[insn->type];

	if ((flags & DEF_SRC) && (flags & USE_SRC))
		return 1;

	return 0;
}

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

bool insn_vreg_def(struct use_position *use, struct var_info *var)
{
	unsigned long flags;
	struct insn *insn;

	insn = use->insn;
	flags = insn_flags[insn->type];

	if (flags & DEF_SRC)
		if (insn->src.reg.interval->var_info == var)
			return true;

	if (flags & DEF_DST) {
		if (insn->type != INSN_PHI) {
			if (insn->dest.reg.interval->var_info == var)
				return true;
		} else {
			if (insn->ssa_dest.reg.interval->var_info == var)
				return true;
		}
	}

	return false;
}

int insn_defs_reg(struct insn *insn, struct use_position **regs)
{
	unsigned long flags;
	int nr = 0;

	flags = insn_flags[insn->type];

	if (flags & DEF_SRC)
		regs[nr++] = &insn->src.reg;

	if (flags & DEF_DST) {
		if (insn->type != INSN_PHI)
			regs[nr++] = &insn->dest.reg;
		else regs[nr++] = &insn->ssa_dest.reg;
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

bool insn_vreg_use(struct use_position *use, struct var_info *var)
{
	unsigned long flags;
	struct insn *insn;

	insn = use->insn;
	flags = insn_flags[insn->type];

	if (use->kind == USE_KIND_INVALID)
		return true;

	if (insn_is_phi(insn)) {
		if (flags & USE_SRC) {
			for (unsigned long i = 0; i < insn->nr_srcs; i++)
				if (insn->ssa_srcs[i].reg.interval->var_info == var)
					return true;
		}

		return false;
	}
	if (flags & USE_SRC)
		if (insn->src.reg.interval->var_info == var)
			return true;

	if (flags & USE_DST)
		if (insn->dest.reg.interval->var_info == var)
			return true;

	if (flags & USE_IDX_SRC)
		if (insn->src.index_reg.interval->var_info == var)
			return true;

	if (flags & USE_IDX_DST)
		if (insn->dest.index_reg.interval->var_info == var)
			return true;

	return false;

}

unsigned long insn_uses_reg(struct insn *insn, struct use_position **regs)
{
	unsigned long flags;
	unsigned long nr = 0;

	flags = insn_flags[insn->type];

	if (insn_is_phi(insn)) {
		if (flags & USE_SRC) {
			for (unsigned long i = 0; i < insn->nr_srcs; i++)
				regs[nr++] = &insn->ssa_srcs[i].reg;
		}

		return nr;
	}

	if (flags & USE_SRC)
		regs[nr++] = &insn->src.reg;

	if (flags & USE_DST)
		regs[nr++] = &insn->dest.reg;

	if (flags & USE_IDX_SRC)
		regs[nr++] = &insn->src.index_reg;

	if (flags & USE_IDX_DST)
		regs[nr++] = &insn->dest.index_reg;

	return nr;
}

int insn_operand_use_kind(struct insn *insn, struct operand *operand)
{
	unsigned long flags;
	int kind_mask;
	int use_mask;
	int def_mask;

	flags = insn_flags[insn->type];

	if (operand == &insn->src) {
		use_mask = USE_SRC;
		def_mask = DEF_SRC;
	} else {
		assert(operand == &insn->dest);
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

bool insn_is_branch(struct insn *insn)
{
	unsigned long flags = insn_flags[insn->type];

	return flags & TYPE_BRANCH;
}

bool insn_is_jmp_mem(struct insn *insn)
{
	if (!insn)
		return false;

	if (insn->type == INSN_JMP_MEMINDEX || insn->type == INSN_JMP_MEMBASE)
		return true;

	return false;
}

bool insn_is_call(struct insn *insn)
{
	unsigned long flags = insn_flags[insn->type];

	return flags & TYPE_CALL;
}

bool insn_is_phi(struct insn *insn)
{
	if (insn->type == INSN_PHI)
		return true;

	return false;
}

unsigned long nr_srcs_phi(struct insn *insn)
{
	if (!insn_is_phi(insn))
		return 0;

	return insn->nr_srcs;
}
