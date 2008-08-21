/*
 * Tracing for the JIT compiler
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/basic-block.h>
#include <jit/compilation-unit.h>
#include <jit/statement.h>
#include <jit/tree-printer.h>
#include <jit/vars.h>

#include <vm/buffer.h>
#include <vm/string.h>
#include <vm/vm.h>

#include "disass.h"

#include <stdbool.h>
#include <stdio.h>

bool opt_trace_method;
bool opt_trace_cfg;
bool opt_trace_tree_ir;
bool opt_trace_liveness;
bool opt_trace_regalloc;
bool opt_trace_machine_code;

void trace_method(struct compilation_unit *cu)
{
	struct methodblock *method = cu->method;
	unsigned char *p;
	int i;

	printf("\nTRACE: %s.%s%s\n", CLASS_CB(method->class)->name, method->name, method->type);

	printf("Length: %d\n", method->code_size);
	printf("Code: ");
	p = method->jit_code;
	for (i = 0; i < method->code_size; i++) {
		printf("%02x ", p[i]);
	}
	printf("\n\n");
}

void trace_cfg(struct compilation_unit *cu)
{
	struct basic_block *bb;

	printf("Control Flow Graph:\n\n");
	printf("  #:\t\tRange\n");

	for_each_basic_block(bb, &cu->bb_list) {
		printf("  %p\t%lu..%lu", bb, bb->start, bb->end);
		if (bb->is_eh)
			printf(" (eh)\n");
		printf("\n");
	}

	printf("\n");
}

void trace_tree_ir(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct statement *stmt;
	struct string *str;

	printf("High-Level Intermediate Representation:\n\n");

	for_each_basic_block(bb, &cu->bb_list) {
		for_each_stmt(stmt, &bb->stmt_list) {
			str = alloc_str();
			tree_print(&stmt->node, str);
			printf(str->value);
			free_str(str);
		}
		printf("\n");
	}
}

void trace_liveness(struct compilation_unit *cu)
{
	unsigned long offset;
	struct basic_block *bb;
	struct var_info *var;
	struct insn *insn;

	printf("Liveness:\n\n");

	printf("      ");
	offset = 0;
	for_each_basic_block(bb, &cu->bb_list) {
		for_each_insn(insn, &bb->insn_list) {
			printf("%-2lu ", offset++);
		}
	}
	printf("\n");

	for_each_variable(var, cu->var_infos) {
		struct live_range *range = &var->interval->range;

		printf("  %2lu: ", var->vreg);

		offset = 0;
		for_each_basic_block(bb, &cu->bb_list) {
			for_each_insn(insn, &bb->insn_list) {
				if (in_range(range, offset++)) {
					if (var->interval->reg == REG_UNASSIGNED)
						printf("***");
					else
						printf("---");
				}
				else
					printf("   ");
			}
		}
		if (!range_is_empty(range))
			printf(" (start: %2lu, end: %2lu)\n", range->start, range->end);
		else
			printf(" (empty)\n");
	}
	printf("\n");
}

void trace_regalloc(struct compilation_unit *cu)
{
	struct var_info *var;

	printf("Register Allocation:\n\n");

	for_each_variable(var, cu->var_infos) {
		printf("  %2lu: %s", var->vreg, reg_name(var->interval->reg));
		if (var->interval->fixed_reg)
			printf(" (fixed)");
		printf("\n");
	}
	printf("\n");
}

void trace_machine_code(struct compilation_unit *cu)
{
	void *start, *end;

	printf("Disassembler Listing:\n\n");

	start  = buffer_ptr(cu->objcode);
	end = buffer_current(cu->objcode);

	disassemble(start, end);
	printf("\n");
}
