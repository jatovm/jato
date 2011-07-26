#include "vm/verifier.h"
#include "lib/list.h"

#include <libharness.h>
#include <stdlib.h>

void test_local_var_utilities(void)
{
	struct verifier_context *chk;
	struct verifier_block *b;
	struct verifier_state *final, *init;

	chk = malloc(sizeof(struct verifier_context));
	assert_not_null(chk);
	chk->max_locals = 5;

	/* allocation & initialization */
	b = alloc_verifier_block(chk, 0);
	assert_not_null(b);
	final = b->final_state;
	init = b->initial_state;
	assert_int_equals(UNKNOWN, final->vars[0].state);
	assert_false(final->vars[0].op.is_fragment);

	/* peek simple unknown */
	assert_int_equals(0, peek_vrf_lvar(b, J_INT, 2));
	assert_int_equals(DEFINED, init->vars[2].state);
	assert_int_equals(J_INT, init->vars[2].op.vm_type);
	assert_int_equals(DEFINED, final->vars[2].state);
	assert_int_equals(J_INT, final->vars[2].op.vm_type);

	/* peek double unknown */
	assert_int_equals(0, peek_vrf_lvar(b, J_DOUBLE, 3));
	assert_int_equals(DEFINED, init->vars[3].state);
	assert_int_equals(J_DOUBLE, init->vars[3].op.vm_type);
	assert_int_equals(DEFINED, init->vars[4].state);
	assert_int_equals(J_DOUBLE, init->vars[4].op.vm_type);
	assert_true(init->vars[4].op.is_fragment);
	assert_int_equals(DEFINED, final->vars[3].state);
	assert_int_equals(J_DOUBLE, final->vars[3].op.vm_type);
	assert_int_equals(DEFINED, final->vars[4].state);
	assert_int_equals(J_DOUBLE, final->vars[4].op.vm_type);
	assert_true(final->vars[4].op.is_fragment);

	/* store simple */
	store_vrf_lvar(b, J_INT, 0);
	assert_int_equals(DEFINED, final->vars[0].state);
	assert_int_equals(J_INT, final->vars[0].op.vm_type);

	/* store double */
	store_vrf_lvar(b, J_DOUBLE, 0);
	assert_int_equals(DEFINED, final->vars[0].state);
	assert_int_equals(J_DOUBLE, final->vars[0].op.vm_type);
	assert_int_equals(DEFINED, final->vars[1].state);
	assert_int_equals(J_DOUBLE, final->vars[1].op.vm_type);
	assert_true(final->vars[1].op.is_fragment);

	/* peek simple wrong type */
	store_vrf_lvar(b, J_INT, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_lvar(b, J_CHAR, 0));

	/* peek double wrong type */
	store_vrf_lvar(b, J_INT, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_lvar(b, J_FLOAT, 0));
	store_vrf_lvar(b, J_DOUBLE, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_lvar(b, J_FLOAT, 0));

	/* peek double fragment */

	store_vrf_lvar(b, J_DOUBLE, 0);
	store_vrf_lvar(b, J_DOUBLE, 1);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_lvar(b, J_DOUBLE, 0));

	store_vrf_lvar(b, J_DOUBLE, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_lvar(b, J_DOUBLE, 1));

	/* peek simple OK */
	store_vrf_lvar(b, J_INT, 0);
	assert_int_equals(0, peek_vrf_lvar(b, J_INT, 0));

	/* peek double OK */
	store_vrf_lvar(b, J_FLOAT, 0);
	assert_int_equals(0, peek_vrf_lvar(b, J_FLOAT, 0));

	free_verifier_block(b);
	free(chk);
}

