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
