#include "jit/bc-offset-mapping.h"

#include "jit/bytecode-to-ir.h"
#include "jit/expression.h"
#include "jit/exception.h"
#include "jit/subroutine.h"
#include "jit/statement.h"
#include "jit/tree-node.h"
#include "jit/compiler.h"

#include "cafebabe/constant_pool.h"

#include "vm/bytecode.h"
#include "vm/method.h"
#include "vm/die.h"
#include "vm/preload.h"
#include "vm/verifier.h"

#include "lib/stack.h"
#include "lib/bitset.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define verify_goto_w		verify_goto
#define verify_jsr_w		verify_jsr

#define BYTECODE(opc, name, size, type) [opc] = verify_ ## name,
static verify_fn_t verifiers[] = {
#  include <vm/bytecode-def.h>
};
#undef BYTECODE

struct verifier_local_var *alloc_verifier_local_var(int nb_vars)
{
	int i;
	struct verifier_local_var *vars;

	vars = malloc(nb_vars * sizeof(struct verifier_local_var));
	if (!vars)
		return NULL;

	for (i=0;i<nb_vars;i++) {
		vars[i].state = UNKNOWN;
		vars[i].op.is_fragment = false;
	}

	return vars;
}

struct verifier_stack *alloc_verifier_stack(enum vm_type vm_type)
{
	struct verifier_stack *newst;

	newst = malloc(sizeof(struct verifier_stack));
	if (!newst)
		return NULL;

	newst->op.vm_type = vm_type;
	newst->op.is_fragment = false;

	return newst;
}

struct verifier_state *alloc_verifier_state(int nb_vars)
{
	struct verifier_state *news;

	news = malloc(sizeof(struct verifier_state));
	if (!news)
		return NULL;

	news->stack = alloc_verifier_stack(VM_TYPE_MAX);
	if (!news->stack)
		return NULL;
	INIT_LIST_HEAD(&news->stack->slots);

	news->vars = alloc_verifier_local_var(nb_vars);
	news->nb_vars = nb_vars;

	return news;
}

struct verifier_block *alloc_verifier_block(struct verifier_context *vrf, uint32_t begin_offset)
{
	struct verifier_block *newb;

	newb = malloc(sizeof(struct verifier_block));
	if (!newb)
		return NULL;

	newb->begin_offset = begin_offset;
	newb->code = vrf->code + begin_offset;
	newb->pc = 0;

	newb->initial_state = alloc_verifier_state(vrf->max_locals);
	if (!newb->initial_state)
		return NULL;

	newb->final_state = alloc_verifier_state(vrf->max_locals);
	if (!newb->final_state)
		return NULL;

	newb->following_offsets = malloc(sizeof(uint32_t) * INITIAL_FOLLOWERS_SIZE);
	newb->nb_followers = 0;

	newb->parent_ctx = vrf;

	return newb;
}

struct verifier_jump_destinations *alloc_verifier_jump_destinations(unsigned long dest)
{
	struct verifier_jump_destinations *newjd;

	newjd = malloc(sizeof(struct verifier_jump_destinations));
	if (!newjd)
		return NULL;

	newjd->dest = dest;

	return newjd;
}

struct verifier_context *alloc_verifier_context(struct vm_method *vmm)
{
	struct verifier_context *vrf;

	vrf = malloc(sizeof(struct verifier_context));
	if (!vrf)
		return NULL;

	vrf->method = vmm;

	vrf->code = vmm->code_attribute.code;
	vrf->code_size = vmm->code_attribute.code_length;

	vrf->max_locals = vmm->code_attribute.max_locals;
	vrf->max_stack = vmm->code_attribute.max_stack;

	vrf->is_wide = false;

	vrf->jmp_dests = alloc_verifier_jump_destinations(0);
	if (!vrf->jmp_dests)
		return NULL;
	INIT_LIST_HEAD(&vrf->jmp_dests->list);

