/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <errno.h>
#include <jit/compilation-unit.h>
#include <jit/insn-selector.h>
#include <jit/jit-compiler.h>
#include <jit/statement.h>
#include <jit/tree-printer.h>
#include <vm/alloc.h>
#include <vm/buffer.h>
#include <vm/natives.h>
#include <vm/string.h>
#include <x86-objcode.h>

#include "disass.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OBJCODE_SIZE 256

int show_basic_blocks;
int show_tree;
int show_disasm;

static void print_method_info(struct methodblock *method)
{
	printf("Method: %s, Class: %s\n", method->name, CLASS_CB(method->class)->name);
}

static void print_disasm(struct methodblock *method, void *start, void *end)
{
	print_method_info(method);
	disassemble(start, end);
	printf("\n");
}

static void print_basic_blocks(struct compilation_unit *cu)
{
	struct basic_block *bb;

	print_method_info(cu->method);

	list_for_each_entry(bb, &cu->bb_list, bb_list_node) {
		printf("BB %p, start: %lu, end: %lu\n", bb, bb->start, bb->end);
	}
}

static void print_tree(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct statement *stmt;
	struct string *str;
	
	print_method_info(cu->method);

	list_for_each_entry(bb, &cu->bb_list, bb_list_node) {
		printf("BB %p:\n", bb);
		list_for_each_entry(stmt, &bb->stmt_list, stmt_list_node) {
			str = alloc_str();
			tree_print(&stmt->node, str);
			printf(str->value);
			printf("\n");
			free_str(str);
		}
	}

}

static struct buffer_operations exec_buf_ops = {
	.expand = expand_exec,
	.free   = generic_buffer_free,
};

int jit_compile(struct compilation_unit *cu)
{
	struct basic_block *bb;
	int err = 0;

	err = build_cfg(cu);
	if (err)
		goto out;

	err = convert_to_ir(cu);
	if (err)
		goto out;

	if (show_basic_blocks)
		print_basic_blocks(cu);

	if (show_tree)
		print_tree(cu);

	list_for_each_entry(bb, &cu->bb_list, bb_list_node) {
		insn_select(bb);
	}

	cu->objcode_buf = __alloc_buffer(OBJCODE_SIZE, &exec_buf_ops);
	if (!cu->objcode_buf) {
		err = -ENOMEM;
		goto out;
	}
	x86_emit_prolog(cu->objcode_buf, 0);
	list_for_each_entry(bb, &cu->bb_list, bb_list_node) {
		x86_emit_obj_code(bb, cu->objcode_buf);
	}
	x86_emit_obj_code(cu->exit_bb, cu->objcode_buf);
	x86_emit_epilog(cu->objcode_buf, 0);

	if (show_disasm)
		print_disasm(cu->method,
			     buffer_ptr(cu->objcode_buf),
			     buffer_current(cu->objcode_buf));

	cu->is_compiled = true;

  out:
	if (err) {
		struct classblock *cb = CLASS_CB(cu->method->class);
		printf("%s: Failed to compile method `%s' in class `%s', error: %i\n",
		       __FUNCTION__, cu->method->name, cb->name, err);
	}

	return err;
}

void *jit_magic_trampoline(struct compilation_unit *cu)
{
	void *ret;

	pthread_mutex_lock(&cu->mutex);

	if (cu->method->access_flags & ACC_NATIVE) {
		struct methodblock *method = cu->method;
		const char *method_name, *class_name;

		class_name  = CLASS_CB(method->class)->name;
		method_name = method->name;

		ret = vm_lookup_native(class_name, method_name);
	}
	else if (!cu->is_compiled) {
		jit_compile(cu);
		ret = buffer_ptr(cu->objcode_buf);
	}
	else
		ret = buffer_ptr(cu->objcode_buf);

	pthread_mutex_unlock(&cu->mutex);

	return ret;
}

#define TRAMP_OBJSIZE 15

static struct jit_trampoline *alloc_jit_trampoline(void)
{
	struct jit_trampoline *tramp = malloc(sizeof(*tramp));
	if (!tramp)
		return NULL;

	memset(tramp, 0, sizeof(*tramp));

	tramp->objcode = __alloc_buffer(TRAMP_OBJSIZE, &exec_buf_ops);
	if (!tramp->objcode)
		goto failed;

	return tramp;

  failed:
	free_jit_trampoline(tramp);
	return NULL;
}

void free_jit_trampoline(struct jit_trampoline *tramp)
{
	free_buffer(tramp->objcode);
	free(tramp);
}

struct jit_trampoline *build_jit_trampoline(struct compilation_unit *cu)
{
	struct jit_trampoline *tramp = alloc_jit_trampoline();
	if (tramp)
		x86_emit_trampoline(cu, jit_magic_trampoline,
				    tramp->objcode,
				    TRAMP_OBJSIZE);
	return tramp;
}

void *jit_prepare_for_exec(struct methodblock *mb)
{
	mb->compilation_unit = alloc_compilation_unit(mb);
	mb->trampoline = build_jit_trampoline(mb->compilation_unit);

	return trampoline_ptr(mb);
}
