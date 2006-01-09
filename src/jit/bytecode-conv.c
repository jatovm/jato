/*
 * Copyright (C) 2005  Pekka Enberg
 *
 * This file contains functions for converting Java bytecode to three-address
 * code.
 */

#include <statement.h>
#include <byteorder.h>
#include <operand-stack.h>

#include <stdlib.h>
#include <assert.h>

static struct statement *alloc_stmt(enum statement_type type)
{
	struct statement *stmt = malloc(sizeof(*stmt));
	if (stmt)
		stmt->type = type;
	return stmt;
}

static struct statement *convert_nop(struct classblock *cb,
				     unsigned char *code, size_t len,
				     struct operand_stack *stack)
{
	return alloc_stmt(STMT_NOP);
}

static struct statement *convert_aconst_null(struct classblock *cb,
					     unsigned char *code, size_t len,
					     struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(&stmt->s_left, CONST_REFERENCE, 0);
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *convert_iconst(struct classblock *cb,
					unsigned char *code, size_t len,
					struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(&stmt->s_left, CONST_INT,
				  code[0] - OPC_ICONST_0);
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *convert_lconst(struct classblock *cb,
					unsigned char *code, size_t len,
					struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(&stmt->s_left, CONST_LONG,
				  code[0] - OPC_LCONST_0);
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *convert_fconst(struct classblock *cb,
					unsigned char *code, size_t len,
					struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_fconst(&stmt->s_left, CONST_FLOAT,
				   code[0] - OPC_FCONST_0);
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *convert_dconst(struct classblock *cb,
					unsigned char *code, size_t len,
					struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_fconst(&stmt->s_left, CONST_DOUBLE,
				   code[0] - OPC_DCONST_0);
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *convert_bipush(struct classblock *cb,
					unsigned char *code, size_t len,
					struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(&stmt->s_left, CONST_INT, (char)code[1]);
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *convert_sipush(struct classblock *cb,
					unsigned char *code, size_t len,
					struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(&stmt->s_left, CONST_INT,
				  (short)be16_to_cpu(*(u2 *) & code[1]));
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *__convert_ldc(struct constant_pool *cp,
				       unsigned long cp_idx,
				       struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		u1 type = CP_TYPE(cp, cp_idx);
		ConstantPoolEntry entry = be64_to_cpu(CP_INFO(cp, cp_idx));
		switch (type) {
		case CONSTANT_Integer:
			operand_set_const(&stmt->s_left, CONST_INT, entry);
			break;
		case CONSTANT_Float:
			operand_set_fconst(&stmt->s_left, CONST_FLOAT,
					   *(float *)&entry);
			break;
		case CONSTANT_String:
			operand_set_const(&stmt->s_left, CONST_REFERENCE,
					  entry);
			break;
		case CONSTANT_Long:
			operand_set_const(&stmt->s_left, CONST_LONG, entry);
			break;
		case CONSTANT_Double:
			operand_set_fconst(&stmt->s_left, CONST_DOUBLE,
					   *(double *)&entry);
			stmt->s_left.constant.type = CONST_DOUBLE;
			break;
		default:
			assert(!"unknown constant type");
		}
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *convert_ldc(struct classblock *cb, unsigned char *code,
				     size_t len, struct operand_stack *stack)
{
	return __convert_ldc(&cb->constant_pool, code[1], stack);
}

static struct statement *convert_ldc_w(struct classblock *cb,
				       unsigned char *code, size_t len,
				       struct operand_stack *stack)
{
	return __convert_ldc(&cb->constant_pool, be16_to_cpu(*(u2 *) & code[1]), stack);
}

static struct statement *convert_ldc2_w(struct classblock *cb,
					unsigned char *code, size_t len,
					struct operand_stack *stack)
{
	return __convert_ldc(&cb->constant_pool, be16_to_cpu(*(u2 *) & code[1]), stack);
}

static struct statement *__convert_load(unsigned char index,
					enum local_variable_type type,
					struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_local_var(&stmt->s_left, type, index);
		stack_push(stack, stmt->target);
	}
	return stmt;
}

static struct statement *convert_load(unsigned char *code, size_t len,
				      enum local_variable_type type,
				      struct operand_stack *stack)
{
	return __convert_load(code[1], type, stack);
}

static struct statement *convert_iload(struct classblock *cb,
				       unsigned char *code, size_t len,
				       struct operand_stack *stack)
{
	return convert_load(code, len, LOCAL_VAR_INT, stack);
}

static struct statement *convert_lload(struct classblock *cb,
				       unsigned char *code, size_t len,
				       struct operand_stack *stack)
{
	return convert_load(code, len, LOCAL_VAR_LONG, stack);
}

static struct statement *convert_fload(struct classblock *cb,
				       unsigned char *code, size_t len,
				       struct operand_stack *stack)
{
	return convert_load(code, len, LOCAL_VAR_FLOAT, stack);
}

static struct statement *convert_dload(struct classblock *cb,
				       unsigned char *code, size_t len,
				       struct operand_stack *stack)
{
	return convert_load(code, len, LOCAL_VAR_DOUBLE, stack);
}

static struct statement *convert_aload(struct classblock *cb,
				       unsigned char *code, size_t len,
				       struct operand_stack *stack)
{
	return convert_load(code, len, LOCAL_VAR_REFERENCE, stack);
}

static struct statement *convert_iload_x(struct classblock *cb,
					 unsigned char *code, size_t len,
					 struct operand_stack *stack)
{
	return __convert_load(code[0] - OPC_ILOAD_0, LOCAL_VAR_INT, stack);
}

static struct statement *convert_lload_x(struct classblock *cb,
					 unsigned char *code, size_t len,
					 struct operand_stack *stack)
{
	return __convert_load(code[0] - OPC_LLOAD_0, LOCAL_VAR_LONG, stack);
}

static struct statement *convert_fload_x(struct classblock *cb,
					 unsigned char *code, size_t len,
					 struct operand_stack *stack)
{
	return __convert_load(code[0] - OPC_FLOAD_0, LOCAL_VAR_FLOAT, stack);
}

static struct statement *convert_dload_x(struct classblock *cb,
					 unsigned char *code, size_t len,
					 struct operand_stack *stack)
{
	return __convert_load(code[0] - OPC_DLOAD_0, LOCAL_VAR_DOUBLE, stack);
}

static struct statement *convert_aload_x(struct classblock *cb,
					 unsigned char *code, size_t len,
					 struct operand_stack *stack)
{
	return __convert_load(code[0] - OPC_ALOAD_0, LOCAL_VAR_REFERENCE,
			      stack);
}

static struct statement *convert_xaload(struct classblock *cb,
					unsigned char *code, size_t len,
					struct operand_stack *stack)
{
	unsigned long index = stack_pop(stack);
	unsigned long arrayref = stack_pop(stack);
	struct statement *assign = malloc(sizeof(struct statement));

	assert(assign);
	assign->type = STMT_ASSIGN;
	operand_set_arrayref(&assign->s_left, arrayref, index);

	stack_push(stack, assign->target);

	struct statement *arraycheck = malloc(sizeof(struct statement));
	assert(arraycheck);
	arraycheck->type = STMT_ARRAY_CHECK;
	operand_set_temporary(&arraycheck->s_left, arrayref);
	operand_set_temporary(&arraycheck->s_right, index);
	arraycheck->next = assign;

	struct statement *nullcheck = alloc_stmt(STMT_NULL_CHECK);
	if (nullcheck) {
		operand_set_temporary(&nullcheck->s_left, arrayref);
		nullcheck->next = arraycheck;
	}
	return nullcheck;
}

typedef struct statement *(*convert_fn_t) (struct classblock *,
					   unsigned char *, size_t,
					   struct operand_stack * stack);

struct converter {
	convert_fn_t convert;
	unsigned long require;
};

#define DECLARE_CONVERTER(opc, fn, bytes) \
		[opc] = { .convert = fn, .require = bytes }

static struct converter converters[] = {
	DECLARE_CONVERTER(OPC_NOP, convert_nop, 1),
	DECLARE_CONVERTER(OPC_ACONST_NULL, convert_aconst_null, 1),
	DECLARE_CONVERTER(OPC_ICONST_M1, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_0, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_1, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_2, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_3, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_4, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_5, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_LCONST_0, convert_lconst, 1),
	DECLARE_CONVERTER(OPC_LCONST_1, convert_lconst, 1),
	DECLARE_CONVERTER(OPC_FCONST_0, convert_fconst, 1),
	DECLARE_CONVERTER(OPC_FCONST_1, convert_fconst, 1),
	DECLARE_CONVERTER(OPC_FCONST_2, convert_fconst, 1),
	DECLARE_CONVERTER(OPC_DCONST_0, convert_dconst, 1),
	DECLARE_CONVERTER(OPC_DCONST_1, convert_dconst, 1),
	DECLARE_CONVERTER(OPC_BIPUSH, convert_bipush, 2),
	DECLARE_CONVERTER(OPC_SIPUSH, convert_sipush, 2),
	DECLARE_CONVERTER(OPC_LDC, convert_ldc, 2),
	DECLARE_CONVERTER(OPC_LDC_W, convert_ldc_w, 3),
	DECLARE_CONVERTER(OPC_LDC2_W, convert_ldc2_w, 3),
	DECLARE_CONVERTER(OPC_ILOAD, convert_iload, 2),
	DECLARE_CONVERTER(OPC_LLOAD, convert_lload, 2),
	DECLARE_CONVERTER(OPC_FLOAD, convert_fload, 2),
	DECLARE_CONVERTER(OPC_DLOAD, convert_dload, 2),
	DECLARE_CONVERTER(OPC_ALOAD, convert_aload, 2),
	DECLARE_CONVERTER(OPC_ILOAD_0, convert_iload_x, 2),
	DECLARE_CONVERTER(OPC_ILOAD_1, convert_iload_x, 2),
	DECLARE_CONVERTER(OPC_ILOAD_2, convert_iload_x, 2),
	DECLARE_CONVERTER(OPC_ILOAD_3, convert_iload_x, 2),
	DECLARE_CONVERTER(OPC_LLOAD_0, convert_lload_x, 2),
	DECLARE_CONVERTER(OPC_LLOAD_1, convert_lload_x, 2),
	DECLARE_CONVERTER(OPC_LLOAD_2, convert_lload_x, 2),
	DECLARE_CONVERTER(OPC_LLOAD_3, convert_lload_x, 2),
	DECLARE_CONVERTER(OPC_FLOAD_0, convert_fload_x, 2),
	DECLARE_CONVERTER(OPC_FLOAD_1, convert_fload_x, 2),
	DECLARE_CONVERTER(OPC_FLOAD_2, convert_fload_x, 2),
	DECLARE_CONVERTER(OPC_FLOAD_3, convert_fload_x, 2),
	DECLARE_CONVERTER(OPC_DLOAD_0, convert_dload_x, 2),
	DECLARE_CONVERTER(OPC_DLOAD_1, convert_dload_x, 2),
	DECLARE_CONVERTER(OPC_DLOAD_2, convert_dload_x, 2),
	DECLARE_CONVERTER(OPC_DLOAD_3, convert_dload_x, 2),
	DECLARE_CONVERTER(OPC_ALOAD_0, convert_aload_x, 2),
	DECLARE_CONVERTER(OPC_ALOAD_1, convert_aload_x, 2),
	DECLARE_CONVERTER(OPC_ALOAD_2, convert_aload_x, 2),
	DECLARE_CONVERTER(OPC_ALOAD_3, convert_aload_x, 2),
	DECLARE_CONVERTER(OPC_IALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_LALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_FALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_DALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_AALOAD, convert_xaload, 1),
};

struct statement *convert_bytecode_to_stmts(struct classblock *cb,
					    unsigned char *code, size_t count,
					    struct operand_stack *stack)
{
	struct converter *converter = &converters[code[0]];
	if (!converter || count < converter->require)
		return NULL;
	return converter->convert(cb, code, count, stack);
}
