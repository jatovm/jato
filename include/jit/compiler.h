#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "lib/buffer.h"
#include "vm/stack.h"
#include "vm/vm.h"

#include <pthread.h>
#include <stdbool.h>

struct vm_method;
struct compilation_unit;
struct expression;
struct statement;
struct buffer;

struct fixup_site {
	/* Compilation unit to which relcall_insn belongs */
	struct compilation_unit *cu;
	/* We need this, because we don't have native pointer at
	   instruction selection */
	struct insn *relcall_insn;
	struct list_head fixup_list_node;
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
int allocate_registers(struct compilation_unit *cu);
int insert_spill_reload_insns(struct compilation_unit *cu);
int emit_machine_code(struct compilation_unit *);
void *jit_magic_trampoline(struct compilation_unit *);

struct jit_trampoline *alloc_jit_trampoline(void);
struct jit_trampoline *build_jit_trampoline(struct compilation_unit *);
void free_jit_trampoline(struct jit_trampoline *);
struct fixup_site *alloc_fixup_site(void);
void free_fixup_site(struct fixup_site *);
void trampoline_add_fixup_site(struct jit_trampoline *, struct fixup_site *);
unsigned char *fixup_site_addr(struct fixup_site *);

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

extern bool opt_trace_cfg;
extern bool opt_trace_tree_ir;
extern bool opt_trace_lir;
extern bool opt_trace_liveness;
extern bool opt_trace_regalloc;
extern bool opt_trace_machine_code;
extern bool opt_trace_gc_maps;
extern bool opt_trace_magic_trampoline;
extern bool opt_trace_bytecode_offset;
extern bool opt_trace_invoke;
extern bool opt_trace_invoke_verbose;
extern bool opt_trace_exceptions;
extern bool opt_trace_bytecode;
extern bool opt_trace_compile;

void trace_magic_trampoline(struct compilation_unit *);
void trace_method(struct compilation_unit *);
void trace_cfg(struct compilation_unit *);
void trace_tree_ir(struct compilation_unit *);
void trace_lir(struct compilation_unit *);
void trace_liveness(struct compilation_unit *);
void trace_regalloc(struct compilation_unit *);
void trace_machine_code(struct compilation_unit *);
void trace_gc_maps(struct compilation_unit *);
void trace_invoke(struct compilation_unit *);
void trace_exception(struct compilation_unit *, struct jit_stack_frame *, unsigned char *);
void trace_exception_handler(struct compilation_unit *, unsigned char *);
void trace_exception_unwind(struct jit_stack_frame *);
void trace_exception_unwind_to_native(struct jit_stack_frame *);
void trace_bytecode(struct vm_method *);
void trace_return_value(struct vm_method *, unsigned long long);

#endif
