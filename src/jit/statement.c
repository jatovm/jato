/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <statement.h>

#include <stdlib.h>
#include <assert.h>

static void convert_to_stmt(struct classblock *cb, unsigned char *code, size_t count, struct statement *stmt)
{
	assert(count > 0);
	unsigned char opc = code[0];
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
			stmt->operand.value = (char)code[1];
			break;
		case OPC_SIPUSH:
			assert(count > 2);
			stmt->type = STMT_ASSIGN;
			stmt->operand.type = CONST_INT;
			stmt->operand.value = (short)(code[1] << 8 | code[2]);
			break;
		case OPC_LDC_QUICK:
			assert(count > 1);
			unsigned char cp_idx = code[1];
			struct constant_pool *cp = &cb->constant_pool;
			stmt->type = STMT_ASSIGN;
			if (CP_TYPE(cp, cp_idx) == CONSTANT_Integer) {
				stmt->operand.type = CONST_INT;
				stmt->operand.value = CP_INFO(cp, cp_idx);
			} else {
				stmt->operand.type = CONST_FLOAT;
				union {
					u4 value;
					float fvalue;
				} val;
				val.value = CP_INFO(cp, cp_idx);
				stmt->operand.fvalue = val.fvalue;
			}
			break;
	};
}

struct statement *stmt_from_bytecode(struct classblock *cb, unsigned char *code, size_t count)
{
	struct statement *ret = malloc(sizeof(*ret));
	if (!ret)
		return NULL;

	convert_to_stmt(cb, code, count, ret);

	return ret;
}
