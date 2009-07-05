/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include "vm/list.h"

void test_list_retains_elements(void)
{
	struct list_head list = LIST_HEAD_INIT(list);
	struct list_head entry = LIST_HEAD_INIT(list);
	
	assert_true(list_is_empty(&list));
	list_add(&entry, &list);
	assert_false(list_is_empty(&list));
}

static void assert_list_head(struct list_head *prev,
			     struct list_head *next,
			     struct list_head *head)
{
	assert_ptr_equals(prev, head->prev);
	assert_ptr_equals(next, head->next);
}

void test_add_updates_links(void)
{
	struct list_head list = LIST_HEAD_INIT(list);
	struct list_head entry1 = LIST_HEAD_INIT(list);
	struct list_head entry2 = LIST_HEAD_INIT(list);

	list_add(&entry2, &list);
	list_add(&entry1, &list);

	assert_list_head(&entry2, &entry1, &list);
	assert_list_head(&list, &entry2, &entry1);
	assert_list_head(&entry1, &list, &entry2);
}

void test_add_tail(void)
{
	struct list_head list = LIST_HEAD_INIT(list);
	struct list_head entry1 = LIST_HEAD_INIT(list);
	struct list_head entry2 = LIST_HEAD_INIT(list);

	list_add_tail(&entry1, &list);
	list_add_tail(&entry2, &list);

	assert_list_head(&entry2, &entry1, &list);
	assert_list_head(&list, &entry2, &entry1);
	assert_list_head(&entry1, &list, &entry2);
}

struct foo {
	int i;
	struct list_head node;
};

void test_list_entry(void)
{
	struct foo foo;
	INIT_LIST_HEAD(&foo.node);
	
	assert_ptr_equals(&foo, list_entry(&foo.node, struct foo, node));
}
