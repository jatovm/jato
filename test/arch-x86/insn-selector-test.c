/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <jit/basic-block.h>
#include <jit/expression.h>
#include <jit/statement.h>
#include <jit/compiler.h>
#include <vm/vm.h>
#include <vm/class.h>
#include <arch/instruction.h>

#include <test/vars.h>
#include <test/vm.h>

static struct methodblock method = { };

static void
assert_memlocal_reg_insn(enum insn_type insn_type, unsigned long src_slot_idx,
			 enum machine_reg dest_reg, struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(src_slot_idx, insn->src.slot->index);
	assert_int_equals(dest_reg, mach_reg(&insn->dest.reg));
}

static void assert_membase_reg_insn(enum insn_type insn_type,
				    enum machine_reg src_base_reg,
				    long src_displacement,
				    enum machine_reg dest_reg,
				    struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(src_base_reg, mach_reg(&insn->src.base_reg));
	assert_int_equals(src_displacement, insn->src.disp);
	assert_int_equals(dest_reg, mach_reg(&insn->dest.reg));
}

static void assert_memindex_reg_insn(enum insn_type insn_type,
				     enum machine_reg src_base_reg,
				     enum machine_reg src_index_reg,
				     unsigned char src_shift,
				     enum machine_reg dest_reg,
				     struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(src_base_reg, mach_reg(&insn->src.base_reg));
	assert_int_equals(src_index_reg, mach_reg(&insn->src.index_reg));
	assert_int_equals(src_shift, insn->src.shift);
	assert_int_equals(dest_reg, mach_reg(&insn->dest.reg));
}

static void
assert_var_memlocal_insn(enum insn_type insn_type, struct var_info *src_var,
			 unsigned long dest_slot_idx, struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_ptr_equals(src_var, mach_reg_var(&insn->src.reg));
	assert_int_equals(dest_slot_idx, insn->dest.slot->index);
}

static void
assert_reg_memlocal_insn(enum insn_type insn_type, enum machine_reg src_reg,
			 unsigned long dest_slot_idx, struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(src_reg, mach_reg(&insn->src.reg));
	assert_int_equals(dest_slot_idx, insn->dest.slot->index);
}

static void assert_reg_memindex_insn(enum insn_type insn_type,
				     enum machine_reg src_reg,
				     enum machine_reg base_reg,
				     enum machine_reg index_reg,
				     unsigned char shift,
				     struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(src_reg, mach_reg(&insn->src.reg));
	assert_int_equals(base_reg, mach_reg(&insn->dest.base_reg));
	assert_int_equals(index_reg, mach_reg(&insn->dest.index_reg));
	assert_int_equals(shift, insn->dest.shift);
}

static void assert_reg_insn(enum insn_type insn_type,
			    enum machine_reg expected_reg, struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(expected_reg, mach_reg(&insn->operand.reg));
}

static void assert_reg_reg_insn(enum insn_type insn_type,
				enum machine_reg expected_src,
				enum machine_reg expected_dest,
				struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(expected_src, mach_reg(&insn->src.reg));
	assert_int_equals(expected_dest, mach_reg(&insn->dest.reg));
}

static void assert_reg_var_insn(enum insn_type insn_type,
				enum machine_reg expected_src,
				struct var_info *expected_dest,
				struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(expected_src, mach_reg(&insn->src.reg));
	assert_ptr_equals(expected_dest, mach_reg_var(&insn->dest.reg));
}

static void assert_imm_insn(enum insn_type insn_type,
			    unsigned long expected_imm, struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(expected_imm, insn->operand.imm);
}

static void assert_imm_reg_insn(enum insn_type insn_type,
				unsigned long expected_imm,
				enum machine_reg expected_reg,
				struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(expected_imm, insn->src.imm);
	assert_int_equals(expected_reg, mach_reg(&insn->dest.reg));
}

static void assert_imm_membase_insn(enum insn_type insn_type,
				    unsigned long expected_imm,
				    enum machine_reg expected_base_reg,
				    long expected_disp,
				    struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(expected_imm, insn->src.imm);
	assert_int_equals(expected_base_reg, mach_reg(&insn->dest.base_reg));
	assert_int_equals(expected_disp, insn->dest.disp);
}

