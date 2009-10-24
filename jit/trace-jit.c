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
#include "jit/instruction.h"
#include "jit/lir-printer.h"
#include "jit/exception.h"
#include "jit/cu-mapping.h"
#include "jit/statement.h"
#include "jit/vars.h"
#include "jit/args.h"
#include "lib/buffer.h"
#include "lib/bitset.h"
#include "lib/string.h"
#include "vm/bytecodes.h"
#include "vm/class.h"
#include "vm/jni.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/preload.h"
#include "vm/trace.h"
#include "vm/vm.h"

#include "arch/stack-frame.h"

#include <ctype.h>
#include <malloc.h>
#include <pthread.h>
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

bool opt_trace_method;
regex_t method_trace_regex;

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
bool opt_trace_bytecode;
bool opt_trace_compile;

bool method_matches_regex(struct vm_method *vmm)
{
	if (!opt_trace_method)
		return true;

	int err;

	char *signature;
	err = asprintf(&signature, "%s.%s%s",
		vmm->class->name, vmm->name, vmm->type);
	if (err == -1)
		die("asprintf");

	err = regexec(&method_trace_regex, signature, 0, NULL, 0);
	free(signature);

	return err == 0;
}

void trace_method(struct compilation_unit *cu)
{
	struct vm_method *method = cu->method;
	unsigned char *p;
	unsigned int i, j;

	if (!method_matches_regex(method))
		return;

	trace_printf("\nTRACE: %s.%s%s\n",
		method->class->name, method->name, method->type);

	trace_printf("Length: %d\n", method->code_attribute.code_length);
	trace_printf("Code:\n");
	p = method->code_attribute.code;

	unsigned int n = method->code_attribute.code_length;
	unsigned int rows = n / 16;
	unsigned int cols = 16;

	for (i = 0; i <= rows; ++i) {
		if (i == rows) {
			cols = n % 16;
			if (!cols)
				break;
		}

		trace_printf("[ %04u ] ", i * 16);

		for (j = 0; j < cols; ++j) {
			trace_printf("%02x%s",
				p[i * 16 + j],
				j == 7 ? "  " : " ");
		}

		trace_printf("\n");
	}

	trace_printf("\n\n");
}

void trace_cfg(struct compilation_unit *cu)
{
	struct basic_block *bb;

	if (!cu_matches_regex(cu))
		return;

	trace_printf("Control Flow Graph:\n\n");
	trace_printf("  #:\t\tRange\t\tSuccessors\t\tPredecessors\n");

	for_each_basic_block(bb, &cu->bb_list) {
		unsigned long i;

		trace_printf("  %p\t%lu..%lu\t", bb, bb->start, bb->end);
		if (bb->is_eh)
			trace_printf(" (eh)");

		trace_printf("\t");

		for (i = 0; i < bb->nr_successors; i++) {
			if (i != 0)
				trace_printf(", ");

			trace_printf("%p", bb->successors[i]);
		}

		if (i == 0)
			trace_printf("none    ");

		trace_printf("\t");

		for (i = 0; i < bb->nr_predecessors; i++) {
			if (i != 0)
				trace_printf(", ");

			trace_printf("%p", bb->predecessors[i]);
		}

		if (i == 0)
			trace_printf("none    ");

		trace_printf("\n");
	}

	trace_printf("\n");
}

void trace_tree_ir(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct statement *stmt;
	struct string *str;

	if (!cu_matches_regex(cu))
		return;

	trace_printf("High-Level Intermediate Representation (HIR):\n\n");

	for_each_basic_block(bb, &cu->bb_list) {
		trace_printf("[bb %p]:\n\n", bb);
		for_each_stmt(stmt, &bb->stmt_list) {
			str = alloc_str();
			tree_print(&stmt->node, str);
			trace_printf("%s", str->value);
			free_str(str);
		}
		trace_printf("\n");
	}
}

