/*
 * Subroutine bytecode inlining
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 *
 * Inlines all subroutines. This eliminates  JSR, JSR_W and RET
 * instructions. The algorithm is based on the following work:
 *
 *   "Subroutine Inlining and Bytecode Abstraction to Simplify Static
 *    and Dynamic Analysis", Cyrille Artho, Armin Biere.
 */

#include "cafebabe/line_number_table_attribute.h"
#include "cafebabe/code_attribute.h"

#include "jit/bc-offset-mapping.h"
#include "jit/subroutine.h"
#include "jit/exception.h"
#include "jit/pc-map.h"

#include "vm/bytecode.h"
#include "vm/method.h"
#include "vm/stdlib.h"
#include "vm/stream.h"
#include "vm/die.h"
#include "vm/trace.h"

#include "lib/list.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define PC_UNKNOWN ULONG_MAX

struct code_state {
	unsigned char *code;
	uint32_t code_length;
};

struct subroutine {
	unsigned long start_pc;
	unsigned long end_pc;
	unsigned long ret_index;
	unsigned long prolog_size;
	unsigned long epilog_size;

	int nr_dependencies;

	int nr_dependants;
	/* holds subroutines which depend on this subroutine. */
	struct subroutine **dependants;

	int nr_call_sites;
	unsigned long *call_sites;
	unsigned long call_sites_total_size;

	struct list_head subroutine_list_node;
};

struct inlining_context {
	struct list_head subroutine_list;
	struct code_state code;
	struct pc_map pc_map;

	uint16_t exception_table_length;
	struct cafebabe_code_attribute_exception *exception_table;

	uint16_t line_number_table_length;
	struct cafebabe_line_number_table_entry *line_number_table;

	struct vm_method *method;
};

struct subroutine_scan_context {
	struct inlining_context *i_ctx;
	struct subroutine *sub;
	bool *visited_code;
};

static unsigned long next_pc(const unsigned char *code, unsigned long current)
{
	unsigned long opc_size;

	opc_size = bc_insn_size(code, current);
	assert(opc_size != 0);

	return current + opc_size;
}

static struct subroutine *alloc_subroutine(void)
{
	struct subroutine *sub;

	sub = zalloc(sizeof(*sub));
	if (!sub)
		return NULL;

	INIT_LIST_HEAD(&sub->subroutine_list_node);

	return sub;
}

static void free_subroutine(struct subroutine *sub)
{
	if (sub->call_sites)
		free(sub->call_sites);

	if (sub->dependants)
		free(sub->dependants);

	free(sub);
}

static int code_state_init(struct code_state *code_state,
			   const unsigned char *code, int code_length)
{
	code_state->code_length = code_length;
	code_state->code = malloc(code_length);
	if (!code_state->code)
		return warn("out of memory"), -ENOMEM;

	memcpy(code_state->code, code, code_length);
	return 0;
}

static int code_state_init_empty(struct code_state *code_state, int code_length)
{
	code_state->code_length = code_length;
	code_state->code = malloc(code_length);
	if (!code_state->code)
		return warn("out of memory"), -ENOMEM;

	return 0;
}

static void code_state_deinit(struct code_state *state)
{
	if (state->code)
		free(state->code);
}

static struct inlining_context *alloc_inlining_context(struct vm_method *method)
{
	struct inlining_context *ctx;
	int size;

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return NULL;

	ctx->method = method;

	INIT_LIST_HEAD(&ctx->subroutine_list);

	if (code_state_init(&ctx->code, method->code_attribute.code,
			    method->code_attribute.code_length))
		goto error_code;

	ctx->exception_table_length =
		method->code_attribute.exception_table_length;
	size = sizeof(*method->code_attribute.exception_table) *
		ctx->exception_table_length;
	ctx->exception_table = malloc(size);
	if (!ctx->exception_table)
		goto error_exception_table;

	memcpy(ctx->exception_table, method->code_attribute.exception_table,
	       size);

	/*
	 * The size of the map is code_length + 1 because we need a
	 * mapping for the address that is equal to code_length. This
	 * address can occure in exception handlers' range addresses.
	 */
	if (pc_map_init_identity(&ctx->pc_map, ctx->code.code_length + 1))
		goto error_pc_map;

	return ctx;

 error_pc_map:
	free(ctx->exception_table);
 error_exception_table:
	free(ctx->code.code);
 error_code:
	free(ctx);

	return NULL;
}

static void free_inlining_context(struct inlining_context *ctx)
{
	struct subroutine *this;

	list_for_each_entry(this, &ctx->subroutine_list, subroutine_list_node) {
		free_subroutine(this);
	}

	pc_map_deinit(&ctx->pc_map);
	free(ctx);
}

static struct subroutine_scan_context *
alloc_subroutine_scan_context(struct inlining_context *i_ctx,
			      struct subroutine *sub)
{
	struct subroutine_scan_context *ctx;

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return NULL;

	ctx->i_ctx = i_ctx;

