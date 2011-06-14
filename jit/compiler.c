/*
 * Compiles bytecode methods to machine code.
 *
 * Copyright (C) 2005-2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "jit/compilation-unit.h"
#include "jit/compiler.h"
#include "jit/statement.h"
#include "jit/bc-offset-mapping.h"
#include "jit/exception.h"
#include "jit/perf-map.h"
#include "jit/subroutine.h"

#include "vm/class.h"
#include "vm/method.h"
#include "vm/trace.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void compile_error(struct compilation_unit *cu, int err)
{
	struct vm_method *method = cu->method;
	struct vm_class *class = method->class;

	error("Failed to compile method `%s' in class `%s', error: %i ('%s')",
		method->name, class->name, err, strerror(abs(err)));
}

#define SYMBOL_LEN 128

static void perf_append_cu(struct compilation_unit *cu)
{
	unsigned long addr, size;
	char symbol[SYMBOL_LEN];

	cu_symbol(cu, symbol, SYMBOL_LEN);

	addr = (unsigned long) cu_native_ptr(cu);
	size = cu_native_size(cu);

	perf_map_append(symbol, addr, size);
}

int compile(struct compilation_unit *cu)
{
	int err;

	if (opt_trace_compile)
		trace_method(cu);

	err = inline_subroutines(cu->method);
	if (err)
		goto out;

	if (opt_trace_bytecode)
		trace_bytecode(cu->method);

	err = analyze_control_flow(cu);
	if (err)
		goto out;

	err = convert_to_ir(cu);
	if (err)
		goto out;

	if (opt_ssa_enable) {
		err = compute_dfns(cu);
		if (err)
			goto out;
	}

	if (opt_trace_cfg)
		trace_cfg(cu);

	if (opt_trace_tree_ir)
		trace_tree_ir(cu);

	err = select_instructions(cu);
	if (err)
		goto out;

	compute_insn_positions(cu);

	if (opt_trace_lir)
		trace_lir(cu);

	if (opt_ssa_enable) {
		err = compute_dom(cu);
		if (err)
			goto out;

		err = compute_dom_frontier(cu);
		if (err)
			goto out;

		err = compute_ssa(cu);
		if (err)
			goto out;
	}

	err = analyze_liveness(cu);
	if (err)
		goto out;

	if (opt_trace_liveness)
		trace_liveness(cu);

	err = allocate_registers(cu);
	if (err)
		goto out;

	err = insert_spill_reload_insns(cu);
	if (err)
		goto out;

	if (opt_trace_regalloc)
		trace_regalloc(cu);

	assert(all_insn_have_bytecode_offset(cu));

	err = emit_machine_code(cu);
	if (err)
		goto out;

	err = build_bc_offset_map(cu);
	if (err)
		goto out;

	if (opt_trace_machine_code)
		trace_machine_code(cu);

	resolve_fixup_offsets(cu);

	perf_append_cu(cu);
  out:
	if (opt_trace_compile)
		trace_flush();

	if (err && !exception_occurred())
		compile_error(cu, err);

	return err;
}