static void assert_rel_insn(enum insn_type insn_type,
			    unsigned long expected_imm, struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_int_equals(expected_imm, insn->operand.rel);
}

static void assert_branch_insn(enum insn_type insn_type,
			       struct basic_block *if_true, struct insn *insn)
{
	assert_int_equals(insn_type, insn->type);
	assert_ptr_equals(if_true, insn->operand.branch_target);
}

void test_should_select_insn_for_every_statement(void)
{
	struct insn *insn;
	struct expression *expr1, *expr2;
	struct statement *stmt1, *stmt2;
	struct basic_block *bb;
	struct methodblock method = {
		.args_count = 4,
		.max_locals = 4,
	};
	struct compilation_unit *cu;
	enum machine_reg dreg;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);

	expr1 = binop_expr(J_INT, OP_ADD, local_expr(J_INT, 0), local_expr(J_INT, 1));

	stmt1 = alloc_statement(STMT_EXPRESSION);
	stmt1->expression = &expr1->node;

	expr2 = binop_expr(J_INT, OP_ADD, local_expr(J_INT, 2), local_expr(J_INT, 3));

	stmt2 = alloc_statement(STMT_RETURN);
	stmt2->return_value = &expr2->node;

	bb_add_stmt(bb, stmt1);
	bb_add_stmt(bb, stmt2);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(INSN_ADD_MEMBASE_REG, REG_EBP, 24, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 2, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(INSN_ADD_MEMBASE_REG, REG_EBP, 32, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg, REG_EAX, insn);

	free_compilation_unit(cu);
}

static struct basic_block *alloc_simple_binop_bb(enum binary_operator expr_op,
						 struct expression *left,
						 struct expression *right)
{
	static struct methodblock method = {
		.args_count = 2,
		.max_locals = 2,
	};
	struct compilation_unit *cu;
	struct expression *expr;
	struct basic_block *bb;
	struct statement *stmt;

	expr = binop_expr(J_INT, expr_op, left, right);
	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);
	return bb;
}

static struct basic_block *create_local_local_binop_bb(enum binary_operator expr_op)
{
	return alloc_simple_binop_bb(expr_op, local_expr(J_INT, 0), local_expr(J_INT, 1));
}

static void assert_select_local_local_binop(enum binary_operator expr_op, enum insn_type insn_type)
{
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	bb = create_local_local_binop_bb(expr_op);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(insn_type, REG_EBP, 24, dreg, insn);

	free_compilation_unit(bb->b_parent);
}

static struct basic_block *create_local_value_binop_bb(enum binary_operator expr_op)
{
	return alloc_simple_binop_bb(expr_op, local_expr(J_INT, 0), value_expr(J_INT, 0xdeadbeef));
}

static void assert_select_local_value_binop(enum binary_operator expr_op, enum insn_type insn_type)
{
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	bb = create_local_value_binop_bb(expr_op);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(insn_type, 0xdeadbeef, dreg, insn);

	free_compilation_unit(bb->b_parent);
}

void test_select_add_local_to_local(void)
{
	assert_select_local_local_binop(OP_ADD, INSN_ADD_MEMBASE_REG);
	assert_select_local_value_binop(OP_ADD, INSN_ADD_IMM_REG);
}

void test_select_sub_local_from_local(void)
{
	assert_select_local_local_binop(OP_SUB, INSN_SUB_MEMBASE_REG);
}

void test_select_local_local_mul(void)
{
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	bb = create_local_local_binop_bb(OP_MUL);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(INSN_MUL_MEMBASE_REG, REG_EBP, 24, REG_EAX, insn);

	free_compilation_unit(bb->b_parent);
}

void test_select_local_local_div(void)
{
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	bb = create_local_local_binop_bb(OP_DIV);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_CLTD_REG_REG, REG_EAX, REG_EDX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(INSN_DIV_MEMBASE_REG, REG_EBP, 24, REG_EAX, insn);

	free_compilation_unit(bb->b_parent);
}