	ctx->visited_code = zalloc(i_ctx->code.code_length * sizeof(bool));
	if (!ctx->visited_code) {
		free(ctx);
		return NULL;
	}

	ctx->sub = sub;

	return ctx;
}

static void free_subroutine_scan_context(struct subroutine_scan_context *ctx)
{
	free(ctx->visited_code);
	free(ctx);
}

static inline unsigned long subroutine_get_body_size(struct subroutine *sub)
{
	return sub->end_pc - sub->start_pc - sub->prolog_size;
}

static inline unsigned long subroutine_get_size(struct subroutine *sub)
{
	return sub->end_pc + sub->epilog_size - sub->start_pc;
}

static struct subroutine *lookup_subroutine(struct inlining_context *ctx,
					    unsigned long target)
{
	struct subroutine *this;

	list_for_each_entry(this, &ctx->subroutine_list, subroutine_list_node) {
		if (this->start_pc == target)
			return this;
	}

	return NULL;
}

static void add_subroutine(struct inlining_context *ctx, struct subroutine *sub)
{
	/* TODO: use hash table to speedup lookup. */
	list_add_tail(&sub->subroutine_list_node, &ctx->subroutine_list);
}

static int subroutine_add_call_site(struct subroutine *sub,
				    const unsigned char *code,
				    unsigned long call_site)
{
	unsigned long *new_tab;
	int new_size;

	new_size = (sub->nr_call_sites + 1) * sizeof(unsigned long);
	new_tab = realloc(sub->call_sites, new_size);
	if (!new_tab)
		return warn("out of memory"), -ENOMEM;

	sub->call_sites = new_tab;
	sub->call_sites[sub->nr_call_sites++] = call_site;

	sub->call_sites_total_size += bc_insn_size(code, call_site);

	return 0;
}

static int subroutine_add_dependency(struct inlining_context *ctx,
				     struct subroutine *sub,
				     unsigned long target)
{
	struct subroutine *target_sub;
	struct subroutine **new_tab;
	int new_size;

	target_sub = lookup_subroutine(ctx, target);
	if (!target_sub)
		return -1;

	new_size = (target_sub->nr_dependants + 1) * sizeof(void *);
	new_tab = realloc(target_sub->dependants, new_size);
	if (!new_tab)
		return warn("out of memory"), -ENOMEM;

	target_sub->dependants = new_tab;
	target_sub->dependants[target_sub->nr_dependants++] = sub;

	sub->nr_dependencies++;

	return 0;
}

static int do_subroutine_scan(struct subroutine_scan_context *ctx,
			      unsigned long pc)
{
	unsigned char *c;
	int err;

	c = ctx->i_ctx->code.code;

	while (pc < ctx->i_ctx->code.code_length) {
		if (ctx->visited_code[pc])
			return 0;

		ctx->visited_code[pc]++;

		if (bc_is_ret(&c[pc])) {
			if (bc_get_ret_index(&c[pc]) == ctx->sub->ret_index &&
			    (ctx->sub->end_pc == PC_UNKNOWN ||
			     pc > ctx->sub->end_pc))
					ctx->sub->end_pc = pc;

			return 0;
		}

		if (bc_is_astore(&c[pc]) &&
		    bc_get_astore_index(&c[pc]) == ctx->sub->ret_index)
			return warn("return address override"), -EINVAL;

		if (bc_is_return(c[pc]) || bc_is_athrow(c[pc]))
			break;

		if (bc_is_goto(c[pc])) {
			pc += bc_target_off(&c[pc]);
			continue;
		}

		if (c[pc] == OPC_TABLESWITCH) {
			struct tableswitch_info info;

			get_tableswitch_info(c, pc, &info);
			err = do_subroutine_scan(ctx, pc + info.default_target);
			if (err)
				return err;

			for (unsigned int i = 0; i < info.count; i++) {
				int32_t target;

				target = read_s32(info.targets + i * 4);
				err = do_subroutine_scan(ctx, pc + target);
				if (err)
					return err;
			}

			break;
		}

		if (c[pc] == OPC_LOOKUPSWITCH) {
			struct lookupswitch_info info;

			get_lookupswitch_info(c, pc, &info);
			err = do_subroutine_scan(ctx, pc + info.default_target);
			if (err)
				return err;

			for (unsigned int i = 0; i < info.count; i++) {
				int32_t target;

				target = read_s32(info.pairs + i * 8 + 4);
				err = do_subroutine_scan(ctx, pc + target);
				if (err)
					return err;
			}

			break;
		}

		if (bc_is_jsr(c[pc])) {
			unsigned long target = pc + bc_target_off(&c[pc]);

			if (target == ctx->sub->start_pc)
				return warn("subroutine recursion"), -EINVAL;

			subroutine_add_dependency(ctx->i_ctx, ctx->sub, target);

			err = do_subroutine_scan(ctx, target);
			if (err)
				return err;
		} else 	if (bc_is_branch(c[pc])) {
			unsigned long target = pc + bc_target_off(&c[pc]);

			err = do_subroutine_scan(ctx, target);
			if (err)
				return err;
		}

		pc = next_pc(c, pc);
	}

	return 0;
}

