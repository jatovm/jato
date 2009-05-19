/*
 * Copyright (C) 2009 Tomasz Grabiec
 */

#include <libharness.h>
#include <limits.h>
#include <vm/radix-tree.h>

void test_tree_put_and_lookup()
{
	unsigned long key;
	struct radix_tree *tree;
	void *result;

	tree = alloc_radix_tree(2, sizeof(key)*8);

	key = 1;
	tree_put(tree, key, (void*)0xcafebabe);
	result = tree_lookup(tree, key);
	assert_ptr_equals((void*)0xcafebabe, result);

	key = ULONG_MAX;
	tree_put(tree, key, (void*)0xdeadbeef);
	result = tree_lookup(tree, key);
	assert_ptr_equals((void*)0xdeadbeef, result);

	key = key - 1;
	result = tree_lookup_preceeding(tree, key);
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

	tree_put(tree, key, value);
	result = tree_lookup(tree, key);
	assert_ptr_equals(value, result);

	tree_remove(tree, key);
	result = tree_lookup(tree, key);
	assert_ptr_equals(NULL, result);

	free_radix_tree(tree);
}
