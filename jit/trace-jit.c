/*
 * Tracing for the JIT compiler
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "jit/bc-offset-mapping.h"
#include "jit/compilation-unit.h"
#include "jit/tree-printer.h"
#include "jit/basic-block.h"
#include "jit/disassemble.h"
#include "jit/lir-printer.h"
#include "jit/exception.h"
#include "jit/cu-mapping.h"
#include "jit/statement.h"
#include "jit/vars.h"
#include "jit/args.h"
#include "vm/preload.h"
#include "vm/object.h"

#include "lib/buffer.h"
#include "vm/class.h"
#include "vm/method.h"
#include "lib/string.h"
#include "vm/vm.h"

#include "arch/stack-frame.h"

#include <malloc.h>
#include <stdlib.h>
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
bool opt_trace_invoke;
bool opt_trace_invoke_verbose;
bool opt_trace_exceptions;

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
	printf("  #:\t\tRange\t\tSuccessors\t\tPredecessors\n");

	for_each_basic_block(bb, &cu->bb_list) {
		unsigned long i;

		printf("  %p\t%lu..%lu\t", bb, bb->start, bb->end);
		if (bb->is_eh)
			printf(" (eh)");

		printf("\t");

		for (i = 0; i < bb->nr_successors; i++) {
			if (i != 0)
				printf(", ");

			printf("%p", bb->successors[i]);
		}

		if (i == 0)
			printf("none    ");

		printf("\t");

		for (i = 0; i < bb->nr_predecessors; i++) {
			if (i != 0)
				printf(", ");

			printf("%p", bb->predecessors[i]);
		}

		if (i == 0)
			printf("none    ");

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
			printf("%s", str->value);
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

static void print_arg(enum vm_type arg_type, const unsigned long *args,
		      int *arg_index)
{
	if (arg_type == J_LONG || arg_type == J_DOUBLE) {
		unsigned long long value;

		value = *(unsigned long long*)(args + *arg_index);
		(*arg_index) += 2;

		printf("0x%llx", value);
		return;
	}

	printf("0x%lx ", args[*arg_index]);

	if (arg_type == J_REFERENCE) {
		struct vm_object *obj;

		obj = (struct vm_object *)args[*arg_index];

		if (!obj) {
			printf("null");
			goto out;
		}

		if (!is_on_heap((unsigned long)obj)) {
			printf("*** pointer not on heap ***");
			goto out;
		}

		if (obj->class == vm_java_lang_String) {
			char *str;

			str = vm_string_to_cstr(obj);
			printf("= \"%s\"", str);
			free(str);
		}

		printf(" (%s)", obj->class->name);
	}

 out:
	(*arg_index)++;
	printf("\n");
}

static void trace_invoke_args(struct vm_method *vmm,
			      struct jit_stack_frame *frame)
{
	enum vm_type arg_type;
	const char *type_str;
	int arg_index;

	arg_index = 0;

	if (!vm_method_is_static(vmm)) {
		printf("\tthis\t: ");
		print_arg(J_REFERENCE, frame->args, &arg_index);
	}

	type_str = vmm->type;

	if (!strncmp(type_str, "()", 2)) {
		printf("\targs\t: none\n");
		return;
	}

	printf("\targs\t:\n");

	while ((type_str = parse_method_args(type_str, &arg_type))) {
		printf("\t   %-12s: ", get_vm_type_name(arg_type));
		print_arg(arg_type, frame->args, &arg_index);
	}
}

static void trace_return_address(struct jit_stack_frame *frame)
{
	printf("\tret\t: %p", (void*)frame->return_address);

	if (is_native(frame->return_address)) {
		printf(" (native)\n");
	} else {
		struct compilation_unit *cu;
		struct vm_method *vmm;
		struct vm_class *vmc;

		cu = jit_lookup_cu(frame->return_address);
		if (!cu) {
			printf(" (no compilation unit mapping)\n");
			return;
		}

		vmm = cu->method;;
		vmc = vmm->class;

		printf(" (%s.%s%s)\n", vmc->name, vmm->name, vmm->type );
	}
}


void trace_invoke(struct compilation_unit *cu)
{
	struct vm_method *vmm = cu->method;
	struct vm_class *vmc = vmm->class;

	printf("trace invoke: %s.%s%s\n", vmc->name, vmm->name, vmm->type);

	if (opt_trace_invoke_verbose) {
		struct jit_stack_frame *frame;

		frame =  __builtin_frame_address(1);

		printf("\tentry\t: %p\n", buffer_ptr(cu->objcode));
		trace_return_address(frame);
		trace_invoke_args(vmm, frame);
	}
}

void trace_exception(struct compilation_unit *cu, struct jit_stack_frame *frame,
		     unsigned char *native_ptr)
{
	struct vm_object *exception;
	struct vm_method *vmm;
	struct vm_class *vmc;


	vmm = cu->method;
	vmc = vmm->class;

	exception = exception_occurred();
	assert(exception);

	printf("trace exception: exception object %p (%s) thrown\n",
	       exception, exception->class->name);

	printf("\tfrom\t: %p (%s.%s%s)\n", native_ptr, vmc->name, vmm->name,
	       vmm->type);
}

void trace_exception_handler(unsigned char *ptr)
{
	printf("\taction\t: jump to handler at %p\n", ptr);
}

void trace_exception_unwind(struct jit_stack_frame *frame)
{
	struct compilation_unit *cu;
	struct vm_method *vmm;
	struct vm_class *vmc;

	cu = jit_lookup_cu(frame->return_address);

	vmm = cu->method;
	vmc = vmm->class;

	printf("\taction\t: unwind to %p (%s.%s%s)\n",
	       (void*)frame->return_address, vmc->name, vmm->name, vmm->type);
}

void trace_exception_unwind_to_native(struct jit_stack_frame *frame)
{
	printf("\taction\t: unwind to native caller at %p\n",
	       (void*)frame->return_address);
}