void test_verifier_stack_utilities(void)
{
	struct verifier_context *chk;
	struct verifier_block *b;
	struct verifier_state *final, *init;
	struct verifier_stack *el;

	chk = malloc(sizeof(struct verifier_context));
	assert_not_null(chk);
	chk->max_locals = 1;

	/* allocation & initialization */
	b = alloc_verifier_block(chk, 0);
	assert_not_null(b);

	final = b->final_state;
	init = b->initial_state;

	/* pop bottom */
	assert_int_equals(0, pop_vrf_op(b, J_INT));
	el = list_first_entry(&final->stack->slots, struct verifier_stack, slots);
	assert_int_equals(VM_TYPE_MAX, el->op.vm_type);
	el = list_first_entry(&init->stack->slots, struct verifier_stack, slots);
	assert_int_equals(J_INT, el->op.vm_type);

	/* push simple */
	push_vrf_op(b, J_INT);
	el = list_first_entry(&final->stack->slots, struct verifier_stack, slots);
	assert_not_null(el);
	assert_int_equals(el->op.vm_type, J_INT);
	assert_false(el->op.is_fragment);

	/* push double */
	push_vrf_op(b, J_DOUBLE);
	el = list_first_entry(&final->stack->slots, struct verifier_stack, slots);
	assert_not_null(el);
	assert_int_equals(el->op.vm_type, J_DOUBLE);
	assert_true(el->op.is_fragment);
	el = list_entry(el->slots.next, struct verifier_stack, slots);
	assert_not_null(el);
	assert_int_equals(el->op.vm_type, J_DOUBLE);
	assert_false(el->op.is_fragment);

	/* peek simple wrong */
	push_vrf_op(b, J_INT);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_op(b, J_CHAR));

	/* peek double wrong */

	push_vrf_op(b, J_FLOAT);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_op(b, J_DOUBLE));

	push_vrf_op(b, J_INT);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_op(b, J_DOUBLE));

	/* peek simple OK */
	push_vrf_op(b, J_INT);
	assert_int_equals(0, peek_vrf_op(b, J_INT));

	/* peek double OK */
	push_vrf_op(b, J_DOUBLE);
	assert_int_equals(0, peek_vrf_op(b, J_DOUBLE));

	/* pop simple wrong */
	push_vrf_op(b, J_INT);
	assert_int_equals(E_TYPE_CHECKING, pop_vrf_op(b, J_CHAR));

	/* pop double wrong */

	push_vrf_op(b, J_FLOAT);
	assert_int_equals(E_TYPE_CHECKING, pop_vrf_op(b, J_DOUBLE));

	push_vrf_op(b, J_INT);
	assert_int_equals(E_TYPE_CHECKING, pop_vrf_op(b, J_DOUBLE));

	/* pop simple OK */
	push_vrf_op(b, J_CHAR);
	push_vrf_op(b, J_INT);
	assert_int_equals(0, pop_vrf_op(b, J_INT));
	assert_int_equals(0, peek_vrf_op(b, J_CHAR));

	/* pop double OK */
	push_vrf_op(b, J_FLOAT);
	push_vrf_op(b, J_DOUBLE);
	assert_int_equals(0, pop_vrf_op(b, J_DOUBLE));
	assert_int_equals(0, peek_vrf_op(b, J_FLOAT));

	free_verifier_block(b);
	free(chk);
}

void test_jump_destinations_utilities(void)
{
	unsigned long prev_dest;
	struct verifier_jump_destinations *jd, *cur;
	struct bitset *insn_map;

	jd = alloc_verifier_jump_destinations(0);
	assert_not_null(jd);
	INIT_LIST_HEAD(&jd->list);

	insn_map = alloc_bitset(16);
	assert_not_null(insn_map);

	assert_int_equals(0, add_jump_destination(jd, 2));
	assert_int_equals(0, add_jump_destination(jd, 15));
	assert_int_equals(0, add_jump_destination(jd, 1));

	prev_dest = 0;
	list_for_each_entry(cur, &jd->list, list) {
		assert_true((prev_dest < cur->dest));
		prev_dest = cur->dest;
	}

	set_bit(insn_map->bits, 0);
	set_bit(insn_map->bits, 1);
	set_bit(insn_map->bits, 2);
	set_bit(insn_map->bits, 15);

	list_for_each_entry(cur, &jd->list, list)
		assert_true(test_bit(insn_map->bits, cur->dest));

	free_verifier_jump_destinations(jd);
	free(insn_map);
}