	vrf->vb_list = alloc_verifier_block(vrf, 0);
	if (!vrf->vb_list)
		return NULL;
	INIT_LIST_HEAD(&vrf->vb_list->blocks);

	return vrf;
}

void free_verifier_local_var(struct verifier_local_var *vars)
{
	free(vars);
}

void free_verifier_stack(struct verifier_stack *stack)
{
	struct verifier_stack *ptr, *tmp;

	list_for_each_entry_safe(stack, tmp, &stack->slots, slots) {
		list_del(&ptr->slots);
		free(ptr);
	}

	free(stack);
}

void free_verifier_jump_destinations(struct verifier_jump_destinations *jmp_dests)
{
	free(jmp_dests);
}

void free_verifier_state(struct verifier_state *state)
{
	free(state->vars);
	free_verifier_stack(state->stack);
	free(state);
}

void free_verifier_block(struct verifier_block *block)
{
	free_verifier_state(block->initial_state);
	free_verifier_state(block->final_state);
	free(block);
}

void free_verifier_context(struct verifier_context *ctx)
{
	struct verifier_block *tmp, *ptr;

	list_for_each_entry_safe(ptr, tmp, &ctx->vb_list->blocks, blocks) {
		free_verifier_block(ptr);
	}

	free(ctx);
}

#if 0
static enum cafebabe_constant_tag vm_type_to_constant_pool_tag(enum vm_type vm_type) {
	switch (vm_type) {
		case J_INT:
			return CAFEBABE_CONSTANT_TAG_INTEGER;
		case J_LONG:
			return CAFEBABE_CONSTANT_TAG_LONG;
		case J_FLOAT:
			return CAFEBABE_CONSTANT_TAG_FLOAT;
		case J_DOUBLE:
			return CAFEBABE_CONSTANT_TAG_DOUBLE;
		case J_REFERENCE:
			return CAFEBABE_CONSTANT_TAG_STRING;
		default:
			return -1;
	}
}
#endif

static int verify_constant_tag(struct verifier_context *vrf, enum cafebabe_constant_tag tag, unsigned int index)
{
	const struct cafebabe_class *class = vrf->method->class->class;

	if (index >= class->constant_pool_count) {
		printf("Invalid reference to constant %u.\n", index);
		return E_WRONG_CONSTANT_POOL_INDEX;
	}

	if (class->constant_pool[index].tag != tag) {
		return E_TYPE_CHECKING;}

	return 0;
}

static inline void undef_vrf_lvar(struct verifier_state *s, unsigned int idx)
{
	s->vars[idx].state = UNDEFINED;
}

static inline void def_vrf_lvar(struct verifier_state *s, enum vm_type vm_type, unsigned int idx)
{
	s->vars[idx].state = DEFINED;
	s->vars[idx].op.vm_type = vm_type;

	if (!vm_type_is_pair(vm_type))
		return;

	s->vars[idx].op.is_fragment = false;
	s->vars[idx+1].state = DEFINED;
	s->vars[idx+1].op.vm_type = vm_type;
	s->vars[idx+1].op.is_fragment = true;
}

int peek_vrf_lvar(struct verifier_block *b, enum vm_type vm_type, unsigned int idx)
{
	struct verifier_state *final = b->final_state, *init = b->initial_state;

	/* If the state is unknown we cannot tell if the transition is valid.
	 * But we can infer the type of the variable for the rest of the block.
	 */
	if (final->vars[idx].state == UNKNOWN) {
		if (vm_type_is_pair(vm_type) && final->vars[idx+1].state != UNKNOWN)
			return E_TYPE_CHECKING;

		def_vrf_lvar(init, vm_type, idx);
		return store_vrf_lvar(b, vm_type, idx);
	}

	if (final->vars[idx].op.vm_type != vm_type)
		return E_TYPE_CHECKING;

	if (vm_type_is_pair(vm_type)) {
		if (final->vars[idx+1].state != DEFINED)
			return E_TYPE_CHECKING;
		if (final->vars[idx].op.is_fragment || final->vars[idx+1].op.vm_type != vm_type || !final->vars[idx+1].op.is_fragment)
			return E_TYPE_CHECKING;
	}

	return 0;
}

