/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <vm/bitset.h>
#include <stdlib.h>
#include <libharness.h>

#define BITSET_SIZE 8

void test_bitset_is_initially_zeroed(void)
{
	struct bitset *bitset;
	int i;

	bitset = alloc_bitset(BITSET_SIZE);
	for (i = 0; i < BITSET_SIZE; i++)
		assert_int_equals(0, test_bit(bitset->bits, i));
		
	free(bitset);
}

void test_bitset_retains_set_bit(void)
{
	struct bitset *bitset;
	
	bitset = alloc_bitset(BITSET_SIZE);

	set_bit(bitset->bits, 0);
	assert_int_equals(1, test_bit(bitset->bits, 0));
	assert_int_equals(0, test_bit(bitset->bits, 1));

	set_bit(bitset->bits, BITSET_SIZE-1);
	assert_int_equals(1, test_bit(bitset->bits, BITSET_SIZE-1));
	assert_int_equals(0, test_bit(bitset->bits, BITSET_SIZE-2));

	free(bitset);
}

void test_bitset_union(void)
{
	struct bitset *bitset, *all_set;
	int i;

	bitset = alloc_bitset(BITSET_SIZE);
	all_set = alloc_bitset(BITSET_SIZE);

	for (i = 0; i < BITSET_SIZE/2; i++)
		set_bit(bitset->bits, i);

	for (i = 0; i < BITSET_SIZE; i++)
		set_bit(all_set->bits, i);

	bitset_union_to(all_set, bitset);

	for (i = 0; i < BITSET_SIZE/2; i++)
		assert_int_equals(1, test_bit(bitset->bits, i));

	free(bitset);
	free(all_set);
}

void test_bitset_copy_to(void)
{
	struct bitset *src, *target;
	int i;

	src = alloc_bitset(BITSET_SIZE);
	target = alloc_bitset(BITSET_SIZE);

	for (i = 0; i < BITSET_SIZE; i += 2)
		set_bit(src->bits, i);

	bitset_copy_to(src, target);

	for (i = 0; i < BITSET_SIZE; i++)
		assert_int_equals(test_bit(src->bits, i), test_bit(target->bits, i));

	free(src);
	free(target);
}

void test_bitset_sub(void)
{
	struct bitset *half_set, *bitset;
	int i;

	half_set = alloc_bitset(BITSET_SIZE);
	bitset = alloc_bitset(BITSET_SIZE);

	for (i = 0; i < BITSET_SIZE/2; i++)
		set_bit(half_set->bits, i);

	for (i = 0; i < BITSET_SIZE; i++)
		set_bit(bitset->bits, i);

	bitset_sub(half_set, bitset);

	for (i = 0; i < BITSET_SIZE/2; i++)
		assert_int_equals(0, test_bit(bitset->bits, i));

	for (i = BITSET_SIZE/2; i < BITSET_SIZE; i++)
		assert_int_equals(1, test_bit(bitset->bits, i));

	free(bitset);
	free(half_set);
}

void test_bitset_equal(void)
{
	struct bitset *half_set, *ones, *zeros;
	int i;

	half_set = alloc_bitset(BITSET_SIZE);
	ones = alloc_bitset(BITSET_SIZE);
	zeros = alloc_bitset(BITSET_SIZE);

	for (i = 0; i < BITSET_SIZE/2; i++)
		set_bit(half_set->bits, i);

	for (i = 0; i < BITSET_SIZE; i++)
		set_bit(ones->bits, i);

	assert_int_equals(0, bitset_equal(half_set, ones));
	assert_int_equals(1, bitset_equal(half_set, half_set));
	assert_int_equals(1, bitset_equal(ones, ones));
	assert_int_equals(1, bitset_equal(zeros, zeros));

	free(zeros);
	free(ones);
	free(half_set);
}

void test_bitset_clear_all(void)
{
	struct bitset *bitset;
	int i;

	bitset = alloc_bitset(BITSET_SIZE);

	for (i = 0; i < BITSET_SIZE; i++)
		set_bit(bitset->bits, i);

	bitset_clear_all(bitset);

	for (i = 0; i < BITSET_SIZE; i++)
		assert_int_equals(0, test_bit(bitset->bits, i));

	free(bitset);
}

void test_bitset_set_all(void)
{
	struct bitset *bitset;
	int i;

	bitset = alloc_bitset(BITSET_SIZE);

	bitset_set_all(bitset);

	for (i = 0; i < BITSET_SIZE; i++)
		assert_int_equals(1, test_bit(bitset->bits, i));

	free(bitset);
}

void test_bitset_ffs_all_zeros(void)
{
	struct bitset *bitset;

	bitset = alloc_bitset(BITSET_SIZE);

	assert_true(bitset_ffs(bitset) < 0);

	free(bitset);
}

void test_bitset_ffs_some_set(void)
{
	struct bitset *bitset;

	bitset = alloc_bitset(BITSET_SIZE);
	set_bit(bitset->bits, 0);
	set_bit(bitset->bits, 1);
	set_bit(bitset->bits, 3);

	assert_int_equals(0, bitset_ffs(bitset));
	clear_bit(bitset->bits, 0);

	assert_int_equals(1, bitset_ffs(bitset));
	clear_bit(bitset->bits, 1);

	assert_int_equals(3, bitset_ffs(bitset));
	clear_bit(bitset->bits, 3);

	assert_true(bitset_ffs(bitset) < 0);

	free(bitset);
}
