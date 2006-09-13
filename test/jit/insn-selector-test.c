/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <jit/basic-block.h>
#include <jit/expression.h>
#include <jit/instruction.h>
#include <jit/insn-selector.h>
#include <jit/statement.h>
#include <jit/jit-compiler.h>

static void assert_insn(enum insn_opcode insn_op, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_0(insn_op), insn->type);
}

static void assert_membase_reg_insn(enum insn_opcode insn_op,
				    enum reg src_base_reg,
				    long src_displacement,
				    enum reg dest_reg,
				    struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_2(insn_op, OPERAND_MEMBASE, OPERAND_REGISTER), insn->type);
	assert_int_equals(src_base_reg, insn->src.base_reg);
	assert_int_equals(src_displacement, insn->src.disp);
	assert_int_equals(dest_reg, insn->dest.reg);
}

static void assert_reg_membase_insn(enum insn_opcode insn_op,
				    enum reg src_reg,
				    enum reg dest_reg,
				    long dest_displacement,
				    struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_2(insn_op, OPERAND_REGISTER, OPERAND_MEMBASE), insn->type);
	assert_int_equals(src_reg, insn->src.reg);
	assert_int_equals(dest_reg, insn->dest.base_reg);
	assert_int_equals(dest_displacement, insn->dest.disp);
}

static void assert_reg_insn(enum insn_opcode expected_opc,
			    enum reg expected_reg, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_1(expected_opc, OPERAND_REGISTER), insn->type);
	assert_int_equals(expected_reg, insn->operand.reg);
}

static void assert_reg_reg_insn(enum insn_opcode expected_opc,
				enum reg expected_src,
				enum reg expected_dest,
				struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_2(expected_opc, OPERAND_REGISTER, OPERAND_REGISTER), insn->type);
	assert_int_equals(expected_src, insn->src.reg);
	assert_int_equals(expected_dest, insn->dest.reg);
}

static void assert_imm_insn(enum insn_opcode expected_opc,
			    unsigned long expected_imm, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_1(expected_opc, OPERAND_IMMEDIATE), insn->type);
	assert_int_equals(expected_imm, insn->operand.imm);
}

static void assert_imm_reg_insn(enum insn_opcode expected_opc,
				unsigned long expected_imm,
				enum reg expected_reg,
				struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_2(expected_opc, OPERAND_IMMEDIATE, OPERAND_REGISTER), insn->type);
	assert_int_equals(expected_imm, insn->src.imm);
	assert_int_equals(expected_reg, insn->dest.reg);
}

static void assert_imm_membase_insn(enum insn_opcode expected_opc,
				    unsigned long expected_imm,
				    enum reg expected_base_reg,
				    long expected_disp,
				    struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_2(expected_opc, OPERAND_IMMEDIATE, OPERAND_MEMBASE), insn->type);
	assert_int_equals(expected_imm, insn->src.imm);
	assert_int_equals(expected_base_reg, insn->dest.base_reg);
	assert_int_equals(expected_disp, insn->dest.disp);
}

static void assert_rel_insn(enum insn_opcode expected_opc,
			    unsigned long expected_imm, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_1(expected_opc, OPERAND_RELATIVE), insn->type);
	assert_int_equals(expected_imm, insn->operand.rel);
}

static void assert_branch_insn(enum insn_opcode expected_opc,
			       struct basic_block *if_true, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_1(expected_opc, OPERAND_BRANCH), insn->type);
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
	};
	struct compilation_unit cu = {
		.method = &method,
	};
	
	bb = alloc_basic_block(&cu, 0, 1);

	expr1 = binop_expr(J_INT, OP_ADD, local_expr(J_INT, 0), local_expr(J_INT, 1));

	stmt1 = alloc_statement(STMT_EXPRESSION);
	stmt1->expression = &expr1->node;

	expr2 = binop_expr(J_INT, OP_ADD, local_expr(J_INT, 2), local_expr(J_INT, 3));

	stmt2 = alloc_statement(STMT_RETURN);
	stmt2->return_value = &expr2->node;

	bb_add_stmt(bb, stmt1);
	bb_add_stmt(bb, stmt2);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EBP, 8, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_ADD, REG_EBP, 12, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EBP, 16, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_ADD, REG_EBP, 20, REG_EAX, insn);

	free_basic_block(bb);
}

