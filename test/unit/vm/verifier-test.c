#include "vm/verifier.h"
#include "lib/list.h"

#include <libharness.h>
#include <stdlib.h>

void test_local_var_utilities(void)
{
	struct verifier_state *s;

	/* allocation & initialization */
	s = alloc_verifier_state(5);
	assert_not_null(s);
	assert_int_equals(UNKNOWN, s->vars[0].state);
	assert_false(s->vars[0].op.is_fragment);

	/* undefine */
	undef_verifier_local_var(s, 0);
	assert_int_equals(UNDEFINED, s->vars[0].state);

	/* store simple */
	store_verifier_local_var(s, J_INT, 0);
	assert_int_equals(DEFINED, s->vars[0].state);
	assert_int_equals(J_INT, s->vars[0].op.vm_type);

	/* store double */
	store_verifier_local_var(s, J_DOUBLE, 0);
	assert_int_equals(DEFINED, s->vars[0].state);
	assert_int_equals(J_DOUBLE, s->vars[0].op.vm_type);
	assert_int_equals(DEFINED, s->vars[1].state);
	assert_int_equals(J_DOUBLE, s->vars[1].op.vm_type);
	assert_true(s->vars[1].op.is_fragment);

	/* peek simple unknown */
	assert_int_equals(0, peek_verifier_local_var_type(s, J_INT, 2));
	assert_int_equals(DEFINED, s->vars[2].state);
	assert_int_equals(J_INT, s->vars[2].op.vm_type);

	/* peek double unknown */
	assert_int_equals(0, peek_verifier_local_var_type(s, J_DOUBLE, 3));
	assert_int_equals(DEFINED, s->vars[3].state);
	assert_int_equals(J_DOUBLE, s->vars[3].op.vm_type);
	assert_int_equals(DEFINED, s->vars[4].state);
	assert_int_equals(J_DOUBLE, s->vars[4].op.vm_type);
	assert_true(s->vars[4].op.is_fragment);

	/* peek simple undef */
	undef_verifier_local_var(s, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_INT, 0));

	/* peek double undef */

	undef_verifier_local_var(s, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_DOUBLE, 0));

	undef_verifier_local_var(s, 1);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_DOUBLE, 0));

	store_verifier_local_var(s, J_DOUBLE, 0);
	undef_verifier_local_var(s, 1);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_DOUBLE, 0));

	/* peek simple wrong type */
	store_verifier_local_var(s, J_INT, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_CHAR, 0));

	/* peek double wrong type */
	store_verifier_local_var(s, J_INT, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_FLOAT, 0));
	store_verifier_local_var(s, J_DOUBLE, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_FLOAT, 0));

	/* peek double fragment */

	store_verifier_local_var(s, J_DOUBLE, 0);
	store_verifier_local_var(s, J_DOUBLE, 1);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_DOUBLE, 0));

	store_verifier_local_var(s, J_DOUBLE, 0);
	assert_int_equals(E_TYPE_CHECKING, peek_verifier_local_var_type(s, J_DOUBLE, 1));

	/* peek simple OK */
	store_verifier_local_var(s, J_INT, 0);
	assert_int_equals(0, peek_verifier_local_var_type(s, J_INT, 0));

	/* peek double OK */
	store_verifier_local_var(s, J_FLOAT, 0);
	assert_int_equals(0, peek_verifier_local_var_type(s, J_FLOAT, 0));

	free_verifier_state(s);
}

