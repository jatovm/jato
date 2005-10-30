/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <statement.h>
#include <byteorder.h>
#include <operand-stack.h>

#include <stdlib.h>
#include <assert.h>

static void convert_nop(struct classblock *cb, unsigned char *code, size_t len,
			struct statement *stmt, struct operand_stack *stack)
{
	stmt->type = STMT_NOP;
}

static void convert_aconst_null(struct classblock *cb, unsigned char *code,
				size_t len, struct statement *stmt,
				struct operand_stack *stack)
{
	stmt->type = STMT_ASSIGN;
	stmt->s_left.o_const.type = CONST_REFERENCE;
}

static void convert_iconst(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 0);
	stmt->type = STMT_ASSIGN;
	stmt->s_left.o_const.type = CONST_INT;
	stmt->s_left.o_const.value = code[0]-OPC_ICONST_0;
}

static void convert_lconst(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 0);
	stmt->type = STMT_ASSIGN;
	stmt->s_left.o_const.type = CONST_LONG;
	stmt->s_left.o_const.value = code[0]-OPC_LCONST_0;
}

static void convert_fconst(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 0);
	stmt->type = STMT_ASSIGN;
	stmt->s_left.o_const.type = CONST_FLOAT;
	stmt->s_left.o_const.fvalue = code[0]-OPC_FCONST_0;
}

static void convert_dconst(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 0);
	stmt->type = STMT_ASSIGN;
	stmt->s_left.o_const.type = CONST_DOUBLE;
	stmt->s_left.o_const.fvalue = code[0]-OPC_DCONST_0;
}

static void convert_bipush(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	stmt->type = STMT_ASSIGN;
	stmt->s_left.o_const.type = CONST_INT;
	stmt->s_left.o_const.value = (char)code[1];
}

static void convert_sipush(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 2);
	stmt->type = STMT_ASSIGN;
	stmt->s_left.o_const.type = CONST_INT;
	stmt->s_left.o_const.value = (short) be16_to_cpu(*(u2*) &code[1]);
}

static void __convert_ldc(struct constant_pool *cp, unsigned long cp_idx,
			  struct statement *stmt)
{
	stmt->type = STMT_ASSIGN;
	u1 type = CP_TYPE(cp, cp_idx);
	ConstantPoolEntry entry = be64_to_cpu(CP_INFO(cp, cp_idx));
	switch (type) {
		case CONSTANT_Integer:
			stmt->s_left.o_const.type = CONST_INT;
			stmt->s_left.o_const.value = entry;
			break;
		case CONSTANT_Float:
			stmt->s_left.o_const.type = CONST_FLOAT;
			stmt->s_left.o_const.fvalue = *(float *) &entry;
			break;
		case CONSTANT_String:
			stmt->s_left.o_const.type = CONST_REFERENCE;
			stmt->s_left.o_const.value = entry;
			break;
		case CONSTANT_Long:
			stmt->s_left.o_const.type = CONST_LONG;
			stmt->s_left.o_const.value = entry;
			break;
		case CONSTANT_Double:
			stmt->s_left.o_const.type = CONST_DOUBLE;
			stmt->s_left.o_const.fvalue = *(double *) &entry;
			break;
		default:
			assert(!"unknown constant type");
	}
}

static void convert_ldc(struct classblock *cb, unsigned char *code, size_t len,
			struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	__convert_ldc(&cb->constant_pool, code[1], stmt);
	stack_push(stack, stmt->target);
}

static void convert_ldc_w(struct classblock *cb, unsigned char *code, size_t len,
			  struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 2);
	__convert_ldc(&cb->constant_pool, be16_to_cpu(*(u2*) &code[1]), stmt);
	stack_push(stack, stmt->target);
}

static void convert_ldc2_w(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 2);
	__convert_ldc(&cb->constant_pool, be16_to_cpu(*(u2*) &code[1]), stmt);
	stack_push(stack, stmt->target);
}

static void __convert_load(unsigned char lv_index, enum local_variable_type lv_type,
			   struct statement *stmt, struct operand_stack *stack)
{
	stmt->type = STMT_ASSIGN;
	stmt->s_left.o_type = OPERAND_LOCAL_VARIABLE;
	stmt->s_left.o_local.lv_type = lv_type;
	stmt->s_left.o_local.lv_index = lv_index;
	stack_push(stack, stmt->target);
}

static void convert_load(unsigned char *code, size_t len, enum local_variable_type lv_type,
			 struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	__convert_load(code[1], lv_type, stmt, stack);
}

static void convert_iload(struct classblock *cb, unsigned char *code, size_t len,
			  struct statement *stmt, struct operand_stack *stack)
{
	convert_load(code, len, LOCAL_VARIABLE_INT, stmt, stack);
}

static void convert_lload(struct classblock *cb, unsigned char *code, size_t len,
			  struct statement *stmt, struct operand_stack *stack)
{
	convert_load(code, len, LOCAL_VARIABLE_LONG, stmt, stack);
}

static void convert_fload(struct classblock *cb, unsigned char *code, size_t len,
			  struct statement *stmt, struct operand_stack *stack)
{
	convert_load(code, len, LOCAL_VARIABLE_FLOAT, stmt, stack);
}

static void convert_dload(struct classblock *cb, unsigned char *code, size_t len,
			  struct statement *stmt, struct operand_stack *stack)
{
	convert_load(code, len, LOCAL_VARIABLE_DOUBLE, stmt, stack);
}

static void convert_aload(struct classblock *cb, unsigned char *code, size_t len,
			  struct statement *stmt, struct operand_stack *stack)
{
	convert_load(code, len, LOCAL_VARIABLE_REFERENCE, stmt, stack);
}

