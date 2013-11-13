/*
 * Copyright (c) 2009  Pekka Enberg
 * 
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
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