void test_select_local_local_rem(void)
{
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	bb = create_local_local_binop_bb(OP_REM);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_CLTD_REG_REG, REG_EAX, REG_EDX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(INSN_DIV_MEMBASE_REG, REG_EBP, 24, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_MOV_REG_REG, REG_EDX, REG_EAX, insn);

	free_compilation_unit(bb->b_parent);
}

static struct basic_block *create_unop_bb(enum unary_operator expr_op)
{
	static struct methodblock method = {
		.args_count = 1,
		.max_locals = 1,
	};
	struct compilation_unit *cu;
	struct expression *expr;
	struct basic_block *bb;
	struct statement *stmt;

	expr = unary_op_expr(J_INT, expr_op, local_expr(J_INT, 0));
	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);
	return bb;
}

static void assert_select_local_unop(enum unary_operator expr_op, enum insn_type insn_type)
{
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	bb = create_unop_bb(expr_op);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(insn_type, dreg, insn);

	free_compilation_unit(bb->b_parent);
}

void test_select_neg_local(void)
{
	assert_select_local_unop(OP_NEG, INSN_NEG_REG);
}

static void assert_select_local_local_shift(enum binary_operator expr_op, enum insn_type insn_type)
{
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	bb = create_local_local_binop_bb(expr_op);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, REG_EBP, 24, REG_ECX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(insn_type, REG_ECX, dreg, insn);

	free_compilation_unit(bb->b_parent);
}

void test_select_shl_local_to_local(void)
{
	assert_select_local_local_shift(OP_SHL, INSN_SHL_REG_REG);
}

void test_select_shr_local_to_local(void)
{
	assert_select_local_local_shift(OP_SHR, INSN_SAR_REG_REG);
}

void test_select_ushr_local_to_local(void)
{
	assert_select_local_local_shift(OP_USHR, INSN_SHR_REG_REG);
}

void test_select_or_local_from_local(void)
{
	assert_select_local_local_binop(OP_OR, INSN_OR_MEMBASE_REG);
}

void test_select_and_local_from_local(void)
{
	assert_select_local_local_binop(OP_AND, INSN_AND_MEMBASE_REG);
}

void test_select_xor_local_from_local(void)
{
	assert_select_local_local_binop(OP_XOR, INSN_XOR_MEMBASE_REG);
}

void test_select_return(void)
{
	struct compilation_unit *cu;
	struct expression *value;
	struct basic_block *bb;
	struct statement *stmt;
	struct insn *insn;
	enum machine_reg dreg, dreg2;

	value = value_expr(J_LONG, 0xcafecacadeadbeef);

	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &value->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xdeadbeef, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xcafecaca, dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg2, REG_EDX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_branch_insn(INSN_JMP_BRANCH, cu->exit_bb, insn);

	free_compilation_unit(cu);
}

void test_select_void_return(void)
{
	struct compilation_unit *cu;
	struct basic_block *bb;
	struct statement *stmt;
	struct insn *insn;

	stmt = alloc_statement(STMT_VOID_RETURN);

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_branch_insn(INSN_JMP_BRANCH, cu->exit_bb, insn);

	free_compilation_unit(cu);
}

void test_select_invoke_without_arguments(void)
{
	struct expression *expr, *args_list;
	struct basic_block *bb;
	struct statement *stmt;
	struct insn *insn;
	struct methodblock mb = {
		.args_count = 0,
		.type = "()I",
	};

	mb.compilation_unit = alloc_compilation_unit(&mb);
	mb.trampoline = alloc_jit_trampoline();

	bb = get_basic_block(mb.compilation_unit, 0, 1);

	args_list = no_args_expr();
	expr = invoke_expr(&mb);
	expr->args_list = &args_list->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) trampoline_ptr(&mb), insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);
}

