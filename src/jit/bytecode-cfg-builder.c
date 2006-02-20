/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jam.h>
#include <jit-compiler.h>
#include <bytecodes.h>

void build_cfg(struct compilation_unit *cu)
{
	struct basic_block *bb;
	unsigned long prev, offset = 0;

	cu->entry_bb = bb = alloc_basic_block(offset, cu->code_len);
	while (offset < cu->code_len) {
		prev = offset;
		offset += bytecode_size(cu->code + offset);

		if (bytecode_is_branch(cu->code[prev]))
			bb_split(bb, offset);
	}
}