int store_vrf_lvar(struct verifier_block *s, enum vm_type vm_type, unsigned int idx)
{
	def_vrf_lvar(s->final_state, vm_type, idx);

	return 0;
}

static inline int tail_def_vrf_op(struct verifier_state *s, enum vm_type vm_type)
{
	struct verifier_stack *new;

	new = alloc_verifier_stack(vm_type);
	if (!new)
		return ENOMEM;

	if (vm_type_is_pair(vm_type)) {
		new->op.is_fragment = true;

		list_add_tail(&new->slots, &s->stack->slots);

		new = alloc_verifier_stack(vm_type);
		if (!new)
			return ENOMEM;
	}

	list_add_tail(&new->slots, &s->stack->slots);

	return 0;
}

int vrf_stack_size(struct verifier_stack *st)
{
	return list_size(&st->slots);
}

int push_vrf_op(struct verifier_block *b, enum vm_type vm_type)
{
	struct verifier_stack *new;

	new = alloc_verifier_stack(vm_type);
	if (!new)
		return ENOMEM;

	if (vm_type_is_pair(vm_type)) {
		list_add(&new->slots, &b->final_state->stack->slots);

		new = alloc_verifier_stack(vm_type);
		if (!new)
			return ENOMEM;

		new->op.is_fragment = true;
	}

	list_add(&new->slots, &b->final_state->stack->slots);

	return 0;
}

int pop_vrf_op(struct verifier_block *b, enum vm_type vm_type)
{
	struct verifier_state *final = b->final_state, *init = b->initial_state;
	struct verifier_stack *el;

	el = list_first_entry(&final->stack->slots, struct verifier_stack, slots);

	/* VM_TYPE_MAX marks the bottom of the stack. If we see it we can
	 * then infer what would have been needed in the the initial stack.
	 */
	if (el->op.vm_type == VM_TYPE_MAX)
		return tail_def_vrf_op(init, vm_type);

	if (vm_type_is_pair(vm_type)) {
		if (el->op.vm_type != vm_type || !el->op.is_fragment)
			return E_TYPE_CHECKING;

		list_del(&el->slots);
		free(el);

		el = list_first_entry(&final->stack->slots, struct verifier_stack, slots);
	}

	if (el->op.vm_type != vm_type || el->op.is_fragment)
		return E_TYPE_CHECKING;

	list_del(&el->slots);
	free(el);

	return 0;
}

int peek_vrf_op(struct verifier_block *b, enum vm_type vm_type)
{
	struct verifier_state *final = b->final_state, *init = b->initial_state;
	struct verifier_stack *el;

	el = list_first_entry(&final->stack->slots, struct verifier_stack, slots);

	/* VM_TYPE_MAX marks the bottom of the stack. If we see it we can
	 * then infer what would have been needed in the the initial stack.
	 */
	if (el->op.vm_type == VM_TYPE_MAX)
		return tail_def_vrf_op(init, vm_type);

	if (vm_type_is_pair(vm_type)) {
		if (el->op.vm_type != vm_type || !el->op.is_fragment)
			return E_TYPE_CHECKING;

		el = list_entry(el->slots.next, struct verifier_stack, slots);
	}

	if (el->op.vm_type != vm_type || el->op.is_fragment)
		return E_TYPE_CHECKING;

	return 0;
}

static inline int valid_dest(int offset, unsigned long pc, unsigned long code_size)
{
	int err = 0;

	if ((long) pc + offset < 0)
		err = E_INVALID_BRANCH;

	if (err) {
		printf("Invalid negative jump destination %ld.\n", (long) pc + offset);
		return err;
	}

	if (pc + offset >= code_size)
		err = E_INVALID_BRANCH;

	if (err) {
		printf("Invalid jump destination %lu.\n", (unsigned long) pc + offset);
		return err;
	}

	return 0;
}