void trace_lir(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct string *str;
	struct insn *insn;

	if (!cu_matches_regex(cu))
		return;

	trace_printf("Low-Level Intermediate Representation (LIR):\n\n");

	trace_printf("Bytecode   LIR\n");
	trace_printf("offset     offset    Instruction          Operands\n");
	trace_printf("---------  -------   -----------          --------\n");

	for_each_basic_block(bb, &cu->bb_list) {
		trace_printf("[bb %p]:\n", bb);
		for_each_insn(insn, &bb->insn_list) {
			str = alloc_str();
			lir_print(insn, str);

			if (opt_trace_bytecode_offset) {
				unsigned long bc_offset;
				struct string *bc_str;

				bc_offset = insn_get_bc_offset(insn);
				bc_str = alloc_str();

				print_bytecode_offset(bc_offset, bc_str);

				trace_printf("[ %5s ]  ", bc_str->value);
				free_str(bc_str);
			}

			trace_printf(" %5lu:   %s\n", insn->lir_pos, str->value);
			free_str(str);
		}
	}

	trace_printf("\n");
}

static void
print_var_liveness(struct compilation_unit *cu, struct var_info *var)
{
	struct live_interval *interval;
	unsigned long offset;

	trace_printf("  %2lu: ", var->vreg);
	interval = var->interval;
	offset = 0;

	if (interval_is_empty(interval))
		goto skip;

	for (; offset < interval_start(interval); offset++)
		trace_printf("   ");

	for (; offset < interval_end(interval); offset++) {
		if (interval_covers(interval, offset)) {
			if (next_use_pos(var->interval, offset) == offset) {
				/* In use */
				trace_printf("UUU");
			} else {
				if (var->interval->reg == MACH_REG_UNASSIGNED)
					trace_printf("***");
				else
					trace_printf("---");
			}
		} else
			trace_printf("   ");
	}

 skip:
	for (; offset < cu->last_insn; offset++)
		trace_printf("   ");

	if (!interval_is_empty(interval))
		trace_printf(" (start: %2lu, end: %2lu)\n",
			     interval_start(interval),
			     interval_end(interval));
	else
		trace_printf(" (empty)\n");
}

static void print_bitset(struct bitset *bitset)
{
	for (unsigned int i = 0; i < bitset->nr_bits; ++i) {
		trace_printf("%s", test_bit(bitset->bits, i)
			? "***" : "---");
	}
}

static void print_bb_live_sets(struct basic_block *bb)
{
	trace_printf("[bb %p]\n", bb);

	trace_printf("          ");
	for (unsigned int i = 0; i < bb->live_in_set->nr_bits; ++i)
		trace_printf("%-3d", i);
	trace_printf("\n");

	trace_printf("live in:  ");
	print_bitset(bb->live_in_set);
	trace_printf("\n");

	trace_printf("uses:     ");
	print_bitset(bb->use_set);
	trace_printf("\n");

	trace_printf("defines:  ");
	print_bitset(bb->def_set);
	trace_printf("\n");

	trace_printf("live out: ");
	print_bitset(bb->live_out_set);
	trace_printf("\n");

	trace_printf("\n");
}

void trace_liveness(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct var_info *var;
	unsigned long offset;

	if (!cu_matches_regex(cu))
		return;

	trace_printf("Liveness:\n\n");

	trace_printf("Legend: (U) In use, (-) Fixed register, (*) Non-fixed register\n\n");
	trace_printf("      ");

	for (offset = 0; offset < cu->last_insn; offset++)
		trace_printf("%-2lu ", offset);

	trace_printf("\n");

	for_each_variable(var, cu->var_infos)
		print_var_liveness(cu, var);

	trace_printf("\n");

	for_each_basic_block(bb, &cu->bb_list)
		print_bb_live_sets(bb);
	trace_printf("\n");
}

