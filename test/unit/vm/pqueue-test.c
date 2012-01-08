/*
 * Copyright (c) 2009  Pekka Enberg
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
 */

#include <libharness.h>

#include "lib/pqueue.h"

struct pqueue *pqueue;

static void setup(void)
{
	pqueue = pqueue_alloc();
}

static void teardown(void)
{
	pqueue_free(pqueue);
}

void test_new_pqueue_is_empty(void)
{
	setup();

	assert_true(pqueue_is_empty(pqueue));

	teardown();
}

void test_pqueue_is_not_empty_after_insert(void)
{
	setup();

	pqueue_insert(pqueue, 0UL, (void *) 0UL);
	assert_false(pqueue_is_empty(pqueue));

	teardown();
}

void test_pqueue_is_empty_after_last_element_is_removed(void)
{
	setup();

	pqueue_insert(pqueue, 0UL, (void *) 0UL);
	pqueue_insert(pqueue, 1UL, (void *) 1UL);

	pqueue_remove_top(pqueue);
	pqueue_remove_top(pqueue);

	assert_true(pqueue_is_empty(pqueue));

	teardown();
}

void test_pqueue_returns_highest_priority_element_first(void)
{
	setup();

	pqueue_insert(pqueue, 2UL, (void *) 2UL);
	pqueue_insert(pqueue, 1UL, (void *) 1UL);
	pqueue_insert(pqueue, 0UL, (void *) 0UL);

	assert_ptr_equals((void *) 0UL, pqueue_remove_top(pqueue));
	assert_ptr_equals((void *) 1UL, pqueue_remove_top(pqueue));
	assert_ptr_equals((void *) 2UL, pqueue_remove_top(pqueue));

	teardown();
}
