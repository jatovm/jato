/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jam.h>
#include <jit-compiler.h>

void build_cfg(struct compilation_unit *cu)
{
	struct basic_block *bb;
	unsigned long offset = 0;

	cu->entry_bb = bb = alloc_basic_block(offset, cu->code_len);
	while (offset < cu->code_len) {
		unsigned long prev_offset = offset;
		offset += bytecode_sizes[cu->code[offset]];
		if (cu->code[prev_offset] == OPC_IFNONNULL)
			bb_split(bb, offset);
	}
}