/**
 * Traverses through all instructions reachable from subroutine @sub
 * to locate subroutine's end and collect dependencies on other
 * subroutines. Performs also some bytecode verification.
 */
static int subroutine_scan(struct inlining_context *ctx, struct subroutine *sub)
{
	unsigned long target;
	int err;

	if (sub->start_pc >= ctx->code.code_length)
		return warn("invalid branch offset"), -EINVAL;

	if (!bc_is_astore(&ctx->code.code[sub->start_pc]))
		return warn("invalid subroutine start"), -EINVAL;

	sub->ret_index = bc_get_astore_index(&ctx->code.code[sub->start_pc]);
	target = next_pc(ctx->code.code, sub->start_pc);
	sub->prolog_size = target - sub->start_pc;

	struct subroutine_scan_context *sub_ctx =
		alloc_subroutine_scan_context(ctx, sub);
	if (!sub_ctx)
		return warn("out of memory"), -ENOMEM;

	sub->end_pc = PC_UNKNOWN;

	err = do_subroutine_scan(sub_ctx, target);
	free_subroutine_scan_context(sub_ctx);
	if (err)
		return err;

	if (sub->end_pc != PC_UNKNOWN)
		goto out;

	/*
	 * In some situations ECJ compiler generates subroutines which
	 * don't have RET instruction on any possible execution paths
	 * but is still present. Example:
	 *
	 * 0: jsr 2
	 * 2: astore_1
	 * 3: iconst_0
	 * 4: ireturn
	 * 5: ret 1
	 *
	 * We try to locate the RET by straigh scanning.
	 */
	for (unsigned long pc = sub->start_pc; pc < ctx->code.code_length;
	     pc = next_pc(ctx->code.code, pc))
	{
		if (bc_is_ret(&ctx->code.code[pc]) &&
		    bc_get_ret_index(&ctx->code.code[pc]) == sub->ret_index) {
			sub->end_pc = pc;
			goto out;
		}
	}

	return warn("subroutine end not found"), -EINVAL;

 out:
	sub->epilog_size = bc_insn_size(ctx->code.code, sub->end_pc);
	return 0;
}

static int scan_subroutines(struct inlining_context *ctx)
{
	struct subroutine *this;
	int err;

	list_for_each_entry(this, &ctx->subroutine_list, subroutine_list_node) {
		err = subroutine_scan(ctx, this);
		if (err)
			return err;
	}

	return 0;
}

/**
 * Detects all subroutines by scanning code for JSR instructions. All
 * call sites are connected to target subroutines.
 */
static int detect_subroutines(struct inlining_context *ctx)
{
	unsigned long pc;
	int err;

	unsigned long length = ctx->code.code_length;
	unsigned char *c = ctx->code.code;

	for (pc = 0; pc < length; pc = next_pc(c, pc)) {
		struct subroutine *sub;
		unsigned long target;

		if (!bc_is_jsr(c[pc]))
			continue;

		target = pc + bc_target_off(&c[pc]);

		sub = lookup_subroutine(ctx, target);
		if (sub) {
			err = subroutine_add_call_site(sub, c, pc);
			if (err)
				return warn("out of memory"), -ENOMEM;

			continue;
		}

		sub = alloc_subroutine();
		if (!sub)
			return warn("out of memory"), -ENOMEM;

		sub->start_pc = target;

		add_subroutine(ctx, sub);

		err = subroutine_add_call_site(sub, c, pc);
		if (err)
			return err;
	}

	return 0;
}

static int call_site_compare(const void *a, const void *b)
{
	const unsigned long *a2 = a;
	const unsigned long *b2 = b;

	return *a2 - *b2;
}

static void subroutine_sort_call_sites(struct subroutine *sub)
{
	qsort(sub->call_sites, sub->nr_call_sites, sizeof(unsigned long),
	      call_site_compare);
}

static long get_new_branch_offset(struct pc_map *pc_map, unsigned long new_pc,
				  unsigned long target, long target_offset)
{
	unsigned long new_target;
	int err;

	if (target_offset >= 0)
		err = pc_map_get_min_greater_than(pc_map,
						  target, new_pc, &new_target);
	else
		err = pc_map_get_max_lesser_than(pc_map,
						 target, new_pc, &new_target);

	assert(!err);

	return (long) (new_target - new_pc);
}

static void update_switch_target(struct pc_map *pc_map,
				 unsigned char *code,
				 unsigned long switch_pc,
				 unsigned long offset_pc,
				 unsigned char *new_code)
{
	long target_offset = read_s32(&code[offset_pc]);
	int old_pad = get_tableswitch_padding(switch_pc);

	unsigned long *new_pc;
	pc_map_for_each_value(pc_map, switch_pc, new_pc) {
		int new_pad = get_tableswitch_padding(*new_pc);
		long new_offset = get_new_branch_offset(pc_map, *new_pc,
				switch_pc + target_offset, target_offset);

		unsigned long new_offset_pc = offset_pc - switch_pc + *new_pc
			- old_pad + new_pad;
		write_s32(&new_code[new_offset_pc], new_offset);
	}
}

