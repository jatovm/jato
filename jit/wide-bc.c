/*
 * Copyright (c) 2009  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "jit/bytecode-to-ir.h"
#include "jit/compiler.h"

#include "vm/bytecode.h"

int convert_wide(struct parse_context *ctx)
{
	int result;

	ctx->is_wide = true;
	result = convert_instruction(ctx);
	ctx->is_wide = false;

	return result;
}