static struct basic_block *create_binop_bb(enum binary_operator expr_op)
{
	struct methodblock method = {
		.args_count = 2,
	};
	struct compilation_unit cu = {
		.method = &method,
	};
	struct expression *expr;
	struct basic_block *bb;
	struct statement *stmt;

	expr = binop_expr(J_INT, expr_op, local_expr(J_INT, 0), local_expr(J_INT, 1));
	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &expr->node;

	bb = alloc_basic_block(&cu, 0, 1);
	bb_add_stmt(bb, stmt);
	return bb;
}

static void assert_select_local_local_binop(enum binary_operator expr_op, enum insn_opcode insn_op)
{
	struct basic_block *bb;
	struct insn *insn;

	bb = create_binop_bb(expr_op);
	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EBP, 8, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(insn_op, REG_EBP, 12, REG_EAX, insn);

	free_basic_block(bb);
}

void test_select_add_local_to_local(void)
{
	assert_select_local_local_binop(OP_ADD, OPC_ADD);
}

void test_select_sub_local_from_local(void)
{
	assert_select_local_local_binop(OP_SUB, OPC_SUB);
}

void test_select_mul_local_from_local(void)
{
	assert_select_local_local_binop(OP_MUL, OPC_MUL);
}

void test_select_local_local_div(void)
{
	struct basic_block *bb;
	struct insn *insn;

	bb = create_binop_bb(OP_DIV);
	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EBP, 8, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_insn(OPC_CLTD, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_DIV, REG_EBP, 12, REG_EAX, insn);

	free_basic_block(bb);
}

