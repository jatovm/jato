/*
 * Compiles bytecode methods to machine code.
 *
 * Copyright (C) 2005-2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/compilation-unit.h>
#include <jit/compiler.h>
#include <jit/statement.h>
#include <jit/tree-printer.h>

#include <vm/buffer.h>
#include <vm/natives.h>
#include <vm/string.h>

#include <arch/emit-code.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void compile_error(struct compilation_unit *cu, int err)
{
	struct classblock *cb = CLASS_CB(cu->method->class);

	printf("%s: Failed to compile method `%s' in class `%s', error: %i\n",
	       __FUNCTION__, cu->method->name, cb->name, err);
}

static int compile(struct compilation_unit *cu)
{
	int err;

	if (opt_trace_method)
		trace_method(cu);

	err = analyze_control_flow(cu);
	if (err)
		goto out;

	if (opt_trace_cfg)
		trace_cfg(cu);

	err = convert_to_ir(cu);
	if (err)
		goto out;

	if (opt_trace_tree_ir)
		trace_tree_ir(cu);

	err = select_instructions(cu);
	if (err)
		goto out;

	compute_insn_positions(cu);

	err = analyze_liveness(cu);
	if (err)
		goto out;

	if (opt_trace_liveness)
		trace_liveness(cu);

	err = allocate_registers(cu);
	if (err)
		goto out;

	if (opt_trace_regalloc)
		trace_regalloc(cu);

	err = emit_machine_code(cu);
	if (err)
		goto out;

	if (opt_trace_machine_code)
		trace_machine_code(cu);

	cu->is_compiled = true;

  out:
	if (err)
		compile_error(cu, err);

	return err;
}

static void *jit_native_trampoline(struct compilation_unit *cu)
{
	struct methodblock *method = cu->method;
	const char *method_name, *class_name;

	class_name  = CLASS_CB(method->class)->name;
	method_name = method->name;

	return vm_lookup_native(class_name, method_name);
}

static void *jit_java_trampoline(struct compilation_unit *cu)
{
	if (!cu->is_compiled)
		compile(cu);

	return buffer_ptr(cu->objcode);
}

void *jit_magic_trampoline(struct compilation_unit *cu)
{
	void *ret;

	pthread_mutex_lock(&cu->mutex);

	if (cu->method->access_flags & ACC_NATIVE)
		ret = jit_native_trampoline(cu);
	else
		ret = jit_java_trampoline(cu);

	pthread_mutex_unlock(&cu->mutex);

	return ret;
}

struct jit_trampoline *build_jit_trampoline(struct compilation_unit *cu)
{
	struct jit_trampoline *trampoline;
	
	trampoline = alloc_jit_trampoline();
	if (trampoline)
		emit_trampoline(cu, jit_magic_trampoline, trampoline->objcode);
	return trampoline;
}

int jit_prepare_method(struct methodblock *mb)
{
	mb->compilation_unit = alloc_compilation_unit(mb);
	if (!mb->compilation_unit)
		return -ENOMEM;

	mb->trampoline = build_jit_trampoline(mb->compilation_unit);
	if (!mb->trampoline) {
		free_compilation_unit(mb->compilation_unit);
		return -ENOMEM;
	}
	return 0;
}
