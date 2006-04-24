/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <vm/bitmap.h>
#include <stdlib.h>
#include <libharness.h>

void test_bitmap_is_initially_zeroed(void)
{
	int i;
	unsigned long *bitmap = alloc_bitmap(8);

	for (i = 0; i < 8; i++)
		assert_int_equals(0, test_bit(bitmap, i));
		
	free(bitmap);
}

void test_bitmap_retains_set_bit(void)
{
	unsigned long *bitmap = alloc_bitmap(8);

	set_bit(bitmap, 0);
	assert_int_equals(1, test_bit(bitmap, 0));
	assert_int_equals(0, test_bit(bitmap, 1));

	set_bit(bitmap, 7);
	assert_int_equals(1, test_bit(bitmap, 7));
	assert_int_equals(0, test_bit(bitmap, 6));

	free(bitmap);
}
