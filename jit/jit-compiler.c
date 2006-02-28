/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <alloc.h>
#include <compilation-unit.h>
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

struct compilation_unit *jit_compile(unsigned char *bytecode, unsigned long size)
{
	struct stack expr_stack = STACK_INIT;
	struct compilation_unit *cu = alloc_compilation_unit(bytecode, size, NULL, &expr_stack);
	if (!cu)
		return NULL;
		
	build_cfg(cu);
	convert_to_ir(cu);
	cu->objcode = alloc_exec(OBJCODE_SIZE);
	if (!cu->objcode) {
		free_compilation_unit(cu);
		return NULL;
	}
	memset(cu->objcode, 0, OBJCODE_SIZE);
	insn_select(cu->entry_bb);
	x86_emit_prolog(cu->objcode, OBJCODE_SIZE);
	x86_emit_obj_code(cu->entry_bb, cu->objcode+3, OBJCODE_SIZE-3);
	x86_emit_epilog(cu->objcode+9, OBJCODE_SIZE-9);

	return cu;
}