static void update_tableswitch_targets(struct pc_map *pc_map,
				       unsigned char *code,
				       unsigned long pc,
				       unsigned char *new_code)
{
	unsigned long switch_pc;

	switch_pc = pc;

	pc += 1 + get_tableswitch_padding(pc);

	/* update default target */
	update_switch_target(pc_map, code, switch_pc, pc, new_code);
	pc += 4;

	int low = read_u32(&code[pc]);
	pc += 4;

	int high = read_u32(&code[pc]);
	pc += 4;

	int count = high - low + 1;

	for (int i = 0; i < count; i++) {
		update_switch_target(pc_map, code, switch_pc, pc, new_code);
		pc += 4;
	}
}

static void update_lookupswitch_targets(struct pc_map *pc_map,
					unsigned char *code,
					unsigned long pc,
					unsigned char *new_code)
{
	unsigned long switch_pc;

	switch_pc = pc;

	pc += 1 + get_tableswitch_padding(pc);

	/* update default target */
	update_switch_target(pc_map, code, switch_pc, pc, new_code);
	pc += 4;

	int count = read_u32(&code[pc]);
	pc += 4;

	for (int i = 0; i < count; i++) {
		update_switch_target(pc_map, code, switch_pc, pc + 4, new_code);
		pc += 8;
	}
}

static int update_branch_targets(struct inlining_context *ctx)
{
	unsigned long code_length;
	unsigned char *code;

	code_length = ctx->method->code_attribute.code_length;
	code = ctx->method->code_attribute.code;

	unsigned long pc;
	bytecode_for_each_insn(code, code_length, pc) {
		if (code[pc] == OPC_TABLESWITCH) {
			update_tableswitch_targets(&ctx->pc_map, code, pc,
						   ctx->code.code);
			continue;
		}

		if (code[pc] == OPC_LOOKUPSWITCH) {
			update_lookupswitch_targets(&ctx->pc_map, code, pc,
						    ctx->code.code);
			continue;
		}

		if (!bc_is_branch(code[pc]) || bc_is_jsr(code[pc]))
			continue;

		long target_offset = bc_target_off(&code[pc]);
		unsigned long target = pc + target_offset;

		if (!pc_map_has_value_for(&ctx->pc_map, pc))
			return warn("no mapping for pc=%ld", pc), -EINVAL;

		if (!pc_map_has_value_for(&ctx->pc_map, target))
			return warn("no mapping for pc=%ld", target), -EINVAL;

		unsigned long *new_pc;
		pc_map_for_each_value(&ctx->pc_map, pc, new_pc) {
			long new_offset = get_new_branch_offset(&ctx->pc_map,
						*new_pc, target, target_offset);
			bc_set_target_off(&ctx->code.code[*new_pc], new_offset);
		}
	}

	return 0;
}

static int update_bytecode_offsets(struct inlining_context *ctx,
				   struct subroutine *sub)
{
	int err;

	err = pc_map_get_unique(&ctx->pc_map, &sub->start_pc);
	if (err)
		return warn("no or ambiguous mapping"), -EINVAL;

	err = pc_map_get_unique(&ctx->pc_map, &sub->end_pc);
	if (err)
		return warn("no or ambiguous mapping"), -EINVAL;

	for (int i = 0; i < sub->nr_call_sites; i++) {
		err = pc_map_get_unique(&ctx->pc_map, &sub->call_sites[i]);
		if (err)
			return warn("no or ambiguous mapping"), -EINVAL;
	}

	return 0;
}

/*
 * Rewrites count bytes from src:src_pc to dest:dest_pc. Note that
 * the number of bytes written to the destination may be different than
 * count because of padding change in tableswitch and lookupswitch
 * instructions.
 */
static int do_bytecode_copy(struct code_state *dest, unsigned long *dest_pc,
			    struct code_state *src, unsigned long src_pc,
			    unsigned long count, struct pc_map *map)
{
	unsigned int n = src_pc + count;
	while (src_pc < n) {
		int src_insn_size = bc_insn_size(src->code, src_pc);

		if (pc_map_add(map, src_pc, *dest_pc))
				return warn("out of memory"), -ENOMEM;

		unsigned char opcode = src->code[src_pc];
		if (opcode == OPC_TABLESWITCH || opcode == OPC_LOOKUPSWITCH) {
			/* The copy operation might change the padding */
			dest->code[*dest_pc] = opcode;

			int old_pad = get_tableswitch_padding(src_pc);
			int new_pad = get_tableswitch_padding(*dest_pc);

			memset(dest->code + *dest_pc + 1, 0, new_pad);
			memcpy(dest->code + *dest_pc + 1 + new_pad,
			       src->code + src_pc + 1 + old_pad,
			       src_insn_size - 1 - old_pad);

			src_pc += src_insn_size;
			*dest_pc += src_insn_size - old_pad + new_pad;
		} else {
			memcpy(dest->code + *dest_pc,
			       src->code + src_pc,
			       src_insn_size);

			src_pc  += src_insn_size;
			*dest_pc += src_insn_size;
		}
	}

	assert (src_pc == n);

	return 0;
}

