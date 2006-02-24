/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <bitmap.h>
#include <system.h>
#include <stdlib.h>
#include <string.h>

unsigned long *alloc_bitmap(unsigned long bits)
{
	unsigned long *bitmap, size;

	size = ALIGN(bits, BITS_PER_LONG) / 8;
	bitmap = malloc(size);
	if (bitmap)
		memset(bitmap, 0, size);
	return bitmap;
}

static inline unsigned long *addr_of(unsigned long *bitmap, unsigned long bit)
{
	return bitmap + (bit / BITS_PER_LONG);
}

int test_bit(unsigned long *bitmap, unsigned long bit)
{
	unsigned long *addr, mask;
	addr = addr_of(bitmap, bit);
	mask = 1UL << (bit & (BITS_PER_LONG-1));
	return ((*addr & mask) != 0);
}

void set_bit(unsigned long *bitmap, unsigned long bit)
{
	unsigned long *addr, mask;
	
	addr = addr_of(bitmap, bit); 
	mask = 1UL << (bit & (BITS_PER_LONG-1));

	*addr |= mask;
}
