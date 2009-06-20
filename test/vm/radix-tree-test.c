/*
 * Copyright (C) 2009 Tomasz Grabiec
 */

#include <libharness.h>
#include <limits.h>
#include <vm/radix-tree.h>

void test_tree_insert_and_lookup(void)
{
	unsigned long key;
	struct radix_tree *tree;
	void *result;

	tree = alloc_radix_tree(2, sizeof(key)*8);

	key = 1;
	radix_tree_insert(tree, key, (void*)0xcafebabe);
	result = radix_tree_lookup(tree, key);
	assert_ptr_equals((void*)0xcafebabe, result);

	key = ULONG_MAX;
	radix_tree_insert(tree, key, (void*)0xdeadbeef);
	result = radix_tree_lookup(tree, key);
	assert_ptr_equals((void*)0xdeadbeef, result);

	key = key - 1;
	result = radix_tree_lookup_prev(tree, key);
	assert_ptr_equals((void*)0xcafebabe, result);

	free_radix_tree(tree);
}

void test_tree_remove()
{
	struct radix_tree *tree;
	unsigned long key;
	void *value;
	void *result;

	tree = alloc_radix_tree(2, sizeof(key)*8);

	key = 0xdeadbeef;
	value = (void*)0xcafebabe;

	radix_tree_insert(tree, key, value);
	result = radix_tree_lookup(tree, key);
	assert_ptr_equals(value, result);

	radix_tree_remove(tree, key);
	result = radix_tree_lookup(tree, key);
	assert_ptr_equals(NULL, result);

	free_radix_tree(tree);
}

void test_tree_lookup_previous_in_the_same_node(void)
{
	struct radix_tree *tree;
	void *result;

	tree = alloc_radix_tree(8, 16);

	radix_tree_insert(tree, 1, (void*)0xcafebabe);
	result = radix_tree_lookup_prev(tree, 100);
	assert_ptr_equals((void*)0xcafebabe, result);

	free_radix_tree(tree);
}