void test_verifier_stack_utilities(void)
{
	struct verifier_state *s;
	struct verifier_stack *el;

	s = alloc_verifier_state(0);

	/* allocation & initialization */
	assert_not_null(s);

	/* pop bottom */
	assert_int_equals(0, pop_vrf_op(s, J_INT));

	/* push simple */
	push_vrf_op(s, J_INT);
	el = list_first_entry(&s->stack->slots, struct verifier_stack, slots);
	assert_not_null(el);
	assert_int_equals(el->op.vm_type, J_INT);
	assert_false(el->op.is_fragment);

	/* push double */
	push_vrf_op(s, J_DOUBLE);
	el = list_first_entry(&s->stack->slots, struct verifier_stack, slots);
	assert_not_null(el);
	assert_int_equals(el->op.vm_type, J_DOUBLE);
	assert_true(el->op.is_fragment);
	el = list_entry(el->slots.next, struct verifier_stack, slots);
	assert_not_null(el);
	assert_int_equals(el->op.vm_type, J_DOUBLE);
	assert_false(el->op.is_fragment);

	/* peek simple wrong */
	push_vrf_op(s, J_INT);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_op(s, J_CHAR));

	/* peek double wrong */

	push_vrf_op(s, J_FLOAT);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_op(s, J_DOUBLE));

	push_vrf_op(s, J_INT);
	assert_int_equals(E_TYPE_CHECKING, peek_vrf_op(s, J_DOUBLE));

	/* peek simple OK */
	push_vrf_op(s, J_INT);
	assert_int_equals(0, peek_vrf_op(s, J_INT));

	/* peek double OK */
	push_vrf_op(s, J_DOUBLE);
	assert_int_equals(0, peek_vrf_op(s, J_DOUBLE));

	/* pop simple wrong */
	push_vrf_op(s, J_INT);
	assert_int_equals(E_TYPE_CHECKING, pop_vrf_op(s, J_CHAR));

	/* pop double wrong */

	push_vrf_op(s, J_FLOAT);
	assert_int_equals(E_TYPE_CHECKING, pop_vrf_op(s, J_DOUBLE));

	push_vrf_op(s, J_INT);
	assert_int_equals(E_TYPE_CHECKING, pop_vrf_op(s, J_DOUBLE));

	/* pop simple OK */
	push_vrf_op(s, J_CHAR);
	push_vrf_op(s, J_INT);
	assert_int_equals(0, pop_vrf_op(s, J_INT));
	assert_int_equals(0, peek_vrf_op(s, J_CHAR));

	/* pop double OK */
	push_vrf_op(s, J_FLOAT);
	push_vrf_op(s, J_DOUBLE);
	assert_int_equals(0, pop_vrf_op(s, J_DOUBLE));
	assert_int_equals(0, peek_vrf_op(s, J_FLOAT));

	free_verifier_state(s);
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
	struct verifier_state *stc;
	struct verifier_state *stn;

	stc = alloc_verifier_state(0);
	stn = alloc_verifier_state(0);
	assert_not_null(stc);
	assert_not_null(stn);

	/* First we test with identical stacks. */

	push_vrf_op(stc, J_INT);
	push_vrf_op(stc, J_INT);
	push_vrf_op(stc, J_DOUBLE);

	push_vrf_op(stn, J_INT);
	push_vrf_op(stn, J_INT);
	push_vrf_op(stn, J_DOUBLE);

	assert_int_equals(0, transition_verifier_stack(stc->stack, stn->stack));

	free_verifier_state(stc);

	/* Then with the initial stack larger than the next stack, which
	 * should be OK too.
	 */

	stc = alloc_verifier_state(0);

	push_vrf_op(stc, J_FLOAT);
	push_vrf_op(stc, J_FLOAT);
	push_vrf_op(stc, J_INT);
	push_vrf_op(stc, J_INT);
	push_vrf_op(stc, J_DOUBLE);

	assert_int_equals(0, transition_verifier_stack(stc->stack, stn->stack));

	/* But it shouldn't be OK for instance if the top of the stack were
	 * different !
	 */

	pop_vrf_op(stc, J_DOUBLE);

	assert_int_equals(E_TYPE_CHECKING, transition_verifier_stack(stc->stack, stn->stack));

	free_verifier_state(stc);
	free_verifier_state(stn);
}

void test_transition_verifier_local_var(void)
{
	struct verifier_state *stc;
	struct verifier_state *stn;

	stc = alloc_verifier_state(6);
	stn = alloc_verifier_state(6);
	assert_not_null(stc);
	assert_not_null(stn);

	/* First test with identical variables state. */

	store_verifier_local_var(stc, J_INT, 0);
	store_verifier_local_var(stc, J_DOUBLE, 1);
	undef_verifier_local_var(stc, 3);

	store_verifier_local_var(stn, J_INT, 0);
	store_verifier_local_var(stn, J_DOUBLE, 1);
	undef_verifier_local_var(stn, 3);

	assert_int_equals(0, transition_verifier_local_var(stc->vars, stn->vars, stc->nb_vars));

	/* Then we define a variable in the initial state that is unknown
	 * in the next state, which should still be OK.
	 */

	store_verifier_local_var(stc, J_FLOAT, 4);

	assert_int_equals(0, transition_verifier_local_var(stc->vars, stn->vars, stc->nb_vars));

	/* Finally we ensure the transition is invalid when variables types
	 * do not match.
	 */

	store_verifier_local_var(stc, J_CHAR, 0);

	assert_int_equals(E_TYPE_CHECKING, transition_verifier_local_var(stc->vars, stn->vars, stc->nb_vars));

	free_verifier_state(stc);
	free_verifier_state(stn);
}
