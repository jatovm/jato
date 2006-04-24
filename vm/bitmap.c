/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/bitmap.h>
#include <vm/system.h>

#include <stdlib.h>
#include <string.h>

unsigned long *alloc_bitmap(unsigned long bits)
{
	unsigned long *bitmap, nr_bytes;

	nr_bytes = ALIGN(bits, BITS_PER_LONG) / 8;

	bitmap = malloc(nr_bytes);
	if (bitmap)
		memset(bitmap, 0, nr_bytes);

	return bitmap;
}

static inline unsigned long *addr_of(unsigned long *bitmap, unsigned long bit)
{
	return bitmap + (bit / BITS_PER_LONG);
}

static inline unsigned long bit_mask(unsigned long bit)
{
	return 1UL << (bit & (BITS_PER_LONG-1));
}

int test_bit(unsigned long *bitmap, unsigned long bit)
{
	unsigned long *addr, mask;

	addr = addr_of(bitmap, bit);
	mask = bit_mask(bit);

	return ((*addr & mask) != 0);
}

void set_bit(unsigned long *bitmap, unsigned long bit)
{
	unsigned long *addr, mask;
	
	addr = addr_of(bitmap, bit); 
	mask = bit_mask(bit);

	*addr |= mask;
}
