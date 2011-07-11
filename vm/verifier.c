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

struct verifier_block *alloc_verifier_block(int nb_vars)
{
	struct verifier_block *newb;

	newb = malloc(sizeof(struct verifier_block));
	if (!newb)
		return NULL;

	newb->initial_state = alloc_verifier_state(nb_vars);
	if (newb->initial_state)
		return NULL;

	newb->final_state = alloc_verifier_state(nb_vars);
	if (newb->final_state)
		return NULL;

	return newb;
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

static int verify_local_index(struct verifier_state *s, enum vm_type vm_type, unsigned int idx)
{
	if (idx >= s->nb_vars)
		return E_WRONG_LOCAL_INDEX;

	if (vm_type_is_pair(vm_type))
		if (idx+1 >= s->nb_vars)
			return E_WRONG_LOCAL_INDEX;

	return 0;
}

int peek_verifier_local_var_type(struct verifier_state *s, enum vm_type vm_type, unsigned int idx)
{
	int err;

	err = verify_local_index(s, vm_type, idx);
	if (err)
		return err;

	if (s->vars[idx].state == UNDEFINED)
		return E_TYPE_CHECKING;

	/* If the state is unknown we cannot tell if the transition is valid.
	 * But we can infer the type of the variable for the rest of the block.
	 */
	if (s->vars[idx].state == UNKNOWN) {
		if (vm_type_is_pair(vm_type) && s->vars[idx+1].state != UNKNOWN)
			return E_TYPE_CHECKING;

		return store_verifier_local_var(s, vm_type, idx);
	}

	if (s->vars[idx].op.vm_type != vm_type)
		return E_TYPE_CHECKING;

	if (vm_type_is_pair(vm_type)) {
		if (s->vars[idx+1].state != DEFINED)
			return E_TYPE_CHECKING;
		if (s->vars[idx].op.is_fragment || s->vars[idx+1].op.vm_type != vm_type || !s->vars[idx+1].op.is_fragment)
			return E_TYPE_CHECKING;
	}

	return 0;
}

int store_verifier_local_var(struct verifier_state *s, enum vm_type vm_type, unsigned int idx)
{
	int err;

	err = verify_local_index(s, vm_type, idx);
	if (err)
		return err;

	s->vars[idx].state = DEFINED;
	s->vars[idx].op.vm_type = vm_type;

	if (!vm_type_is_pair(vm_type))
		goto out;

	s->vars[idx].op.is_fragment = false;
	s->vars[idx+1].state = DEFINED;
	s->vars[idx+1].op.vm_type = vm_type;
	s->vars[idx+1].op.is_fragment = true;

out:
	return 0;
}

int undef_verifier_local_var(struct verifier_state *s, unsigned int idx)
{
	int err;

	/* We just need a non-double type here to make sure we do not fail
	 * at undefining the local variable with the higher index.
	 */
	err = verify_local_index(s, J_INT, idx);
	if (err)
		return err;

	s->vars[idx].state = UNDEFINED;

	return 0;
}

int push_vrf_op(struct verifier_state *s, enum vm_type vm_type)
{
	struct verifier_stack *new;

	new = alloc_verifier_stack(vm_type);
	if (!new)
		return ENOMEM;

	if (vm_type_is_pair(vm_type)) {
		list_add(&new->slots, &s->stack->slots);

		new = alloc_verifier_stack(vm_type);
		if (!new)
			return ENOMEM;

		new->op.is_fragment = true;
	}

	list_add(&new->slots, &s->stack->slots);

	return 0;
}

int pop_vrf_op(struct verifier_state *s, enum vm_type vm_type)
{
	struct verifier_stack *el;

	el = list_first_entry(&s->stack->slots, struct verifier_stack, slots);

	/* VM_TYPE_MAX marks the bottom of the stack. If we see it we can
	 * then infer what would have been needed in the the initial stack.
	 */
	if (el->op.vm_type == VM_TYPE_MAX)
		return 0;

	if (vm_type_is_pair(vm_type)) {
		if (el->op.vm_type != vm_type || !el->op.is_fragment)
			return E_TYPE_CHECKING;

		list_del(&el->slots);
		free(el);

		el = list_first_entry(&s->stack->slots, struct verifier_stack, slots);
	}

	if (el->op.vm_type != vm_type || el->op.is_fragment)
		return E_TYPE_CHECKING;

	list_del(&el->slots);
	free(el);

	return 0;
}

int peek_vrf_op(struct verifier_state *s, enum vm_type vm_type)
{
	struct verifier_stack *el;

	el = list_first_entry(&s->stack->slots, struct verifier_stack, slots);

	/* VM_TYPE_MAX marks the bottom of the stack. If we see it we can
	 * then infer what would have been needed in the the initial stack.
	 */
	if (el->op.vm_type == VM_TYPE_MAX)
		return 0;

	if (vm_type_is_pair(vm_type)) {
		if (el->op.vm_type != vm_type || !el->op.is_fragment)
			return E_TYPE_CHECKING;

		el = list_entry(el->slots.next, struct verifier_stack, slots);
	}

	if (el->op.vm_type != vm_type || el->op.is_fragment)
		return E_TYPE_CHECKING;

	return 0;
}

static int cmp_verifier_op(struct verifier_operand *op1, struct verifier_operand *op2)
{
	if (op1->vm_type != op2->vm_type)
		return E_TYPE_CHECKING;

	if (op1->is_fragment != op2->is_fragment)
		return E_TYPE_CHECKING;

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

static int verify_instruction(struct verify_context *vrf)
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

#if 0
	/* Would just produce too much output right now ! */
	if (err == E_NOT_IMPLEMENTED)
		warn("verifying not implemented for instruction at PC=%lu", vrf->offset);
#endif

	return err;
}

static int do_vm_method_verify(struct vm_method *vmm)
{
	int err;
	struct verify_context vrf;
	struct bytecode_buffer buffer;

	buffer.buffer = vmm->code_attribute.code;
	buffer.pos = 0;

	vrf.code_size = vmm->code_attribute.code_length;
	vrf.code = vmm->code_attribute.code;

	vrf.buffer = &buffer;

	while (buffer.pos < vrf.code_size) {
		vrf.offset = vrf.buffer->pos;	/* this is fragile */

		err = verify_instruction(&vrf);
		if (err)
			return err;
	}

	return 0;
}

int vm_method_verify(struct vm_method *vmm)
{
	int err;

	err = do_vm_method_verify(vmm);

	if (err == E_NOT_IMPLEMENTED)
		return 0;

	if (err)
		signal_new_exception(vm_java_lang_VerifyError, NULL);

	return err;
}