int add_jump_destination(struct verifier_jump_destinations *jd, unsigned long dest)
{
	struct verifier_jump_destinations *cur, *new;

	list_for_each_entry(cur, &jd->list, list) {
		if (dest > cur->dest)
			continue;
		else if (dest == cur->dest)
			return 0;

		new = alloc_verifier_jump_destinations(dest);
		if (!new)
			return ENOMEM;

		list_add_tail(&new->list, &cur->list);

		return 0;
	}

	/* If we're here the new destination is greater than the current
	 * max of the list, so we just add it at the end.
	 */
	new = alloc_verifier_jump_destinations(dest);
	if (!new)
		return ENOMEM;

	list_add_tail(&new->list, &jd->list);

	return 0;
}

int add_tableswitch_destinations(struct verifier_jump_destinations *jd, const unsigned char *code, unsigned long pc, unsigned long code_size)
{
	int high, low, def, err, offset;
	unsigned long base_pc, end_pc;

	base_pc = pc;
	pc += 1 + get_tableswitch_padding(pc);

	def = read_s32(code + pc);
	low = read_s32(code + pc + 4);
	high = read_s32(code + pc + 8);
	pc += 12;

	err = valid_dest(def, base_pc, code_size);
	if (err)
		return err;

	err = add_jump_destination(jd, (unsigned long) base_pc + def);
	if (err)
		return err;

	end_pc = (high - low) * 4 + pc;

	while (pc < end_pc) {
		offset = read_s32(code + pc);

		err = valid_dest(offset, base_pc, code_size);
		if (err)
			return err;

		err = add_jump_destination(jd, (unsigned long) base_pc + offset);
		if (err)
			return err;

		pc += 4;
	}

	return 0;
}

int add_lookupswitch_destinations(struct verifier_jump_destinations *jd, const unsigned char *code, unsigned long pc, unsigned long code_size)
{
	int npairs, def, err, offset;
	unsigned long base_pc, end_pc;

	base_pc = pc;
	pc += 1 + get_tableswitch_padding(pc);

	def = read_s32(code + pc);
	npairs = read_s32(code + pc + 4);
	pc += 8;

	err = valid_dest(def, base_pc, code_size);
	if (err)
		return err;

	err = add_jump_destination(jd, (unsigned long) base_pc + def);
	if (err)
		return err;

	end_pc = npairs * 4 * 2 + pc;
	pc += 4; /* We position PC on the offsets. */

	while (pc < end_pc) {
		offset = read_s32(code + pc);

		err = valid_dest(offset, base_pc, code_size);
		if (err)
			return err;

		err = add_jump_destination(jd, (unsigned long) base_pc + offset);
		if (err)
			return err;

		pc += 8;
	}

	return 0;
}

static const char *vm_type_to_str(enum vm_type vm_type)
{
	switch(vm_type) {
		case J_VOID: return "J_VOID";
		case J_REFERENCE: return "J_REFERENCE";
		case J_BYTE: return "J_BYTE";
		case J_SHORT: return "J_SHORT";
		case J_INT: return "J_INT";
		case J_LONG: return "J_LONG";
		case J_CHAR: return "J_CHAR";
		case J_FLOAT: return "J_FLOAT";
		case J_DOUBLE: return "J_DOUBLE";
		case J_BOOLEAN: return "J_BOOLEAN";
		case J_RETURN_ADDRESS: return "J_RETURN_ADDRESS";
		case VM_TYPE_MAX: return "VM_TYPE_MAX";
		default: return "Unknown type";
	}
}