void test_select_local_local_rem(void)
{
	struct basic_block *bb;
	struct insn *insn;

	bb = create_binop_bb(OP_REM);
	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EBP, 8, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_insn(OPC_CLTD, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_DIV, REG_EBP, 12, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_reg_insn(OPC_MOV, REG_EDX, REG_EAX, insn);

	free_basic_block(bb);
}

void test_select_return(void)
{
	struct compilation_unit cu;
	struct expression *value;
	struct basic_block *bb;
	struct statement *stmt;
	struct insn *insn;

	value = value_expr(J_INT, 0xdeadbeef);

	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &value->node;

	bb = alloc_basic_block(&cu, 0, 1);
	bb_add_stmt(bb, stmt);

	cu.exit_bb = alloc_basic_block(&cu, 1, 1);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_reg_insn(OPC_MOV, 0xdeadbeef, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_branch_insn(OPC_JMP, cu.exit_bb, insn);

	free_basic_block(bb);
	free_basic_block(cu.exit_bb);
}

void test_select_void_return(void)
{
	struct compilation_unit cu;
	struct basic_block *bb;
	struct statement *stmt;
	struct insn *insn;

	stmt = alloc_statement(STMT_VOID_RETURN);

	bb = alloc_basic_block(&cu, 0, 1);
	bb_add_stmt(bb, stmt);

	cu.exit_bb = alloc_basic_block(&cu, 1, 1);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_branch_insn(OPC_JMP, cu.exit_bb, insn);

	free_basic_block(bb);
	free_basic_block(cu.exit_bb);
}

void test_select_invoke_without_arguments(void)
{
	struct basic_block *bb = alloc_basic_block(NULL, 0, 1);
	struct insn *insn;
	struct expression *expr, *args_list;
	struct statement *stmt;
	struct methodblock mb = {
		.args_count = 0
	};

	jit_prepare_for_exec(&mb);

	args_list = no_args_expr();
	expr = invoke_expr(J_INT, &mb);
	expr->args_list = &args_list->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;
	bb_add_stmt(bb, stmt);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_rel_insn(OPC_CALL, (unsigned long) trampoline_ptr(&mb), insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);
	free_basic_block(bb);
}

void test_select_invoke_with_arguments(void)
{
	struct basic_block *bb = alloc_basic_block(NULL, 0, 1);
	struct insn *insn;
	struct expression *invoke_expression, *args_list_expression;
	struct statement *stmt;
	struct methodblock mb = {
		.args_count = 2
	};

	jit_prepare_for_exec(&mb);

	args_list_expression = args_list_expr(arg_expr(value_expr(J_INT, 0x02)),
					      arg_expr(value_expr
						       (J_INT, 0x01)));
	invoke_expression = invoke_expr(J_INT, &mb);
	invoke_expression->args_list = &args_list_expression->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke_expression->node;
	bb_add_stmt(bb, stmt);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_insn(OPC_PUSH, 0x02, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_insn(OPC_PUSH, 0x01, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(OPC_CALL, (unsigned long) trampoline_ptr(&mb), insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(OPC_ADD, 8, REG_ESP, insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);
	free_basic_block(bb);
}

void test_select_method_return_value_passed_as_argument(void)
{
	struct basic_block *bb = alloc_basic_block(NULL, 0, 1);
	struct insn *insn;
	struct expression *no_args, *arg, *invoke, *nested_invoke;
	struct statement *stmt;
	struct methodblock mb = { .args_count = 1 }, nested_mb = { .args_count = 0 };

	jit_prepare_for_exec(&mb);
	jit_prepare_for_exec(&nested_mb);

	no_args = no_args_expr();
	nested_invoke = invoke_expr(J_INT, &nested_mb);
	nested_invoke->args_list = &no_args->node;

	arg = arg_expr(nested_invoke);
	invoke = invoke_expr(J_INT, &mb);
	invoke->args_list = &arg->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke->node;
	bb_add_stmt(bb, stmt);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_rel_insn(OPC_CALL, (unsigned long) trampoline_ptr(&nested_mb), insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(OPC_PUSH, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_rel_insn(OPC_CALL, (unsigned long) trampoline_ptr(&mb), insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);

	free_jit_trampoline(nested_mb.trampoline);
	free_compilation_unit(nested_mb.compilation_unit);

	free_basic_block(bb);
}

void test_select_invokevirtual_with_arguments(void)
{
	unsigned long objectref;
	unsigned long method_index;
	struct expression *invoke_expr, *args;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;

	objectref = 0xdeadbeef;
	method_index = 0xcafe;

	args = arg_expr(value_expr(J_REFERENCE, objectref));
	invoke_expr = invokevirtual_expr(J_VOID, method_index);
	invoke_expr->args_list = &args->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke_expr->node;

	bb = alloc_basic_block(NULL, 0, 1);
	bb_add_stmt(bb, stmt);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_insn(OPC_PUSH, objectref, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_ESP, 0, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EAX, offsetof(struct object, class), REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(OPC_ADD, sizeof(struct object), REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EAX,
				offsetof(struct classblock, method_table),
				REG_EAX, insn);
	
	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EAX, method_index * sizeof(void *), REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EAX, offsetof(struct methodblock, trampoline), REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EAX, offsetof(struct buffer, buf), REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_insn(OPC_CALL, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(OPC_ADD, 4, REG_ESP, insn);
	
	free_basic_block(bb);
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

void test_select_local_eq_local_in_if_statement(void)
{
	struct basic_block *bb, *true_bb;
	struct insn *insn;
	struct expression *expr;
	struct statement *stmt;
	struct methodblock method = {
		.args_count = 2,
	};
	struct compilation_unit cu = {
		.method = &method,
	};

	bb = alloc_basic_block(&cu, 0, 1);
	true_bb = alloc_basic_block(&cu, 1, 2);

	expr = binop_expr(J_INT, OP_EQ, local_expr(J_INT, 0), local_expr(J_INT, 1));
	stmt = add_if_stmt(expr, bb, true_bb);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EBP, 8, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_CMP, REG_EBP, 12, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);

	assert_branch_insn(OPC_JE, stmt->if_true, insn);

	free_basic_block(bb);
	free_basic_block(true_bb);
}

void test_select_local_eq_value_in_if_statement(void)
{
	struct basic_block *bb, *true_bb;
	struct insn *insn;
	struct expression *expr;
	struct statement *stmt;
	struct methodblock method = {
		.args_count = 2,
	};
	struct compilation_unit cu = {
		.method = &method,
	};

	bb = alloc_basic_block(&cu, 0, 1);
	true_bb = alloc_basic_block(&cu, 1, 2);

	expr = binop_expr(J_INT, OP_EQ, local_expr(J_INT, 0), value_expr(J_INT, 0xcafebabe));
	stmt = add_if_stmt(expr, bb, true_bb);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EBP, 8, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_imm_reg_insn(OPC_CMP, 0xcafebabe, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);

	assert_branch_insn(OPC_JE, stmt->if_true, insn);

	free_basic_block(bb);
	free_basic_block(true_bb);
}

void test_select_load_field(void)
{
	struct basic_block *bb = alloc_basic_block(NULL, 0, 1);
	struct insn *insn;
	struct expression *expr;
	struct statement *stmt;
	struct fieldblock field;
	long expected_disp;

	expr = field_expr(J_INT, &field);
	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;
	bb_add_stmt(bb, stmt);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_reg_insn(OPC_MOV, (unsigned long) &field, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	expected_disp = offsetof(struct fieldblock, static_value);
	assert_membase_reg_insn(OPC_MOV, REG_EAX, expected_disp, REG_EAX, insn);

	free_basic_block(bb);
}

void test_store_value_to_field(void)
{
	struct basic_block *bb = alloc_basic_block(NULL, 0, 1);
	struct insn *insn;
	struct expression *store_target;
	struct expression *store_value;
	struct statement *stmt;
	struct fieldblock field;
	long expected_disp;

	store_target = field_expr(J_INT, &field);
	store_value  = value_expr(J_INT, 0xcafebabe);
	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &store_target->node;
	stmt->store_src  = &store_value->node;
	bb_add_stmt(bb, stmt);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_reg_insn(OPC_MOV, (unsigned long) &field, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	expected_disp = offsetof(struct fieldblock, static_value);
	assert_imm_membase_insn(OPC_MOV, 0xcafebabe, REG_EAX, expected_disp, insn);

	free_basic_block(bb);
}

static void assert_store_field_to_local(long expected_disp, unsigned long local_idx)
{
	struct fieldblock field;
	struct expression *store_dest, *store_src;
	struct statement *stmt;
	struct basic_block *bb;
	struct insn *insn;
	struct methodblock method = {
		.args_count = 0,
	};
	struct compilation_unit cu = {
		.method = &method,
	};

	store_dest = local_expr(J_INT, local_idx);
	store_src  = field_expr(J_INT, &field);

	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &store_dest->node;
	stmt->store_src  = &store_src->node;

	bb = alloc_basic_block(&cu, 0, 1);
	bb_add_stmt(bb, stmt);

	insn_select(bb);

	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_imm_reg_insn(OPC_MOV, (unsigned long) &field, REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_membase_reg_insn(OPC_MOV, REG_EAX, offsetof(struct fieldblock, static_value), REG_EAX, insn);

	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_reg_membase_insn(OPC_MOV, REG_EAX, REG_EBP, expected_disp, insn);

	free_basic_block(bb);
}

void test_select_store_field_to_local(void)
{
	assert_store_field_to_local(-4, 0);
	assert_store_field_to_local(-8, 1);
}
