#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "lib/buffer.h"
#include "lib/stack.h"
#include "vm/vm.h"

#include <pthread.h>
#include <regex.h>
#include <stdbool.h>
#include <sys/types.h>

struct vm_method;
struct compilation_unit;
struct expression;
struct statement;
struct buffer;

struct fixup_site {
	struct jit_trampoline *target;

	/* Compilation unit to which relcall_insn belongs */
	struct compilation_unit *cu;

	/*
	 * We need insn pointer because we don't have native pointer at
	 * instruction selection. mach_offset is filled in after compilation
	 * is done.
	 */
	struct insn *relcall_insn;
	uint32_t mach_offset;

	struct list_head list_node;
};

struct jit_trampoline {
	struct buffer *objcode;
	struct list_head fixup_site_list;
	/* This mutex is protecting operations on fixup_site_list */
	pthread_mutex_t mutex;
};

struct parse_context {
	struct compilation_unit *cu;
	struct basic_block *bb;

	struct bytecode_buffer *buffer;
	unsigned char *code;
	unsigned long offset;
	unsigned long code_size;
	unsigned char opc;

	bool is_wide;
};

void convert_expression(struct parse_context *ctx, struct expression *expr);
void convert_statement(struct parse_context *ctx, struct statement *stmt);
void do_convert_statement(struct basic_block *bb, struct statement *stmt,
			  unsigned long bc_offset);

int compile(struct compilation_unit *);
int analyze_control_flow(struct compilation_unit *);
int convert_to_ir(struct compilation_unit *);
int analyze_liveness(struct compilation_unit *);
int select_instructions(struct compilation_unit *cu);
int compute_dfns(struct compilation_unit *cu);
int compute_dom(struct compilation_unit *cu);
int compute_dom_frontier(struct compilation_unit *cu);
int lir_to_ssa(struct compilation_unit *cu);
int ssa_to_lir(struct compilation_unit *cu);
int dce(struct compilation_unit *cu);
void imm_copy_propagation(struct compilation_unit *cu);
int allocate_registers(struct compilation_unit *cu);
int insert_spill_reload_insns(struct compilation_unit *cu);
int emit_machine_code(struct compilation_unit *);
void *jit_magic_trampoline(struct compilation_unit *);
void jit_no_such_method_stub(void);

struct jit_trampoline *alloc_jit_trampoline(void);
struct jit_trampoline *build_jit_trampoline(struct compilation_unit *);
void free_jit_trampoline(struct jit_trampoline *);
struct fixup_site *alloc_fixup_site(struct compilation_unit *, struct insn *);
void free_fixup_site(struct fixup_site *);
void trampoline_add_fixup_site(struct jit_trampoline *, struct fixup_site *);
unsigned char *fixup_site_addr(struct fixup_site *);
bool fixup_site_is_ready(struct fixup_site *);
void prepare_call_fixup_sites(struct compilation_unit *);

const char *method_symbol(struct vm_method *method, char *symbol, size_t len);

static inline const char *cu_symbol(struct compilation_unit *cu, char *symbol, size_t len)
{
	return method_symbol(cu->method, symbol, len);
}

static inline void *cu_native_ptr(struct compilation_unit *cu)
{
	return buffer_ptr(cu->objcode);
}

static inline unsigned long cu_native_size(struct compilation_unit *cu)
{
	return buffer_offset(cu->objcode);
}

bool is_native(unsigned long eip);
bool is_on_heap(unsigned long addr);

void fixup_direct_calls(struct jit_trampoline *trampoline, unsigned long target);

extern bool opt_trace_method;
extern regex_t method_trace_regex;

extern bool opt_trace_gate;
extern regex_t method_trace_gate_regex;

extern bool opt_trace_ssa;
extern bool opt_trace_cfg;
extern bool opt_trace_tree_ir;
extern bool opt_trace_lir;
extern bool opt_trace_liveness;
extern bool opt_trace_regalloc;
extern bool opt_trace_machine_code;
extern bool opt_trace_magic_trampoline;
extern bool opt_trace_bytecode_offset;
extern bool opt_trace_invoke;
extern bool opt_trace_invoke_verbose;
extern bool opt_trace_exceptions;
extern bool opt_trace_bytecode;
extern bool opt_trace_compile;
extern bool opt_print_compilation;

extern bool opt_ssa_enable;
extern bool running_on_valgrind;

bool method_matches_regex(struct vm_method *vmm);
bool method_matches_gate_regex(struct vm_method *vmm);

static inline bool cu_matches_regex(struct compilation_unit *cu)
{
	return method_matches_regex(cu->method);
}

void trace_ssa(struct compilation_unit *);
void trace_magic_trampoline(struct compilation_unit *);
void trace_method(struct compilation_unit *);
void trace_cfg(struct compilation_unit *);
void trace_tree_ir(struct compilation_unit *);
void trace_lir(struct compilation_unit *);
void trace_liveness(struct compilation_unit *);
void trace_regalloc(struct compilation_unit *);
void trace_machine_code(struct compilation_unit *);
void trace_invoke(struct compilation_unit *);
void trace_exception(struct compilation_unit *, struct jit_stack_frame *, unsigned char *);
void trace_exception_handler(struct compilation_unit *, unsigned char *);
void trace_exception_unwind(struct jit_stack_frame *);
void trace_exception_unwind_to_native(struct jit_stack_frame *);
void trace_bytecode(struct vm_method *);
void trace_return_value(struct vm_method *, unsigned long long);
void print_compilation(struct vm_method *);

#endif