static int bytecode_copy(struct code_state *dest, unsigned long *dest_pc,
			 struct code_state *src, unsigned long src_pc,
			 struct subroutine *sub, struct pc_map *pc_map,
			 unsigned long count)
{
	unsigned long upto = src_pc + count;
	int err;

	if (src_pc <= sub->start_pc &&  sub->start_pc < upto) {
		unsigned long count_1, count_2;

		/*
		 * The copied region contains current subroutine which
		 * must not be copied.
		 */
		count_1 = sub->start_pc - src_pc;
		err = do_bytecode_copy(dest, dest_pc, src, src_pc, count_1,
				       pc_map);
		if (err)
			return err;

		src_pc += count_1;
		src_pc += sub->end_pc + sub->epilog_size - sub->start_pc;

		count_2 = upto - (sub->end_pc + sub->epilog_size);
		err = do_bytecode_copy(dest, dest_pc, src, src_pc, count_2,
				       pc_map);

		/* TODO: verify that immediately before subroutine
		   start there is either goto, athrow or return. */
	} else {
		err = do_bytecode_copy(dest, dest_pc, src, src_pc, count,
				       pc_map);
	}

	return err;
}

static int line_number_table_entry_compare(const void *p1, const void *p2)
{
	const struct cafebabe_line_number_table_entry *e1 = p1;
	const struct cafebabe_line_number_table_entry *e2 = p2;

	return e1->start_pc - e2->start_pc;
}

static int build_line_number_table(struct inlining_context *ctx)
{
	struct cafebabe_line_number_table_entry *new_table;
	const unsigned char *code;
	unsigned long code_length;
	unsigned long entry_count;
	unsigned long index;
	unsigned long pc;

	code = ctx->method->code_attribute.code;
	code_length = ctx->method->code_attribute.code_length;

	entry_count = bytecode_insn_count(ctx->code.code,
					  ctx->code.code_length);
	new_table = malloc(sizeof(*new_table) * entry_count);
	if (!new_table)
		return warn("out of memory"), -ENOMEM;

	index = 0;

	bytecode_for_each_insn(code, code_length, pc) {
		if (bc_is_jsr(code[pc]) || bc_is_ret(&code[pc]))
			continue;

		unsigned long line_no
			= bytecode_offset_to_line_no(ctx->method, pc);

		unsigned long *new_pc;
		pc_map_for_each_value(&ctx->pc_map, pc, new_pc) {
			assert(index < entry_count);

			struct cafebabe_line_number_table_entry *ent
				= &new_table[index++];

			for (unsigned long i = 0; i < index - 1; i++) {
				if (new_table[i].start_pc == *new_pc)
					error("already mapped");
			}

			ent->start_pc = *new_pc;
			ent->line_number = line_no;
		}
	}

	qsort(new_table, index, sizeof(*new_table),
	      line_number_table_entry_compare);

	ctx->line_number_table = new_table;
	ctx->line_number_table_length = index;

	return 0;
}

static struct cafebabe_code_attribute_exception *
eh_split(struct inlining_context *ctx, int i)
{
	struct cafebabe_code_attribute_exception *new_table;
	int move_size;
	int new_size;

	assert(i >= 0 && i < ctx->exception_table_length);

	new_size = (ctx->exception_table_length + 1) *
		sizeof(struct cafebabe_code_attribute_exception);
	new_table = realloc(ctx->exception_table, new_size);
	if (!new_table)
		return NULL;

	move_size = (ctx->exception_table_length - i - 1) * sizeof(*new_table);
	memmove(&new_table[i + 2], &new_table[i + 1], move_size);

	ctx->exception_table = new_table;
	ctx->exception_table_length++;

	return &new_table[i + 1];
}

static inline bool
eh_outside_subroutine(struct cafebabe_code_attribute_exception *eh,
		      struct subroutine *s)
{
	return eh->start_pc >= s->end_pc + s->epilog_size ||
		eh->end_pc <= s->start_pc;
}

static inline bool
eh_inside_subroutine(struct cafebabe_code_attribute_exception *eh,
		     struct subroutine *s)
{
	return eh->start_pc >= s->start_pc &&
		eh->end_pc <= s->end_pc + s->epilog_size;
}

static inline bool
eh_covers_subroutine(struct cafebabe_code_attribute_exception *eh,
		     struct subroutine *s)
{
	return eh->start_pc <= s->start_pc &&
		eh->end_pc >= s->end_pc + s->epilog_size;
}

static inline bool
eh_target_inside_subroutine(struct cafebabe_code_attribute_exception *eh,
			    struct subroutine *s)
{
	return eh->handler_pc >= s->start_pc &&
		eh->handler_pc < s->end_pc + s->epilog_size;
}

/**
 * Duplicates exception handler entry so that it covers all instances
 * of inlined subroutines it belongs to.
 */
