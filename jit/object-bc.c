/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode arithmetic
 * instructions to immediate representation of the JIT compiler.
 */

#include <statement.h>
#include <byteorder.h>
#include <stack.h>
#include <jit-compiler.h>
#include <bytecodes.h>
#include <errno.h>
#include <vm/const-pool.h>

int convert_getstatic(struct compilation_unit *cu, struct basic_block *bb,
		      unsigned long offset)
{
	struct fieldblock *fb;
	struct classblock *cb;
	struct constant_pool *cp;
	unsigned short index;
	struct expression *value;
	u1 type;

	cb = CLASS_CB(cu->method->class);
	cp = &cb->constant_pool;
	index = cp_index(cu->method->code + offset + 1);
	type = CP_TYPE(cp, index);
	
	if (type != CONSTANT_Resolved)
		return -EINVAL;

	fb = cp_info_ptr(cp, index);
	value = field_expr(str_to_type(fb->type), fb);
	if (!value)
		return -ENOMEM;

	stack_push(cu->expr_stack, value);
	return 0;
}

int convert_putstatic(struct compilation_unit *cu, struct basic_block *bb,
		      unsigned long offset)
{
	struct fieldblock *fb;
	struct classblock *cb;
	struct constant_pool *cp;
	unsigned short index;
	struct statement *store_stmt;
	struct expression *dest, *src;
	u1 type;

	cb = CLASS_CB(cu->method->class);
	cp = &cb->constant_pool;
	index = cp_index(cu->method->code + offset + 1);
	type = CP_TYPE(cp, index);
	
	if (type != CONSTANT_Resolved)
		return -EINVAL;

	fb = cp_info_ptr(cp, index);
	src = stack_pop(cu->expr_stack);
	dest = field_expr(str_to_type(fb->type), fb);
	if (!dest)
		return -ENOMEM;
	
	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt) {
		expr_put(dest);
		return -ENOMEM;
	}
	store_stmt->store_dest = &dest->node;
	store_stmt->store_src = &src->node;
	bb_insert_stmt(bb, store_stmt);
	
	return 0;
}