static int cmp_verifier_op(struct verifier_operand *op1, struct verifier_operand *op2)
{
	if (op1->vm_type != op2->vm_type) {
		printf("Operand comparison: different types: %s and %s.\n", vm_type_to_str(op1->vm_type), vm_type_to_str(op2->vm_type));
		return E_TYPE_CHECKING;
	}

	if (op1->is_fragment != op2->is_fragment) {
		printf("Operand comparison: one operand is fragment and the other is not.\n");
		return E_TYPE_CHECKING;
	}

	return 0;
}

int transition_verifier_stack(struct verifier_stack *stc, struct verifier_stack *stn)
{
	int err;
	struct verifier_stack *cp, *np;

	/* If the stack of the current state is smaller than the stack
	 * expected in the next state we know the states are
	 * imcompatible.
	 */
	if (list_size(&stc->slots) < list_size(&stn->slots))
		return E_TYPE_CHECKING;

	/* The current stack may contain more elements than the one from
	 * the next stack, but we are not interested in the additional
	 * elements here (we will adjust the next stack later). What we
	 * need is to compare the top elements until the next stack is
	 * empty.
	 */
	np = list_first_entry(&stn->slots, struct verifier_stack, slots);

	list_for_each_entry(cp, &stc->slots, slots) {

		/* The type VM_TYPE_MAX is used to signal the stack bottom.
		*/
		if (np->op.vm_type == VM_TYPE_MAX)
			break;

		err = cmp_verifier_op(&cp->op, &np->op);
		if (err)
			return err;

		np = list_entry(np->slots.next, struct verifier_stack, slots);
	}

	return 0;
}

int transition_verifier_local_var(struct verifier_local_var *varsc, struct verifier_local_var *varsn, int nb_vars)
{
	int i, err;

	for (i=0;i<nb_vars;i++) {
		/* if the variable is unknown in the next state the
		 * transition is valid automatically.
		 */
		if (varsn->state == UNKNOWN)
			goto next_iteration;

		/* if the variable is defined in the next state and not
		 * in the current one the transition is invalid.
		 */
		if (varsn->state == DEFINED && varsc->state == UNDEFINED)
			return E_TYPE_CHECKING;

		err = cmp_verifier_op(&varsc->op, &varsn->op);
		if (err)
			return err;

next_iteration:
		varsc++;
		varsn++;
	}

	return 0;
}

int transition_verifier_state(struct verifier_state *sc, struct verifier_state *sn)
{
	int err;

	err = transition_verifier_stack(sc->stack, sn->stack);
	if (err)
		return err;

	/* We assume the number of variables is set for the method and
	 * will not differ between sc and sn.
	 */
	err = transition_verifier_local_var(sc->vars, sn->vars, sc->nb_vars);
	if (err)
		return err;

	return 0;
}

#if 0
static int verify_instruction(struct verifier_context *vrf)
{
	verify_fn_t verify;

	vrf->opc = bytecode_read_u8(vrf->buffer);

	if (vrf->opc >= ARRAY_SIZE(verifiers))
		return warn("%d out of bounds", vrf->opc), -EINVAL;

	verify = verifiers[vrf->opc];
	if (!verify)
		return warn("no verifier function for %d found", vrf->opc), -EINVAL;

	int err = verify(vrf);

	if (err)
		warn("verifying error at PC=%lu", vrf->offset);

	if (err == E_NOT_IMPLEMENTED)
		warn("verifying not implemented for instruction at PC=%lu", vrf->offset);

	return err;
}
#endif

static int verify_exception_table(struct cafebabe_code_attribute *ca, struct verifier_jump_destinations *jd)
{
	int err, i = 1;
	struct cafebabe_code_attribute_exception *ex = ca->exception_table;

	if (ca->exception_table_length == 0)
		return 0;

	do {
		if (ex->start_pc > ex->end_pc) {
			puts("Exception table: protected code section borns are inversed.");
			return E_INVALID_EXCEPTION_HANDLER;
		}

		if (ex->end_pc > ca->code_length) {
			puts("Exception table: protected code section ends after method code end.");
			return E_INVALID_EXCEPTION_HANDLER;
		}

		err = add_jump_destination(jd, ex->handler_pc);
		if (err)
			return err;

		ex++;
		i++;
	} while (i < ca->exception_table_length);

	return 0;
}


