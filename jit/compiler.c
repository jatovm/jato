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

int compile(struct compilation_unit *cu)
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

	err = insert_spill_reload_insns(cu);
	if (err)
		goto out;

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
