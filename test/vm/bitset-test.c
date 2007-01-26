/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <vm/bitset.h>
#include <stdlib.h>
#include <libharness.h>

void test_bitset_is_initially_zeroed(void)
{
	struct bitset *bitset;
	int i;

	bitset = alloc_bitset(8);
	for (i = 0; i < 8; i++)
		assert_int_equals(0, test_bit(bitset->bits, i));
		
	free(bitset);
}

void test_bitset_retains_set_bit(void)
{
	struct bitset *bitset;
	
	bitset = alloc_bitset(8);

	set_bit(bitset->bits, 0);
	assert_int_equals(1, test_bit(bitset->bits, 0));
	assert_int_equals(0, test_bit(bitset->bits, 1));

	set_bit(bitset->bits, 7);
	assert_int_equals(1, test_bit(bitset->bits, 7));
	assert_int_equals(0, test_bit(bitset->bits, 6));

	free(bitset);
}