void trace_regalloc(struct compilation_unit *cu)
{
	struct var_info *var;

	if (!cu_matches_regex(cu))
		return;

	trace_printf("Register Allocation:\n\n");

	for_each_variable(var, cu->var_infos) {
		struct live_interval *interval;

		for (interval = var->interval; interval != NULL; interval = interval->next_child) {
			trace_printf("  %2lu ", var->vreg);

			if (interval_is_empty(interval))
				trace_printf("(empty)       :");
			else
				trace_printf("(pos: %3lu-%3lu):",
					     interval_start(interval),
					     interval_end(interval));

			trace_printf("\t%s", reg_name(interval->reg));
			trace_printf("\t%-11s", get_vm_type_name(var->vm_type));
			trace_printf("\t%s", interval->fixed_reg ? "fixed\t" : "non-fixed");
			if (interval->need_spill) {
				unsigned long ndx = -1;

				if (interval->spill_slot)
					ndx = interval->spill_slot->index;

				trace_printf("\tspill (%ld)", ndx);
			} else
				trace_printf("\tno spill  ");

			if (interval->need_reload) {
				unsigned long ndx = -1;

				if (interval->spill_parent && interval->spill_parent->spill_slot)
					ndx = interval->spill_parent->spill_slot->index;
				trace_printf("\treload (%ld)", ndx);
			} else
				trace_printf("\tno reload  ");
			trace_printf("\n");
		}
	}
	trace_printf("\n");
}

void trace_machine_code(struct compilation_unit *cu)
{
	void *start, *end;

	if (!cu_matches_regex(cu))
		return;

	trace_printf("Disassembler Listing:\n\n");

	start  = buffer_ptr(cu->objcode);
	end = buffer_current(cu->objcode);

	disassemble(cu, start, end);
	trace_printf("\n");
}

void trace_magic_trampoline(struct compilation_unit *cu)
{
	if (!cu_matches_regex(cu))
		return;

	trace_printf("jit_magic_trampoline: ret0=%p, ret1=%p: %s.%s #%d\n",
	       __builtin_return_address(1),
	       __builtin_return_address(2),
	       cu->method->class->name,
	       cu->method->name,
	       cu->method->method_index);

	trace_flush();
}

static void print_array(struct vm_object *obj)
{
	const int max_elems = 20;
	struct vm_class *elem_class;
	enum vm_type type;

	elem_class = vm_class_get_array_element_class(obj->class);
	type = vm_class_get_storage_vmtype(elem_class);

	trace_printf("= {");

	for (int i = 0; i < obj->array_length && i < max_elems; i++) {
		if (i > 0)
			trace_printf(" ,");

		if (type == J_CHAR) {
			jchar c = array_get_field_char(obj, i);

			if (isprint(c))
				trace_printf("'%c'", c);
			else
				trace_printf("<%d>", c);

			continue;
		}

		switch (type) {
		case J_REFERENCE:
			trace_printf("%p", array_get_field_object(obj, i));
			break;
		case J_BYTE:
			trace_printf("%x", (int)array_get_field_byte(obj, i));
			break;
		case J_SHORT:
			trace_printf("%x", (int)array_get_field_short(obj, i));
			break;
		case J_INT:
			trace_printf("%x", array_get_field_int(obj, i));
			break;
		case J_LONG:
			trace_printf("%llx", array_get_field_long(obj, i));
			break;
		case J_FLOAT:
			trace_printf("%f", array_get_field_float(obj, i));
			break;
		case J_DOUBLE:
			trace_printf("%f", array_get_field_double(obj, i));
			break;
		case J_BOOLEAN:
			trace_printf("%s", array_get_field_boolean(obj, i)
				     ? "true" : "false");
			break;
		default:
			error("invalid array element type");
		}
	}

	if (obj->array_length > max_elems)
		trace_printf(", ...%d more", obj->array_length - max_elems);

	trace_printf("}");
}

