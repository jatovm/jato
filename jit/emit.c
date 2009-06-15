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
#include <vm/method.h>
#include <vm/vm.h>

#include <jit/compilation-unit.h>
#include <jit/basic-block.h>
#include <jit/emit-code.h>
#include <jit/compiler.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void emit_monitorenter(struct compilation_unit *cu)
{
	if (method_is_static(cu->method))
		emit_lock(cu->objcode, cu->method->class);
	else
		emit_lock_this(cu->objcode);
}

static void emit_monitorexit(struct compilation_unit *cu)
{
	if (method_is_static(cu->method))
		emit_unlock(cu->objcode, cu->method->class);
	else
		emit_unlock_this(cu->objcode);
}

static struct buffer_operations exec_buf_ops = {
	.expand = expand_buffer_exec,
	.free   = generic_buffer_free,
};

int emit_machine_code(struct compilation_unit *cu)
{
	unsigned long frame_size;
	struct basic_block *bb;

	cu->objcode = __alloc_buffer(&exec_buf_ops);
	if (!cu->objcode)
		return -ENOMEM;

	frame_size = frame_locals_size(cu->stack_frame);

	emit_prolog(cu->objcode, frame_size);
	if (method_is_synchronized(cu->method))
		emit_monitorenter(cu);

	for_each_basic_block(bb, &cu->bb_list)
		emit_body(bb, cu->objcode);

	emit_body(cu->exit_bb, cu->objcode);
	if (method_is_synchronized(cu->method))
		emit_monitorexit(cu);
	cu->exit_past_unlock_ptr = buffer_current(cu->objcode);
	emit_epilog(cu->objcode);

	emit_body(cu->unwind_bb, cu->objcode);
	if (method_is_synchronized(cu->method))
		emit_monitorexit(cu);
	cu->unwind_past_unlock_ptr = buffer_current(cu->objcode);
	emit_unwind(cu->objcode);

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

	INIT_LIST_HEAD(&trampoline->fixup_site_list);
	pthread_mutex_init(&trampoline->mutex, NULL);

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