static int
copy_exception_handler(struct inlining_context *ctx, struct subroutine *s,
		       int *eh_index, struct pc_map *pc_map)
{
	struct cafebabe_code_attribute_exception *eh;
	unsigned long eh_start_pc;
	unsigned long eh_end_pc;
	unsigned long eh_handler_pc;
	unsigned long body_start_pc;
	unsigned long body_size;

	eh		= &ctx->exception_table[*eh_index];

	body_start_pc	= s->start_pc + s->prolog_size;
	body_size	= subroutine_get_body_size(s);

	eh_handler_pc	= (unsigned long) eh->handler_pc;
	eh_start_pc	= max((unsigned long) eh->start_pc, body_start_pc);
	eh_end_pc	= min((unsigned long) eh->end_pc,   body_start_pc + body_size);

	for (int i = 0; i < s->nr_call_sites; i++) {
		struct cafebabe_code_attribute_exception *new_eh;
		unsigned long sub_start;

		sub_start = s->call_sites[i];
		if (pc_map_get_unique(pc_map, &sub_start))
			return warn("no or ambiguous mapping"), -EINVAL;

		new_eh = eh_split(ctx, *eh_index);
		if (!new_eh)
			return warn("out of memory"), -ENOMEM;

		unsigned long new_start_pc;
		if (pc_map_get_min_greater_than(pc_map, eh_start_pc, sub_start, &new_start_pc))
			error("pc_map_get_min_greater_than");

		new_eh->start_pc = new_start_pc;

		unsigned long new_end_pc;
		if (pc_map_get_min_greater_than(pc_map, eh_end_pc, sub_start, &new_end_pc))
			error("pc_map_get_min_greater_than");

		new_eh->end_pc = new_end_pc;

		new_eh->catch_type = ctx->exception_table[*eh_index].catch_type;

		if (eh_target_inside_subroutine(eh, s)) {
			unsigned long new_handler_pc;
			if (pc_map_get_min_greater_than(pc_map, eh_handler_pc, sub_start, &new_handler_pc))
				error("pc_map_get_min_greater_than");
			new_eh->handler_pc = new_handler_pc;
		} else {
			unsigned long pc = eh_handler_pc;
			if (pc_map_get_unique(pc_map, &pc))
				return -EINVAL;

			new_eh->handler_pc = pc;
		}
	}

	*eh_index = *eh_index + s->nr_call_sites;

	return 0;
}

static int
update_exception_handler(struct cafebabe_code_attribute_exception *eh,
			 struct pc_map *pc_map)
{
	unsigned long start_pc, end_pc, handler_pc;
	int err;

	start_pc = eh->start_pc;
	end_pc = eh->end_pc;
	handler_pc = eh->handler_pc;

	err = pc_map_get_unique(pc_map, &start_pc);
	if (err)
		return warn("no or ambiguous mapping"), -EINVAL;

	err = pc_map_get_unique(pc_map, &end_pc);
	if (err)
		return warn("no or ambiguous mapping"), -EINVAL;

	err = pc_map_get_unique(pc_map, &handler_pc);
	if (err) {
		/*
		 * If handler_pc is in realation with more than one
		 * address it means that the handler is within
		 * subroutine which have been previously inlined. We
		 * don't handle this.
		 */
		if (pc_map_has_value_for(pc_map, handler_pc))
			return warn("invalid handler"), -EINVAL;

		return warn("no mapping for handler"), -EINVAL;
	}

	eh->start_pc = start_pc;
	eh->end_pc = end_pc;
	eh->handler_pc = handler_pc;
	return 0;
}

static void eh_remove(struct inlining_context *ctx, int i)
{
	unsigned long size;

	size = (ctx->exception_table_length - i - 1) *
		sizeof(struct cafebabe_code_attribute_exception);

	memmove(ctx->exception_table + i,
		ctx->exception_table + i + 1,
		size);

	ctx->exception_table_length--;
}

static int
update_and_copy_exception_handlers(struct inlining_context *ctx,
				   struct subroutine *s,
				   struct pc_map *pc_map)
{
	int err;

	for (int i = 0; i < ctx->exception_table_length; i++) {
		struct cafebabe_code_attribute_exception *eh
			= &ctx->exception_table[i];

		if (eh_outside_subroutine(eh, s)) {
			if (update_exception_handler(eh, pc_map))
				return -EINVAL;
		} else if (eh_inside_subroutine(eh, s)) {
			int old_i = i;

			err = copy_exception_handler(ctx, s, &i, pc_map);
			if (err)
				return err;

			eh_remove(ctx, old_i);
			i--;
		} else if (eh_covers_subroutine(eh, s)) {
			int old_i = i;

			if (eh_target_inside_subroutine(eh, s))
				return warn("exception handler jumps into subroutine"),
					-EINVAL;

			err = copy_exception_handler(ctx, s, &i, pc_map);
			if (err)
				return err;

			eh = &ctx->exception_table[old_i];
			if (update_exception_handler(eh, pc_map))
				return -EINVAL;
		} else
			return warn("handler range spans subroutine boundary"),
				-EINVAL;
	}