void test_select_invoke_with_arguments(void)
{
	struct expression *invoke_expression, *args_list_expression;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	struct methodblock mb = {
		.args_count = 2,
		.type = "(II)I",
	};

	mb.compilation_unit = alloc_compilation_unit(&mb);
	mb.trampoline = alloc_jit_trampoline();
	bb = get_basic_block(mb.compilation_unit, 0, 1);

	args_list_expression = args_list_expr(arg_expr(value_expr(J_INT, 0x02)),
					      arg_expr(value_expr
						       (J_INT, 0x01)));
	invoke_expression = invoke_expr(&mb);
	invoke_expression->args_list = &args_list_expression->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke_expression->node;
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, 0x02, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, 0x01, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) trampoline_ptr(&mb), insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 8, REG_ESP, insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);
}

void test_select_method_return_value_passed_as_argument(void)
{
	struct expression *no_args, *arg, *invoke, *nested_invoke;
	struct basic_block *bb;
	struct statement *stmt;
	struct insn *insn;
	struct methodblock mb = {
		.args_count = 1,
		.type = "(I)I",
	};
	struct methodblock nested_mb = {
		.args_count = 0,
		.type = "()I",
	};

	mb.compilation_unit = alloc_compilation_unit(&mb);
	mb.trampoline = alloc_jit_trampoline();
	bb = get_basic_block(mb.compilation_unit, 0, 1);

	nested_mb.compilation_unit = alloc_compilation_unit(&nested_mb);
	nested_mb.trampoline = alloc_jit_trampoline();

	no_args = no_args_expr();
	nested_invoke = invoke_expr(&nested_mb);
	nested_invoke->args_list = &no_args->node;

	arg = arg_expr(nested_invoke);
	invoke = invoke_expr(&mb);
	invoke->args_list = &arg->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke->node;
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) trampoline_ptr(&nested_mb), insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_PUSH_REG, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) trampoline_ptr(&mb), insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);

	free_jit_trampoline(nested_mb.trampoline);
	free_compilation_unit(nested_mb.compilation_unit);
}

void test_select_invokevirtual_with_arguments(void)
{
	struct methodblock method = {
		.max_locals = 1,
	};
	struct expression *invoke_expr, *args;
	struct compilation_unit *cu;
	unsigned long method_index;
	unsigned long objectref;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg, dreg2;

	objectref = 0xdeadbeef;
	method_index = 0xcafe;

	method.method_table_index = method_index;
	method.type = "()V";

	args = arg_expr(value_expr(J_REFERENCE, objectref));
	invoke_expr = invokevirtual_expr(&method);
	invoke_expr->args_list = &args->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke_expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, objectref, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, REG_ESP, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, dreg, offsetof(struct object, class), dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, sizeof(struct object), dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, dreg2,
				offsetof(struct classblock, method_table),
				dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, dreg, method_index * sizeof(void *), dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, dreg2, offsetof(struct methodblock, trampoline), dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, dreg, offsetof(struct buffer, buf), dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_CALL_REG, dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 4, REG_ESP, insn);

	free_compilation_unit(cu);
}

static struct statement *add_if_stmt(struct expression *expr, struct basic_block *bb, struct basic_block *true_bb)
{
	struct statement *stmt;

	stmt = alloc_statement(STMT_IF);
	stmt->expression = &expr->node;
	stmt->if_true = true_bb;
	bb_add_stmt(bb, stmt);

	return stmt;
}

static void assert_select_if_statement_local_local(enum insn_type expected,
						   enum binary_operator binop)
{
	struct basic_block *bb, *true_bb;
	struct compilation_unit *cu;
	struct expression *expr;
	struct statement *stmt;
	struct insn *insn;
	struct methodblock method = {
		.args_count = 2,
		.max_locals = 2,
	};
	enum machine_reg dreg;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	true_bb = get_basic_block(cu, 1, 2);

	expr = binop_expr(J_INT, binop, local_expr(J_INT, 0), local_expr(J_INT, 1));
	stmt = add_if_stmt(expr, bb, true_bb);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(INSN_CMP_MEMBASE_REG, REG_EBP, 24, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_branch_insn(expected, stmt->if_true, insn);

	free_compilation_unit(cu);
}

static void assert_select_if_statement_local_value(enum insn_type expected,
						   enum binary_operator binop)
{
	struct basic_block *bb, *true_bb;
	struct compilation_unit *cu;
	struct expression *expr;
	struct statement *stmt;
	struct insn *insn;
	struct methodblock method = {
		.args_count = 2,
		.max_locals = 2,
	};
	enum machine_reg dreg;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	true_bb = get_basic_block(cu, 1, 2);

