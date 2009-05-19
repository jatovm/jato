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
#include <jit/bc-offset-mapping.h>

#include <vm/buffer.h>
#include <vm/class.h>
#include <vm/method.h>
#include <vm/string.h>
#include <vm/vm.h>

#include <arch/lir-printer.h>

#include "disass.h"

#include <stdbool.h>
#include <stdio.h>

bool opt_trace_method;
bool opt_trace_cfg;
bool opt_trace_tree_ir;
bool opt_trace_lir;
bool opt_trace_liveness;
bool opt_trace_regalloc;
bool opt_trace_machine_code;
bool opt_trace_magic_trampoline;
bool opt_trace_bytecode_offset;

void trace_method(struct compilation_unit *cu)
{
	struct vm_method *method = cu->method;
	unsigned char *p;
	unsigned int i;

	printf("\nTRACE: %s.%s%s\n",
		method->class->name, method->name, method->type);

	printf("Length: %d\n", method->code_attribute.code_length);
	printf("Code: ");
	p = method->code_attribute.code;
	for (i = 0; i < method->code_attribute.code_length; i++) {
		printf("%02x ", p[i]);
	}
	printf("\n\n");
}

void trace_cfg(struct compilation_unit *cu)
{
	struct basic_block *bb;

	printf("Control Flow Graph:\n\n");
	printf("  #:\t\tRange\t\tSuccessors\n");

	for_each_basic_block(bb, &cu->bb_list) {
		unsigned long i;

		printf("  %p\t%lu..%lu\t", bb, bb->start, bb->end);
		if (bb->is_eh)
			printf(" (eh)");

		for (i = 0; i < bb->nr_successors; i++) {
			if (i == 0)
				printf("\t");
			else
				printf(", ");

			printf("%p", bb->successors[i]);
		}

		printf("\n");
	}

	printf("\n");
}

void trace_tree_ir(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct statement *stmt;
	struct string *str;

	printf("High-Level Intermediate Representation (HIR):\n\n");

	for_each_basic_block(bb, &cu->bb_list) {
		printf("[bb %p]:\n\n", bb);
		for_each_stmt(stmt, &bb->stmt_list) {
			str = alloc_str();
			tree_print(&stmt->node, str);
			printf(str->value);
			free_str(str);
		}
		printf("\n");
	}
}

void trace_lir(struct compilation_unit *cu)
{
	unsigned long offset = 0;
	struct basic_block *bb;
	struct string *str;
	struct insn *insn;

	printf("Low-Level Intermediate Representation (LIR):\n\n");

	printf("Bytecode   LIR\n");
	printf("offset     offset    Instruction          Operands\n");
	printf("---------  -------   -----------          --------\n");

	for_each_basic_block(bb, &cu->bb_list) {
		for_each_insn(insn, &bb->insn_list) {
			str = alloc_str();
			lir_print(insn, str);

			if (opt_trace_bytecode_offset) {
				unsigned long bc_offset = insn->bytecode_offset;
				struct string *bc_str;

				bc_str = alloc_str();

				print_bytecode_offset(bc_offset, bc_str);

				printf("[ %5s ]  ", bc_str->value);
				free_str(bc_str);
			}

			printf(" %5lu:   %s\n", offset++, str->value);
			free_str(str);
		}
	}

	printf("\n");
}

static void
print_var_liveness(struct compilation_unit *cu, struct var_info *var)
{
	struct live_range *range = &var->interval->range;
	struct basic_block *bb;
	unsigned long offset;
	struct insn *insn;

	printf("  %2lu: ", var->vreg);

	offset = 0;
	for_each_basic_block(bb, &cu->bb_list) {
		for_each_insn(insn, &bb->insn_list) {
			if (in_range(range, offset)) {
				if (next_use_pos(var->interval, offset) == offset) {
					/* In use */
					printf("UUU");
				} else {
					if (var->interval->reg == REG_UNASSIGNED)
						printf("***");
					else
						printf("---");
				}
			}
			else
				printf("   ");

			offset++;
		}
	}
	if (!range_is_empty(range))
		printf(" (start: %2lu, end: %2lu)\n", range->start, range->end);
	else
		printf(" (empty)\n");
}

void trace_liveness(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct var_info *var;
	unsigned long offset;
	struct insn *insn;

	printf("Liveness:\n\n");

	printf("Legend: (U) In use, (-) Fixed register, (*) Non-fixed register\n\n");

	printf("      ");
	offset = 0;
	for_each_basic_block(bb, &cu->bb_list) {
		for_each_insn(insn, &bb->insn_list) {
			printf("%-2lu ", offset++);
		}
	}
	printf("\n");

	for_each_variable(var, cu->var_infos)
		print_var_liveness(cu, var);

	printf("\n");
}

void trace_regalloc(struct compilation_unit *cu)
{
	struct var_info *var;

	printf("Register Allocation:\n\n");

	for_each_variable(var, cu->var_infos) {
		struct live_interval *interval;

		for (interval = var->interval; interval != NULL; interval = interval->next_child) {
			printf("  %2lu (pos: %2ld-%2lu):", var->vreg, (signed long)interval->range.start, interval->range.end);
			printf("\t%s", reg_name(interval->reg));
			printf("\t%s", interval->fixed_reg ? "fixed\t" : "non-fixed");
			printf("\t%s", interval->need_spill ? "spill\t" : "no spill");
			printf("\t%s", interval->need_reload ? "reload\t" : "no reload");
			printf("\n");
		}
	}
	printf("\n");
}

void trace_machine_code(struct compilation_unit *cu)
{
	void *start, *end;

	printf("Disassembler Listing:\n\n");

	start  = buffer_ptr(cu->objcode);
	end = buffer_current(cu->objcode);

	disassemble(cu, start, end);
	printf("\n");
}

void trace_magic_trampoline(struct compilation_unit *cu)
{
	printf("jit_magic_trampoline: ret0=%p, ret1=%p: %s.%s #%d\n",
	       __builtin_return_address(1),
	       __builtin_return_address(2),
	       cu->method->class->name,
	       cu->method->name,
	       cu->method->method_index);
}