	return 0;
}

static int verify_no_recursion(struct subroutine *sub)
{
	for (int i = 0; i < sub->nr_call_sites; i++)
		if (sub->call_sites[i] > sub->start_pc &&
		    sub->call_sites[i] < sub->end_pc)
			return warn("subroutine recursion"), -EINVAL;

	return 0;
}

static int do_inline_subroutine(struct inlining_context *ctx,
				struct subroutine *sub)
{
	struct code_state new_code;
	struct pc_map pc_map;
	unsigned long new_code_pc;
	unsigned long code_pc;
	int err;

	/* Allocate pc translation map for this inlining iteration */
	if (pc_map_init_empty(&pc_map, ctx->code.code_length + 1))
		return warn("out of memory"), -ENOMEM;

	/*
	 * After inlining process was started all bytecode offsets
	 * should be considered as no longer valid. We must calculate
	 * new values for this subroutine using the global pc
	 * translation map.
	 */
	err = update_bytecode_offsets(ctx, sub);
	if (err)
		goto error_update_bytecode_offsets;

	err = verify_no_recursion(sub);
	if (err)
		goto error_update_bytecode_offsets;

	unsigned long body_size = subroutine_get_body_size(sub);
	unsigned long sub_size = subroutine_get_size(sub);

	unsigned long approx_new_code_length = ctx->code.code_length - sub_size +
		sub->nr_call_sites * body_size -
		sub->call_sites_total_size;

	/* We do not know the final size of rewriten code */
	if (code_state_init_empty(&new_code, approx_new_code_length * 2))
		goto error_update_bytecode_offsets;

	subroutine_sort_call_sites(sub);

	new_code_pc = 0;
	code_pc = 0;

	for (int i = 0; i < sub->nr_call_sites; i++) {
		unsigned long count;

		count = sub->call_sites[i] - code_pc;
		bytecode_copy(&new_code, &new_code_pc, &ctx->code, code_pc,
			      sub, &pc_map, count);
		code_pc += count;

		/* Map address of jsr S to the address where body of S
		   will be inlined */
		pc_map_add(&pc_map, sub->call_sites[i], new_code_pc);

		/* Inline subroutine */
		err = do_bytecode_copy(&new_code, &new_code_pc, &ctx->code,
				       sub->prolog_size + sub->start_pc,
				       body_size, &pc_map);
		if (err)
			goto error;

		/* Map address of ret S to the address of first
		   instruction after inlined subroutine. */
		pc_map_add(&pc_map, sub->end_pc, new_code_pc);

		code_pc = next_pc(ctx->code.code, sub->call_sites[i]);
	}

	err = bytecode_copy(&new_code, &new_code_pc, &ctx->code, code_pc, sub,
			    &pc_map, ctx->code.code_length - code_pc);
	if (err)
		goto error;

	new_code.code_length = new_code_pc;

	/* Add mapping for end of bytecode pc. */
	err = pc_map_add(&pc_map, ctx->code.code_length, new_code.code_length);
	if (err)
		goto error;

	err = update_and_copy_exception_handlers(ctx, sub, &pc_map);
	if (err)
		goto error;

	err = pc_map_join(&ctx->pc_map, &pc_map);
	if (err)
		goto error;

	code_state_deinit(&ctx->code);
	ctx->code = new_code;

	pc_map_deinit(&pc_map);

	return 0;

 error:
	code_state_deinit(&new_code);
 error_update_bytecode_offsets:
	pc_map_deinit(&pc_map);

	return err;
}

static void
remove_subroutine(struct inlining_context *ctx, struct subroutine *sub)
{
	for (int i = 0; i < sub->nr_dependants; i++) {
		if (--sub->dependants[i]->nr_dependencies == 0) {
			/* This will speedup the lookup of independent
			 * subroutines. */
			list_move(&sub->dependants[i]->subroutine_list_node,
				  &ctx->subroutine_list);
		}
	}

	list_del(&sub->subroutine_list_node);
	free_subroutine(sub);
}

static struct subroutine *
find_independent_subroutine(struct inlining_context *ctx)
{
	struct subroutine *this;

	list_for_each_entry(this, &ctx->subroutine_list, subroutine_list_node)
		if (this->nr_dependencies == 0)
			return this;

	return NULL;
}

/**
 * Replace code, line number table and exception table of original
 * method with the ones calculated in @ctx.
 */
static void inlining_context_commit(struct inlining_context *ctx)
{
	free(ctx->method->code_attribute.exception_table);
	free(ctx->method->line_number_table_attribute.line_number_table);

	ctx->method->code_attribute.code = ctx->code.code;
	ctx->method->code_attribute.code_length = ctx->code.code_length;

	ctx->method->code_attribute.exception_table =
		ctx->exception_table;
	ctx->method->code_attribute.exception_table_length =
		ctx->exception_table_length;

	ctx->method->line_number_table_attribute.line_number_table =
		ctx->line_number_table;
	ctx->method->line_number_table_attribute.line_number_table_length =
		ctx->line_number_table_length;
}