	expr = binop_expr(J_INT, binop, local_expr(J_INT, 0), value_expr(J_INT, 0xcafebabe));
	stmt = add_if_stmt(expr, bb, true_bb);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_CMP_IMM_REG, 0xcafebabe, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_branch_insn(expected, stmt->if_true, insn);

	free_compilation_unit(cu);
}

static void assert_select_if_statement(enum insn_type expected,
				       enum binary_operator binop)
{
	assert_select_if_statement_local_local(expected, binop);
	assert_select_if_statement_local_value(expected, binop);
}

void test_select_if_statement(void)
{
	assert_select_if_statement(INSN_JE_BRANCH, OP_EQ);
	assert_select_if_statement(INSN_JNE_BRANCH, OP_NE);
}

void test_select_load_class_field(void)
{
	struct compilation_unit *cu;
	struct fieldblock field;
	struct expression *expr;
	struct basic_block *bb;
	struct statement *stmt;
	struct insn *insn;
	long expected_disp;
	enum machine_reg dreg, dreg2;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);

	expr = class_field_expr(J_INT, &field);
	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, (unsigned long) &field, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	expected_disp = offsetof(struct fieldblock, static_value);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, dreg, expected_disp, dreg2, insn);

	free_compilation_unit(cu);
}

void test_select_load_instance_field(void)
{
	struct methodblock method = {
		.args_count = 0,
		.max_locals = 1,
	};
	struct compilation_unit *cu;
	struct expression *objectref;
	struct fieldblock field;
	struct expression *expr;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg, dreg2, dreg3;

	field.offset = 8;

	objectref = local_expr(J_REFERENCE, 0);
	expr = instance_field_expr(J_INT, &field, objectref);
	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, sizeof(struct object), dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, field.offset, dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg3 = mach_reg(&insn->dest.reg);
	assert_memindex_reg_insn(INSN_MOV_MEMINDEX_REG, dreg, dreg2, 2, dreg3, insn);

	free_compilation_unit(cu);
}

