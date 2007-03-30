/*
 * Compiles bytecode methods to machine code.
 *
 * Copyright (C) 2005-2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <errno.h>
#include <jit/compilation-unit.h>
#include <jit/jit-compiler.h>
#include <jit/statement.h>
#include <jit/tree-printer.h>
#include <vm/buffer.h>
#include <vm/natives.h>
#include <vm/string.h>
#include <x86-objcode.h>

#include "disass.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int debug_tree;
int debug_disassembly;

static void show_method_info(struct methodblock *method)
{
	printf("Method: %s, Class: %s\n", method->name, CLASS_CB(method->class)->name);
}

static void show_disassembly(struct compilation_unit *cu)
{
	struct methodblock *method = cu->method;
	void *start = buffer_ptr(cu->objcode);
	void *end = buffer_current(cu->objcode);

	show_method_info(method);
	disassemble(start, end);
	printf("\n");
}

static void show_tree(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct statement *stmt;
	struct string *str;
	
	show_method_info(cu->method);

	for_each_basic_block(bb, &cu->bb_list) {
		printf("BB %p, start: %lu, end: %lu\n", bb, bb->start, bb->end);
		for_each_stmt(stmt, &bb->stmt_list) {
			str = alloc_str();
			tree_print(&stmt->node, str);
			printf(str->value);
			printf("\n");
			free_str(str);
		}
	}
}

static void compile_error(struct compilation_unit *cu, int err)
{
	struct classblock *cb = CLASS_CB(cu->method->class);

	printf("%s: Failed to compile method `%s' in class `%s', error: %i\n",
	       __FUNCTION__, cu->method->name, cb->name, err);
}

int jit_compile(struct compilation_unit *cu)
{
	int err;

	err = analyze_control_flow(cu);
	if (err)
		goto out;

	err = convert_to_ir(cu);
	if (err)
		goto out;

	if (debug_tree)
		show_tree(cu);

	err = select_instructions(cu);
	if (err)
		goto out;

	err = emit_machine_code(cu);
	if (err)
		goto out;

	if (debug_disassembly)
		show_disassembly(cu);

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
		jit_compile(cu);

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

void *jit_prepare_for_exec(struct methodblock *mb)
{
	mb->compilation_unit = alloc_compilation_unit(mb);
	mb->trampoline = build_jit_trampoline(mb->compilation_unit);

	return trampoline_ptr(mb);
}