static int
do_split_exception_handlers(struct inlining_context *ctx,
			    unsigned long call_site_pc,
			    unsigned long sub_start_pc,
			    unsigned long sub_end_pc)
{
	for (int i = 0; i < ctx->exception_table_length; i++) {
		struct cafebabe_code_attribute_exception *upper_eh;
		struct cafebabe_code_attribute_exception *eh;
		unsigned long lower_range_size;
		unsigned long upper_range_size;
		unsigned long upper_start;
		unsigned long upper_end;

		eh = &ctx->exception_table[i];

		if (eh->start_pc > call_site_pc || eh->end_pc <= call_site_pc)
			continue;

		if (eh->start_pc <= sub_start_pc && eh->end_pc >= sub_end_pc)
			continue;

		upper_end = eh->end_pc;
		upper_start = next_pc(ctx->method->code_attribute.code,
					   call_site_pc);

		eh->end_pc = call_site_pc;

		lower_range_size = eh->end_pc - eh->start_pc;
		upper_range_size = upper_end - upper_start;

		/*
		 * We do not remove exception handler entry when both
		 * ranges are empty because the exception handler's
		 * code cannot be removed. When there is no entry in
		 * exception table for exception handler code then it
		 * will not be detected as an exception handler and
		 * most probably will cause a compilation error.
		 */
		if (upper_range_size == 0)
			continue;

		if (lower_range_size == 0) {
			eh->start_pc = upper_start;
			eh->end_pc = upper_end;
			continue;
		}

		upper_eh = eh_split(ctx, i);
		if (!upper_eh)
			return warn("out of memory"), -ENOMEM;

		eh = &ctx->exception_table[i];

		upper_eh->start_pc = upper_start;
		upper_eh->end_pc = upper_end;
		upper_eh->handler_pc = eh->handler_pc;
		upper_eh->catch_type = eh->catch_type;

		i++;
	}

	return 0;
}

/**
 * When JSR S instruction is protected by an exception handler which does
 * not protect the S body then exception handler range must be split so that
 * it does not cover the JSR instruction.
 */
static int split_exception_handlers(struct inlining_context *ctx)
{
	struct subroutine *this;
	int err;

	list_for_each_entry(this, &ctx->subroutine_list, subroutine_list_node) {
		for (int i = 0; i < this->nr_call_sites; i++) {
			err = do_split_exception_handlers(ctx,
					this->call_sites[i], this->start_pc,
					this->end_pc + this->epilog_size);
			if (err)
				return err;
		}
	}

	return 0;
}

/**
 * There must not be such two subroutines S1 and S2 that
 * S1.start < S2.start < S1.end < S2.end.
 */
static int verify_correct_nesting(struct inlining_context *ctx)
{
	struct subroutine *this;
	int *coverage;

	coverage = zalloc(ctx->code.code_length * sizeof(int));
	if (!coverage)
		return warn("out of memory"), -ENOMEM;

	list_for_each_entry(this, &ctx->subroutine_list, subroutine_list_node) {
		int color = coverage[this->start_pc];

		for (unsigned long i = this->start_pc; i < this->end_pc; i++) {
			if (coverage[i] != color) {
				free(coverage);
				return warn("overlaping subroutines"), -EINVAL;
			}

			coverage[i]++;
		}
	}

	free(coverage);
	return 0;
}

/**
 * Changes to @method are made only when function returns successfully.
 */
int inline_subroutines(struct vm_method *method)
{
	struct inlining_context *ctx;
	int err;

	ctx = alloc_inlining_context(method);
	if (!ctx)
		return warn("out of memory"), -ENOMEM;

	err = detect_subroutines(ctx);
	if (err)
		goto out;

	if (list_is_empty(&ctx->subroutine_list))
		goto out;

	if (opt_trace_bytecode) {
		trace_printf("Code before subroutine inlining:\n");
		trace_bytecode(method);
	}

	err = scan_subroutines(ctx);
	if (err)
		goto out;

	err = verify_correct_nesting(ctx);
	if (err)
		goto out;

	err = split_exception_handlers(ctx);
	if (err)
		goto out;

	while (!list_is_empty(&ctx->subroutine_list)) {
		struct subroutine *sub;

		sub = find_independent_subroutine(ctx);
		if (!sub) {
			warn("mutual dependencies between subroutines");
			err = -EINVAL;
			goto out;
		}

		err = do_inline_subroutine(ctx, sub);
		if (err)
			goto out;

		remove_subroutine(ctx, sub);
	}

	err = update_branch_targets(ctx);
	if (err)
		goto out;

	err = build_line_number_table(ctx);
	if (err)
		goto out;

	inlining_context_commit(ctx);

 out:
	free_inlining_context(ctx);
	return err;
}

int fail_subroutine_bc(struct parse_context *ctx)
{
	error("subroutine bytecode (%d) should have been eliminated by inlining", ctx->opc);

	return -1;
}