static void print_arg(enum vm_type arg_type, const unsigned long *args,
		      int *arg_index)
{
	const int MAX_STRING_LENGTH = 50;

	if (arg_type == J_LONG || arg_type == J_DOUBLE) {
		union {
			unsigned long long ullvalue;
			double dvalue;
		} value;

		value.ullvalue = *(unsigned long long*)(args + *arg_index);
		(*arg_index) += 2;

		trace_printf("0x%llx", value.ullvalue);

		if (arg_type == J_DOUBLE)
			trace_printf("(%lf)", value.dvalue);

		trace_printf("\n");
		return;
	}

	trace_printf("0x%lx ", args[*arg_index]);

	if (arg_type == J_FLOAT) {
		union {
			unsigned long ulvalue;
			float fvalue;
		} value;

		value.ulvalue = args[*arg_index];

		trace_printf("(%f)", value.fvalue);
	}

	if (arg_type == J_CHAR) {
		char c = args[*arg_index] & 0xFF;

		if (isprint(c))
			trace_printf("('%c')", c);
	}

	if (arg_type == J_REFERENCE) {
		struct vm_object *obj;

		obj = (struct vm_object *)args[*arg_index];

		if (!obj) {
			trace_printf("null");
			goto out;
		}

		if (obj->class == vm_java_lang_String) {
			char *str;
			int len;

			str = vm_string_to_cstr(obj);
			len = strlen(str);

			if (len > MAX_STRING_LENGTH) {
				str[MAX_STRING_LENGTH] = 0;
				trace_printf("= \"%s\" (...%d more)", str,
					     len - MAX_STRING_LENGTH);
			} else {
				trace_printf("= \"%s\"", str);
			}

			free(str);
		}

		if (vm_class_is_array_class(obj->class))
			print_array(obj);

		trace_printf(" (%s)", obj->class->name);
	}

 out:
	(*arg_index)++;
	trace_printf("\n");
}

static void trace_invoke_args(struct vm_method *vmm,
			      struct jit_stack_frame *frame)
{
	struct vm_method_arg *arg;
	int arg_index;

	if (vm_method_is_jni(vmm))
		arg_index += 2;

	arg_index = 0;

	if (!vm_method_is_static(vmm)) {
		trace_printf("\tthis\t: ");
		print_arg(J_REFERENCE, frame->args, &arg_index);
	}

	if (list_is_empty(&vmm->args)) {
		trace_printf("\targs\t: none\n");
		return;
	}

	trace_printf("\targs\t:\n");

	list_for_each_entry(arg, &vmm->args, list_node) {
		trace_printf("\t   %-12s: ",
			     get_vm_type_name(arg->type_info.vm_type));
		print_arg(arg->type_info.vm_type, frame->args, &arg_index);
	}
}

static void print_source_and_line(struct compilation_unit *cu,
				  unsigned char *ptr)
{
	unsigned long pc;
	const char *source_file;

	source_file = cu->method->class->source_file_name;
	if (source_file)
		trace_printf("%s", source_file);
	else
		trace_printf("UNKNOWN");

	pc = jit_lookup_bc_offset(cu, ptr);
	if (pc == BC_OFFSET_UNKNOWN)
		return;

	trace_printf(":%d", bytecode_offset_to_line_no(cu->method, pc));
}

static void trace_return_address(struct jit_stack_frame *frame)
{
	trace_printf("\tret\t: %p:", (void*)frame->return_address);

	if (is_native(frame->return_address)) {
		trace_printf(" (native)\n");
	} else {
		struct compilation_unit *cu;
		struct vm_method *vmm;
		struct vm_class *vmc;

		cu = jit_lookup_cu(frame->return_address);
		if (!cu) {
			trace_printf(" (no compilation unit mapping)\n");
			return;
		}

		vmm = cu->method;;
		vmc = vmm->class;

		trace_printf(" %s.%s%s\n", vmc->name, vmm->name, vmm->type );
		trace_printf("\t\t  (");
		print_source_and_line(cu, (void *) frame->return_address);
		trace_printf(")\n");
	}
}