static void verify_error(int err, unsigned long pos, struct verifier_context *vrf)
{
	const char *str;
	switch (err) {
		case E_MALFORMED_BC:
			str = "VerifyError: malformed bytecode";
			break;
		case E_TYPE_CHECKING:
			str = "VerifyError: type checking";
			break;
		case E_INVALID_BRANCH:
			str = "VerifyError: invalid jump";
			break;
		case E_WRONG_LOCAL_INDEX:
			str = "VerifyError: reference to a too high local variable";
			break;
		case E_WRONG_CONSTANT_POOL_INDEX:
			str = "VerifyError: reference to a too high constant";
			break;
		case E_FALLING_OFF:
			str = "VerifyError: falling off method code";
			break;
		case E_INVALID_EXCEPTION_HANDLER:
			printf("VerifyError: error in exception table in method %s of class %s.\n", vrf->method->name, vrf->method->class->name);
			return;
		default:
			str = "VerifyError: unknown error (%i)", err;
	}

	printf("%s at PC=%lu (code size: %lu) in method %s of class %s.\n", str, pos, vrf->code_size, vrf->method->name, vrf->method->class->name);
}

static int verifier_first_pass(struct verifier_context *vrf)
{
	int offset, err = 0;
	long next_insn_offset;
	unsigned int index;
	unsigned long pc;
	unsigned char *code, *last_insn_ptr = vrf->code;
	struct bitset *insn_map;
	struct verifier_jump_destinations *jd;

	pc = 0;
	code = vrf->code;
	insn_map = alloc_bitset(vrf->code_size);

	while (pc < vrf->code_size) {

		/* Ensure the code doesn't end in the middle of an
		 * instruction and that the opcode is valid.
		 */
		next_insn_offset = bc_insn_size_safe(code, pc, vrf->code_size);
		if (next_insn_offset < 0)
			err = next_insn_offset;
		if (err)
			goto out;

		/* Adding the position of the current instruction to the
		 * table for later comparison with possible jump
		 * destinations.
		 */
		set_bit(insn_map->bits, pc);

		if (bc_is_branch(code[pc])) {
			/* In case of the RET instruction, the jump
			 * destination is stored in a local variable, so we
			 * cannot check anything about it as we do not
			 * manipulate values here. Later type checking will
			 * ensure no shenanigans are intented involving the
			 * return address...
			 */
			if (bc_is_ret(code))
				goto next_iteration;

			else if (code[pc] == OPC_TABLESWITCH) {
				err = add_tableswitch_destinations(vrf->jmp_dests, code, pc, vrf->code_size);
				if (!err)
					goto next_iteration;
			}

			else if (code[pc] == OPC_LOOKUPSWITCH) {
				err = add_lookupswitch_destinations(vrf->jmp_dests, code, pc, vrf->code_size);
				if (!err)
					goto next_iteration;
			}

			if (err)
				goto out;

			offset = bc_target_off(code + pc);

			/* Check if the jump destination is possible. */
			err = valid_dest(offset, pc, vrf->code_size);
			if (err)
				goto out;

			/* And register the possible jump destination. */
			err = add_jump_destination(vrf->jmp_dests, pc + offset);
			if (err)
				goto out;
		}

		else if (bc_uses_local_var(code[pc])) {
			index = get_local_var_index(code, pc);
			if (index < vrf->max_locals)
				goto next_iteration;

			printf("Invalid local variable %u referenced.\n", index);
			err = E_WRONG_LOCAL_INDEX;
			goto out;
		}

		else if (bc_is_ldc(code[pc])) {
			if (code[pc] == OPC_LDC)
				index = read_u8(code + pc + 1);
			else
				index = read_u16(code + pc + 1);

			if (code[pc] == OPC_LDC2_W) {
				err = verify_constant_tag(vrf, CAFEBABE_CONSTANT_TAG_DOUBLE, index);
				if (!err)
					goto next_iteration;

				err = verify_constant_tag(vrf, CAFEBABE_CONSTANT_TAG_LONG, index);
			}
			else {
				err = verify_constant_tag(vrf, CAFEBABE_CONSTANT_TAG_INTEGER, index);
				if (!err)
					goto next_iteration;

				err = verify_constant_tag(vrf, CAFEBABE_CONSTANT_TAG_FLOAT, index);
				if (!err)
					goto next_iteration;

				err = verify_constant_tag(vrf, CAFEBABE_CONSTANT_TAG_STRING, index);
				if (!err)
					goto next_iteration;

				/* This seems illegal but Class does load a class reference with ldc ... */
				err = verify_constant_tag(vrf, CAFEBABE_CONSTANT_TAG_CLASS, index);
			}

			if (err) {
				printf("Invalid constant type for constant index %u.\n", index);
				goto out;
			}
		}

next_iteration:
		last_insn_ptr = code + pc;
		pc += next_insn_offset;
	}

	/* Check the exception table of the method (that the borns of the
	 * interval are in order, the end born doesn't go past the end of
	 * the actual code), and adds the handlers as possible jump
	 * destinations.
	 */
	err = verify_exception_table(&vrf->method->code_attribute, vrf->jmp_dests);
	if (err)
		goto out;

	/* Check the jump destinations against the instruction map to
	 * ensure no jump is trying the redirect the control flow to an
	 * operand instead of an instruction start.
	 */
	list_for_each_entry(jd, &vrf->jmp_dests->list, list) {
		if (test_bit(insn_map->bits, jd->dest))
			continue;

		printf("Invalid jump destination %lu.\n", jd->dest);
		err = E_INVALID_BRANCH;

		/* Here we do something that may look strange but
		 * that won't hurt performance anyway because it
		 * only happens if there is actually an error : we
		 * go through the method code another time to find
		 * the first instruction trying to jump at the
		 * invalid offset, so that we can print the right
		 * PC with verify_error at the exit.
		 */

		pc = 0;
		while (pc < vrf->code_size) {
			next_insn_offset = bc_insn_size_safe(code, pc, vrf->code_size);
			if (bc_is_branch(code[pc])) {
				if (bc_is_ret(code))
					goto _next_iteration;

				offset = bc_target_off(code + pc);

				if (pc + offset == jd->dest)
					goto out;
			}
_next_iteration:
			pc += next_insn_offset;
		}
	}

	/* If the PC is not right on the end we're missing some part of the
	 * last instruction.
	 */
	if (pc != vrf->code_size)
		err = E_MALFORMED_BC;

	/* If the last instruction was not a return of some kind or an
	 * inconditional jump there is a chance of 'falling off' the code
	 * of the method, which is illegal.
	 */

	if (!(bc_is_unconditionnal_branch(last_insn_ptr)))
		err = E_FALLING_OFF;

out:
	free(insn_map);

	if (err) {
		verify_error(err, pc, vrf);
		return err;
	}

	return 0;
}

int vm_method_verify(struct vm_method *vmm)
{
	int err;
	struct verifier_context *vrf;

	if (vmm->code_attribute.code_length == 0)
		return 0; /* Nothing to check ! */

	vrf = alloc_verifier_context(vmm);
	if (!vrf)
		return ENOMEM;

	err = verifier_first_pass(vrf);

	if (err == E_NOT_IMPLEMENTED)
		return 0;

	if (err)
		puts("Would have raised VerifyError exception.");
	//signal_new_exception(vm_java_lang_VerifyError, NULL);

	return err;
}