void test_transition_verifier_stack(void)
{
	struct verifier_context *chk;
	struct verifier_block *curb, *nextb;
	struct verifier_state *stc;
	struct verifier_state *stn;

	chk = malloc(sizeof(struct verifier_context));
	assert_not_null(chk);
	chk->max_locals = 0;

	curb = alloc_verifier_block(chk, 0);
	nextb = alloc_verifier_block(chk, 0);
	assert_not_null(curb);
	assert_not_null(nextb);

	stc = curb->final_state;
	stn = nextb->initial_state;

	/* First we test with identical stacks. */

	push_vrf_op(curb, J_INT);
	push_vrf_op(curb, J_INT);
	push_vrf_op(curb, J_DOUBLE);

	pop_vrf_op(nextb, J_DOUBLE);
	pop_vrf_op(nextb, J_INT);
	pop_vrf_op(nextb, J_INT);

	assert_int_equals(0, transition_verifier_stack(stc->stack, stn->stack));

	free_verifier_state(stc);

	/* Then with the initial stack larger than the next stack, which
	 * should be OK too.
	 */

	curb->final_state = alloc_verifier_state(0);
	stc = curb->final_state;
	assert_not_null(stc);

	push_vrf_op(curb, J_FLOAT);
	push_vrf_op(curb, J_INT);
	push_vrf_op(curb, J_INT);
	push_vrf_op(curb, J_DOUBLE);

	assert_int_equals(0, transition_verifier_stack(stc->stack, stn->stack));

	/* But it shouldn't be OK for instance if the the stack were different !
	 */

	pop_vrf_op(nextb, J_DOUBLE);

	assert_int_equals(E_TYPE_CHECKING, transition_verifier_stack(stc->stack, stn->stack));

	free_verifier_block(curb);
	free_verifier_block(nextb);
	free(chk);
}

void test_transition_verifier_local_var(void)
{
	struct verifier_context *chk;
	struct verifier_block *curb, *nextb;
	struct verifier_state *stc;
	struct verifier_state *stn;

	chk = malloc(sizeof(struct verifier_context));
	assert_not_null(chk);
	chk->max_locals = 7;

	curb = alloc_verifier_block(chk, 0);
	nextb = alloc_verifier_block(chk, 0);
	assert_not_null(curb);
	assert_not_null(nextb);

	stc = curb->final_state;
	stn = nextb->initial_state;

	/* First test with identical variables state. */

	store_vrf_lvar(curb, J_INT, 0);
	store_vrf_lvar(curb, J_DOUBLE, 1);

	peek_vrf_lvar(nextb, J_INT, 0);
	peek_vrf_lvar(nextb, J_DOUBLE, 1);

	assert_int_equals(0, transition_verifier_local_var(stc->vars, stn->vars, stc->nb_vars));

	/* Then we define a variable in the initial state that is unknown
	 * in the next state, which should still be OK.
	 */

	peek_vrf_lvar(curb, J_FLOAT, 4);

	assert_int_equals(0, transition_verifier_local_var(stc->vars, stn->vars, stc->nb_vars));

	/* Finally we ensure the transition is invalid when variables types
	 * do not match.
	 */

	store_vrf_lvar(curb, J_INT, 6);
	peek_vrf_lvar(nextb, J_CHAR, 6);

	assert_int_equals(E_TYPE_CHECKING, transition_verifier_local_var(stc->vars, stn->vars, stc->nb_vars));

	free_verifier_block(curb);
	free_verifier_block(nextb);
	free(chk);
}
