/*
 * Emits machine code.
 *
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/alloc.h>
#include <vm/buffer.h>
#include <vm/vm.h>

#include <jit/basic-block.h>
#include <jit/compilation-unit.h>
#include <jit/jit-compiler.h>

#include <arch/emit-code.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static struct buffer_operations exec_buf_ops = {
	.expand = expand_buffer_exec,
	.free   = generic_buffer_free,
};

int emit_machine_code(struct compilation_unit *cu)
{
	struct basic_block *bb;

	cu->objcode = __alloc_buffer(&exec_buf_ops);
	if (!cu->objcode)
		return -ENOMEM;

	emit_prolog(cu->objcode, cu->method->max_locals);
	for_each_basic_block(bb, &cu->bb_list)
		emit_body(bb, cu->objcode);

	emit_body(cu->exit_bb, cu->objcode);
	emit_epilog(cu->objcode, cu->method->max_locals);

	return 0;
}

struct jit_trampoline *alloc_jit_trampoline(void)
{
	struct jit_trampoline *trampoline;
	
	trampoline = malloc(sizeof(*trampoline));
	if (!trampoline)
		return NULL;

	memset(trampoline, 0, sizeof(*trampoline));

	trampoline->objcode = __alloc_buffer(&exec_buf_ops);
	if (!trampoline->objcode)
		goto failed;

	return trampoline;

  failed:
	free_jit_trampoline(trampoline);
	return NULL;
}

void free_jit_trampoline(struct jit_trampoline *trampoline)
{
	free_buffer(trampoline->objcode);
	free(trampoline);
}