void test_store_value_to_class_field(void)
{
	struct expression *store_target;
	struct expression *store_value;
	struct compilation_unit *cu;
	struct fieldblock field;
	struct basic_block *bb;
	struct statement *stmt;
	long expected_disp;
	struct insn *insn;
	enum machine_reg dreg;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	store_target = class_field_expr(J_LONG, &field);
	store_value  = value_expr(J_LONG, 0xdeadbeefcafebabe);
	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &store_target->node;
	stmt->store_src  = &store_value->node;
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, (unsigned long) &field, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	expected_disp = offsetof(struct fieldblock, static_value);
	assert_imm_membase_insn(INSN_MOV_IMM_MEMBASE, 0xcafebabe, dreg, expected_disp, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 4, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_membase_insn(INSN_MOV_IMM_MEMBASE, 0xdeadbeef, dreg, expected_disp, insn);

	free_compilation_unit(cu);
}

void test_store_value_to_instance_field(void)
{
	struct methodblock method = {
		.args_count = 0,
		.max_locals = 1,
	};
	struct expression *store_target;
	struct expression *store_value;
	struct expression *objectref;
	struct compilation_unit *cu;
	struct fieldblock field;
	struct basic_block *bb;
	struct statement *stmt;
	struct insn *insn;
	enum machine_reg dreg, dreg2, dreg3, vreg, vreg2;

	field.offset = 8;
	objectref    = local_expr(J_REFERENCE, 0);
	store_target = instance_field_expr(J_LONG, &field, objectref);
	store_value  = value_expr(J_LONG, 0xdeadbeefcafebabe);
	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &store_target->node;
	stmt->store_src  = &store_value->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);
	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, 0, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg, dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, sizeof(struct object), dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg3 = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, field.offset, dreg3, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	vreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xcafebabe, vreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	vreg2 = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xdeadbeef, vreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_memindex_insn(INSN_MOV_REG_MEMINDEX, vreg, dreg2, dreg3, 2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 1, dreg3, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_memindex_insn(INSN_MOV_REG_MEMINDEX, vreg2, dreg2, dreg3, 2, insn);

	free_compilation_unit(cu);
}

static void assert_store_field_to_local(unsigned long local_idx)
{
	struct expression *store_dest, *store_src;
	struct compilation_unit *cu;
	struct fieldblock field;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	struct methodblock method = {
		.args_count = 0,
		.max_locals = local_idx+1,
	};
	enum machine_reg dreg, dreg2;

	store_dest = local_expr(J_INT, local_idx);
	store_src  = class_field_expr(J_INT, &field);

	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &store_dest->node;
	stmt->store_src  = &store_src->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, (unsigned long) &field, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, dreg, offsetof(struct fieldblock, static_value), dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_memlocal_insn(INSN_MOV_REG_MEMLOCAL, dreg2, local_idx, insn);

	free_compilation_unit(cu);
}

void test_select_store_field_to_local(void)
{
	assert_store_field_to_local(0);
	assert_store_field_to_local(1);
}

void test_select_store_value_to_var(void)
{
	struct expression *store_dest, *store_src;
	struct compilation_unit *cu;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	struct methodblock method = {
		.args_count = 0,
	};
	enum machine_reg dreg;
	struct var_info *temporary;

	cu = alloc_compilation_unit(&method);

	temporary = get_var(cu);
	store_dest = temporary_expr(J_REFERENCE, temporary);
	store_src  = value_expr(J_REFERENCE, 0xdeadbeef);

	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &store_dest->node;
	stmt->store_src  = &store_src->node;

	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xdeadbeef, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_var_insn(INSN_MOV_REG_REG, dreg, mach_reg_var(&insn->dest.reg), insn);

	free_compilation_unit(cu);
}

void test_select_store_var_to_local(void)
{
	struct expression *store_dest, *store_src;
	struct compilation_unit *cu;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	struct methodblock method = {
		.args_count = 0,
		.max_locals = 1,
	};
	struct var_info *temporary;

	cu = alloc_compilation_unit(&method);

	store_dest = local_expr(J_INT, 0);
	temporary = get_var(cu);
	store_src  = temporary_expr(J_INT, temporary);

	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &store_dest->node;
	stmt->store_src  = &store_src->node;

	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_var_memlocal_insn(INSN_MOV_REG_MEMLOCAL, mach_reg_var(&insn->src.reg), 0, insn);

	free_compilation_unit(cu);
}

void test_select_new(void)
{
	struct object *instance_class;
	struct compilation_unit *cu;
	struct expression *expr;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;

	instance_class = new_class();
	expr = new_expr(instance_class);
	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, (unsigned long) instance_class, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) allocObject, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 4, REG_ESP, insn);

	free(instance_class);
	free_compilation_unit(cu);
}

void test_select_newarray(void)
{
	struct compilation_unit *cu;
	struct expression *size, *expr;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	size = value_expr(J_INT, 0xff);
	expr = newarray_expr(T_INT, size);
	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xff, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_PUSH_REG, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, T_INT, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) allocTypeArray, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 8, REG_ESP, insn);

	free_compilation_unit(cu);
}

void test_select_anewarray(void)
{
	struct object *instance_class;
	struct compilation_unit *cu;
	struct expression *expr,*size;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	instance_class = new_class();
	size = value_expr(J_INT,0xFF);

	expr = anewarray_expr(instance_class,size);
	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xff, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, sizeof(instance_class), insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_PUSH_REG, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM,(unsigned long) instance_class, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) allocArray, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 12, REG_ESP, insn);

	free(instance_class);
	free_compilation_unit(cu);
}