void trace_invoke(struct compilation_unit *cu)
{
	struct vm_method *vmm = cu->method;
	if (!method_matches_regex(vmm))
		return;

	struct vm_class *vmc = vmm->class;

	trace_printf("trace invoke: %s.%s%s\n", vmc->name, vmm->name,
		     vmm->type);

	if (opt_trace_invoke_verbose) {
		struct jit_stack_frame *frame;

		frame =  __builtin_frame_address(1);

		trace_printf("\tentry\t: %p\n", buffer_ptr(cu->objcode));
		trace_return_address(frame);
		trace_invoke_args(vmm, frame);
	}

	trace_flush();
}

void trace_exception(struct compilation_unit *cu, struct jit_stack_frame *frame,
		     unsigned char *native_ptr)
{
	struct vm_object *exception;
	struct vm_method *vmm;
	struct vm_class *vmc;
	struct vm_object *msg;
	int dummy;

	if (!cu_matches_regex(cu))
		return;

	vmm = cu->method;
	vmc = vmm->class;

	exception = exception_occurred();
	assert(exception);

	trace_printf("trace exception: exception object %p (%s) thrown\n",
	       exception, exception->class->name);

	dummy = 0;
	msg = field_get_object(exception, vm_java_lang_Throwable_detailMessage);

	trace_printf("\tmessage\t: ");
	print_arg(J_REFERENCE, (unsigned long *)  &msg, &dummy);

	trace_printf("\tfrom\t: %p: %s.%s%s\n", native_ptr, vmc->name, vmm->name,
	       vmm->type);
	trace_printf("\t\t  (");
	print_source_and_line(cu, native_ptr);
	trace_printf(")\n");

	/* XXX: trace is flushed in one of the action tracers. */
}

void trace_exception_handler(struct compilation_unit *cu,
			     unsigned char *ptr)
{
	if (!cu_matches_regex(cu))
		return;

	trace_printf("\taction\t: jump to handler at %p\n", ptr);
	trace_printf("\t\t  (");
	print_source_and_line(cu, ptr);
	trace_printf(")\n");

	trace_flush();
}

void trace_exception_unwind(struct jit_stack_frame *frame)
{
	struct compilation_unit *cu;
	struct vm_method *vmm;
	struct vm_class *vmc;

	cu = jit_lookup_cu(frame->return_address);

	vmm = cu->method;
	vmc = vmm->class;

	trace_printf("\taction\t: unwind to %p: %s.%s%s\n",
	       (void*)frame->return_address, vmc->name, vmm->name, vmm->type);
	trace_printf("\t\t  (");
	print_source_and_line(cu, (void *) frame->return_address);
	trace_printf(")\n");

	trace_flush();
}

void trace_exception_unwind_to_native(struct jit_stack_frame *frame)
{
	trace_printf("\taction\t: unwind to native caller at %p\n",
	       (void*)frame->return_address);

	trace_flush();
}

void trace_bytecode(struct vm_method *method)
{
	if (!method_matches_regex(method))
		return;

	trace_printf("Code:\n");
	bytecode_disassemble(method->class,
			     method->code_attribute.code,
			     method->code_attribute.code_length);
	trace_printf("\nException table:\n");
	print_exception_table(method,
		method->code_attribute.exception_table,
		method->code_attribute.exception_table_length);
	trace_printf("\n");
}

void trace_return_value(struct vm_method *vmm, unsigned long long value)
{
	enum vm_type type;
	int dummy;

	if (!method_matches_regex(vmm))
		return;

	dummy = 0;
	type = method_return_type(vmm);

	trace_printf("trace return: %s.%s%s\n", vmm->class->name, vmm->name,
		     vmm->type);
	if (type == J_VOID) {
		trace_flush();
		return;
	}

	trace_printf("%12s: ", get_vm_type_name(type));
	print_arg(type,(unsigned long *)  &value, &dummy);
	trace_flush();
}