static void convert_iload_x(struct classblock *cb, unsigned char *code, size_t len,
			    struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	__convert_load(code[0]-OPC_ILOAD_0, LOCAL_VARIABLE_INT, stmt, stack);
}

static void convert_lload_x(struct classblock *cb, unsigned char *code, size_t len,
			    struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	__convert_load(code[0]-OPC_LLOAD_0, LOCAL_VARIABLE_LONG, stmt, stack);
}

static void convert_fload_x(struct classblock *cb, unsigned char *code, size_t len,
			    struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	__convert_load(code[0]-OPC_FLOAD_0, LOCAL_VARIABLE_FLOAT, stmt, stack);
}

static void convert_dload_x(struct classblock *cb, unsigned char *code, size_t len,
			    struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	__convert_load(code[0]-OPC_DLOAD_0, LOCAL_VARIABLE_DOUBLE, stmt, stack);
}

static void convert_aload_x(struct classblock *cb, unsigned char *code, size_t len,
			    struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	__convert_load(code[0]-OPC_ALOAD_0, LOCAL_VARIABLE_REFERENCE, stmt, stack);
}

static void set_temporary_operand(struct operand *operand,
				  unsigned long temporary)
{
	operand->o_type = OPERAND_TEMPORARY;
	operand->o_temporary = temporary;
}

static void convert_iaload(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	unsigned long index = stack_pop(stack);
	unsigned long arrayref = stack_pop(stack);

	assert(len > 0);

	struct statement *assign = malloc(sizeof(struct statement));
	assert(assign);
	assign->type = STMT_ARRAY_ASSIGN;
	set_temporary_operand(&assign->s_left, arrayref);
	set_temporary_operand(&assign->s_right, index);

	stack_push(stack, assign->target);

	struct statement *arraycheck = malloc(sizeof(struct statement));
	assert(arraycheck);
	arraycheck->type = STMT_ARRAY_CHECK;
	set_temporary_operand(&arraycheck->s_left, arrayref);
	set_temporary_operand(&arraycheck->s_right, index);
	arraycheck->next = assign;

	stmt->type = STMT_NULL_CHECK;
	set_temporary_operand(&stmt->s_left, arrayref);
	stmt->next = arraycheck;
}

typedef void (*convert_fn_t)(struct classblock *, unsigned char *, size_t,
			     struct statement *, struct operand_stack *stack);

static convert_fn_t converters[] = {
	[OPC_NOP]		= convert_nop,
	[OPC_ACONST_NULL]	= convert_aconst_null,
	[OPC_ICONST_M1]		= convert_iconst,
	[OPC_ICONST_0]		= convert_iconst,
	[OPC_ICONST_1]		= convert_iconst,
	[OPC_ICONST_2]		= convert_iconst,
	[OPC_ICONST_3]		= convert_iconst,
	[OPC_ICONST_4]		= convert_iconst,
	[OPC_ICONST_5]		= convert_iconst,
	[OPC_LCONST_0]		= convert_lconst,
	[OPC_LCONST_1]		= convert_lconst,
	[OPC_FCONST_0]		= convert_fconst,
	[OPC_FCONST_1]		= convert_fconst,
	[OPC_FCONST_2]		= convert_fconst,
	[OPC_DCONST_0]		= convert_dconst,
	[OPC_DCONST_1]		= convert_dconst,
	[OPC_BIPUSH]		= convert_bipush,
	[OPC_SIPUSH]		= convert_sipush,
	[OPC_LDC]		= convert_ldc,
	[OPC_LDC_W]		= convert_ldc_w,
	[OPC_LDC2_W]		= convert_ldc2_w,
	[OPC_ILOAD]		= convert_iload,
	[OPC_LLOAD]		= convert_lload,
	[OPC_FLOAD]		= convert_fload,
	[OPC_DLOAD]		= convert_dload,
	[OPC_ALOAD]		= convert_aload,
	[OPC_ILOAD_0]		= convert_iload_x,
	[OPC_ILOAD_1]		= convert_iload_x,
	[OPC_ILOAD_2]		= convert_iload_x,
	[OPC_ILOAD_3]		= convert_iload_x,
	[OPC_LLOAD_0]		= convert_lload_x,
	[OPC_LLOAD_1]		= convert_lload_x,
	[OPC_LLOAD_2]		= convert_lload_x,
	[OPC_LLOAD_3]		= convert_lload_x,
	[OPC_FLOAD_0]		= convert_fload_x,
	[OPC_FLOAD_1]		= convert_fload_x,
	[OPC_FLOAD_2]		= convert_fload_x,
	[OPC_FLOAD_3]		= convert_fload_x,
	[OPC_DLOAD_0]		= convert_dload_x,
	[OPC_DLOAD_1]		= convert_dload_x,
	[OPC_DLOAD_2]		= convert_dload_x,
	[OPC_DLOAD_3]		= convert_dload_x,
	[OPC_ALOAD_0]		= convert_aload_x,
	[OPC_ALOAD_1]		= convert_aload_x,
	[OPC_ALOAD_2]		= convert_aload_x,
	[OPC_ALOAD_3]		= convert_aload_x,
	[OPC_IALOAD]		= convert_iaload,
};

static void convert_to_stmt(struct classblock *cb, unsigned char *code,
			    size_t count, struct statement *stmt,
			    struct operand_stack *stack)
{
	assert(count > 0);
	convert_fn_t convert = converters[code[0]];
	assert(convert);
	convert(cb, code, count, stmt, stack);
}

struct statement *stmt_from_bytecode(struct classblock *cb,
				     unsigned char *code, size_t count,
				     struct operand_stack *stack)
{
	struct statement *ret = malloc(sizeof(*ret));
	if (!ret)
		return NULL;

	convert_to_stmt(cb, code, count, ret, stack);

	return ret;
}
