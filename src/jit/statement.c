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
	stmt->operand.o_const.type = CONST_REFERENCE;
}

static void convert_iconst(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 0);
	stmt->type = STMT_ASSIGN;
	stmt->operand.o_const.type = CONST_INT;
	stmt->operand.o_const.value = code[0]-OPC_ICONST_0;
}

static void convert_lconst(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 0);
	stmt->type = STMT_ASSIGN;
	stmt->operand.o_const.type = CONST_LONG;
	stmt->operand.o_const.value = code[0]-OPC_LCONST_0;
}

static void convert_fconst(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 0);
	stmt->type = STMT_ASSIGN;
	stmt->operand.o_const.type = CONST_FLOAT;
	stmt->operand.o_const.fvalue = code[0]-OPC_FCONST_0;
}

static void convert_dconst(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 0);
	stmt->type = STMT_ASSIGN;
	stmt->operand.o_const.type = CONST_DOUBLE;
	stmt->operand.o_const.fvalue = code[0]-OPC_DCONST_0;
}

static void convert_bipush(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	stmt->type = STMT_ASSIGN;
	stmt->operand.o_const.type = CONST_INT;
	stmt->operand.o_const.value = (char)code[1];
}

static void convert_sipush(struct classblock *cb, unsigned char *code, size_t len,
			   struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 2);
	stmt->type = STMT_ASSIGN;
	stmt->operand.o_const.type = CONST_INT;
	stmt->operand.o_const.value = (short) be16_to_cpu(*(u2*) &code[1]);
}

static void __convert_ldc(struct constant_pool *cp, unsigned long cp_idx,
			  struct statement *stmt)
{
	stmt->type = STMT_ASSIGN;
	u1 type = CP_TYPE(cp, cp_idx);
	ConstantPoolEntry entry = be64_to_cpu(CP_INFO(cp, cp_idx));
	switch (type) {
		case CONSTANT_Integer:
			stmt->operand.o_const.type = CONST_INT;
			stmt->operand.o_const.value = entry;
			break;
		case CONSTANT_Float:
			stmt->operand.o_const.type = CONST_FLOAT;
			stmt->operand.o_const.fvalue = *(float *) &entry;
			break;
		case CONSTANT_String:
			stmt->operand.o_const.type = CONST_REFERENCE;
			stmt->operand.o_const.value = entry;
			break;
		case CONSTANT_Long:
			stmt->operand.o_const.type = CONST_LONG;
			stmt->operand.o_const.value = entry;
			break;
		case CONSTANT_Double:
			stmt->operand.o_const.type = CONST_DOUBLE;
			stmt->operand.o_const.fvalue = *(double *) &entry;
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

static void convert_iload(struct classblock *cb, unsigned char *code, size_t len,
			  struct statement *stmt, struct operand_stack *stack)
{
	assert(len > 1);
	stmt->type = STMT_ASSIGN;
	stmt->operand.o_type = OPERAND_LOCAL_VARIABLE;
	stmt->operand.o_local.lv_index = code[1];
	stack_push(stack, stmt->target);
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
