/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <jam.h>
#include <statement.h>

#include <stdlib.h>
#include <assert.h>

static void convert_to_stmt(char code[], size_t count, struct statement *stmt)
{
	assert(count > 0);
	char opc = code[0];
	switch (opc) {
		case OPC_NOP:
			stmt->type = STMT_NOP;
			break;
		case OPC_ACONST_NULL:
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_NULL;
			break;
		case OPC_ICONST_M1:
		case OPC_ICONST_0:
		case OPC_ICONST_1:
		case OPC_ICONST_2:
		case OPC_ICONST_3:
		case OPC_ICONST_4:
		case OPC_ICONST_5:
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_INT;
			stmt->operand.value = opc-OPC_ICONST_0;
			break;
		case OPC_LCONST_0:
		case OPC_LCONST_1:
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_LONG;
			stmt->operand.value = opc-OPC_LCONST_0;
			break;
		case OPC_FCONST_0:
		case OPC_FCONST_1:
		case OPC_FCONST_2:
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_FLOAT;
			stmt->operand.fvalue = opc-OPC_FCONST_0;
			break;
		case OPC_DCONST_0:
		case OPC_DCONST_1:
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_DOUBLE;
			stmt->operand.fvalue = opc-OPC_DCONST_0;
			break;
		case OPC_BIPUSH:
			assert(count > 1);
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_INT;
			stmt->operand.value = code[1];
			break;
		case OPC_SIPUSH:
			assert(count > 2);
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_INT;
			stmt->operand.value = code[1] << 8 | (unsigned char)code[2];
			break;
		case OPC_LDC_QUICK:
			assert(count > 1);
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_INT;
			stmt->operand.value = code[1];
			break;
	};
}

struct statement *stmt_from_bytecode(char code[], size_t count)
{
	struct statement *ret = malloc(sizeof(*ret));
	if (!ret)
		return NULL;

	convert_to_stmt(code, count, ret);

	return ret;
}
