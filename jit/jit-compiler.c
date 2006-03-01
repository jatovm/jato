/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <alloc.h>
#include <compilation-unit.h>
#include <errno.h>
#include <jit-compiler.h>
#include <statement.h>
#include <insn-selector.h>
#include <x86-objcode.h>

#include <stdio.h>
#include <string.h>

#if 0
static void dump_objcode(unsigned char *buffer, unsigned long size)
{
	unsigned long i, col;

	col = 0;
	for (i = 0; i < size; i++) {
		printf("%02x ", buffer[i]);
		if (col++ == 15) {
			printf("\n");
			col = 0;
		}
	}
	printf("\n");
}
#endif

#define OBJCODE_SIZE 256

int jit_compile(struct compilation_unit *cu)
{
	struct insn_sequence is;

	build_cfg(cu);
	convert_to_ir(cu);
	insn_select(bb_entry(cu->bb_list.next));

	cu->objcode = alloc_exec(OBJCODE_SIZE);
	if (!cu->objcode) {
		free_compilation_unit(cu);
		return -ENOMEM;
	}
	memset(cu->objcode, 0, OBJCODE_SIZE);
	init_insn_sequence(&is, cu->objcode, OBJCODE_SIZE);
	x86_emit_prolog(&is);
	x86_emit_obj_code(bb_entry(cu->bb_list.next), &is);
	x86_emit_epilog(&is);

	cu->is_compiled = true;

	return 0;
}

void jit_magic_trampoline(struct compilation_unit *cu)
{
	pthread_mutex_lock(&cu->mutex);

	if (!cu->is_compiled)
		jit_compile(cu);

	pthread_mutex_unlock(&cu->mutex);
}