void test_select_multianewarray(void)
{
	struct object *instance_class;
	struct compilation_unit *cu;
	struct expression *expr, *args_list_expression;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;

	instance_class = new_class();

	args_list_expression = args_list_expr(arg_expr(value_expr(J_INT, 0x02)),
					      arg_expr(value_expr(J_INT, 0xff)));

	expr = multianewarray_expr(instance_class);
	expr->multianewarray_dimensions = &args_list_expression->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, 0x02, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, 0xff, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_PUSH_REG, REG_ESP, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, 0x02, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, (unsigned long) instance_class, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) allocMultiArray, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 20, REG_ESP, insn);

	free(instance_class);
	free_compilation_unit(cu);
}

void test_select_arraylength(void)
{
	struct object *instance_class;
	struct compilation_unit *cu;
	struct expression *expr, *arraylength_exp;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg, dreg2;

	instance_class = new_class();
	expr = value_expr(J_REFERENCE, (unsigned long) instance_class);

	arraylength_exp = arraylength_expr(expr);
	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &arraylength_exp->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, (unsigned long) instance_class, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_membase_reg_insn(INSN_MOV_MEMBASE_REG, dreg, sizeof(struct object), dreg2, insn);

	free(instance_class);
	free_compilation_unit(cu);
}

void test_select_instanceof(void)
{
	struct object *instance_class;
	struct compilation_unit *cu;
	struct expression *expr, *ref;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	enum machine_reg dreg;

	instance_class = new_class();
	ref = value_expr(J_REFERENCE, (unsigned long) instance_class);

	expr = instanceof_expr(ref, instance_class);
	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, (unsigned long) instance_class, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(INSN_PUSH_IMM, (unsigned long) instance_class, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_PUSH_REG, dreg, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) is_object_instance_of, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 8, REG_ESP, insn);

	free(instance_class);
	free_compilation_unit(cu);
}

void test_select_goto_stmt(void)
{
	struct compilation_unit *cu;
	struct basic_block *bb, *goto_target;
	struct statement *stmt;
	struct insn *insn;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	goto_target = get_basic_block(cu, 1, 2);

	stmt = alloc_statement(STMT_GOTO);
	stmt->goto_target = goto_target;
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_branch_insn(INSN_JMP_BRANCH, goto_target, insn);

	free_compilation_unit(cu);
}

void test_select_null_check_stmt(void)
{
	struct compilation_unit *cu;
	struct basic_block *bb;
	struct statement *stmt;
	struct expression *expr;
	struct insn *insn;
	enum machine_reg dreg1;

	expr = value_expr(J_REFERENCE, 0xcafebabe);
	stmt = alloc_statement(STMT_NULL_CHECK);
	stmt->expression = &expr->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg1 = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xcafebabe, dreg1, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_PUSH_REG, dreg1, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) check_null, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 4, REG_ESP, insn);

	free_compilation_unit(cu);
}

void test_select_array_check_stmt(void)
{
	struct compilation_unit *cu;
	struct basic_block *bb;
	struct statement *stmt;
	struct expression *base, *index, *array;
	struct insn *insn;
	enum machine_reg dreg1, dreg2, dreg3, dreg4;

	base = value_expr(J_REFERENCE, 0xcafebabe);
	index = value_expr(J_INT, 0x00);

	array = array_deref_expr(J_INT, base, index);

	stmt = alloc_statement(STMT_ARRAY_CHECK);
	stmt->expression = &array->node;

	cu = alloc_compilation_unit(&method);
	bb = get_basic_block(cu, 0, 1);
	bb_add_stmt(bb, stmt);

	select_instructions(bb->b_parent);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	dreg1 = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0xcafebabe, dreg1, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg2 = mach_reg(&insn->dest.reg);
	assert_imm_reg_insn(INSN_MOV_IMM_REG, 0x00, dreg2, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg3 = mach_reg(&insn->dest.reg);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg1, dreg3, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	dreg4 = mach_reg(&insn->dest.reg);
	assert_reg_reg_insn(INSN_MOV_REG_REG, dreg2, dreg4, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_PUSH_REG, dreg4, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(INSN_PUSH_REG, dreg3, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(INSN_CALL_REL, (unsigned long) check_array, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(INSN_ADD_IMM_REG, 8, REG_ESP, insn);

	free_compilation_unit(cu);
}
