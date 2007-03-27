/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/bitset.h>
#include <vm/system.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BITS_PER_BYTE 8

/**
 *	bitset_alloc - Allocate a new bit set
 *	@size: Number of elements in the set.
 *
 *	This function creates a bit set that is large enough to represent
 *	bits with indices zero to @size-1.
 */
struct bitset *alloc_bitset(unsigned long nr_bits)
{
	struct bitset *bitset;
	unsigned long size;

	nr_bits = ALIGN(nr_bits, BITS_PER_LONG);
	size = sizeof(struct bitset) + (nr_bits / BITS_PER_BYTE);
	bitset = malloc(size);
	if (bitset) {
		memset(bitset, 0, size);
		bitset->nr_bits = nr_bits;
	}
	return bitset;
}

static inline unsigned long bit_mask(unsigned long bit)
{
	return 1UL << (bit & (BITS_PER_LONG-1));
}

int test_bit(unsigned long *bitset, unsigned long bit)
{
	unsigned long *addr, mask;

	addr = bitset + (bit / BITS_PER_LONG);
	mask = bit_mask(bit);

	return ((*addr & mask) != 0);
}

void set_bit(unsigned long *bitset, unsigned long bit)
{
	unsigned long *addr, mask;
	
	addr = bitset + (bit / BITS_PER_LONG);
	mask = bit_mask(bit);

	*addr |= mask;
}

void bitset_union_to(struct bitset *from, struct bitset *to)
{
	unsigned long *src, *dest;
	unsigned long i, nr_bits;

	dest = to->bits;
	src = from->bits;
	nr_bits = max(from->nr_bits, to->nr_bits);

	for (i = 0; i < nr_bits / BITS_PER_LONG; i++)
		dest[i] |= src[i];
}

void bitset_copy_to(struct bitset *from, struct bitset *to)
{
	unsigned long *src, *dest;
	unsigned long i, nr_bits;

	dest = to->bits;
	src = from->bits;
	nr_bits = max(from->nr_bits, to->nr_bits);

	for (i = 0; i < nr_bits / BITS_PER_LONG; i++)
		dest[i] = src[i];
}

void bitset_sub(struct bitset *from, struct bitset *to)
{
	unsigned long *src, *dest;
	unsigned long i, nr_bits;

	dest = to->bits;
	src = from->bits;
	nr_bits = max(from->nr_bits, to->nr_bits);

	for (i = 0; i < nr_bits / BITS_PER_LONG; i++)
		dest[i] &= ~src[i];
}

bool bitset_equal(struct bitset *from, struct bitset *to)
{
	unsigned long *src, *dest;
	unsigned long i, nr_bits;

	dest = to->bits;
	src = from->bits;
	nr_bits = max(from->nr_bits, to->nr_bits);

	for (i = 0; i < nr_bits / BITS_PER_LONG; i++) {
		if (dest[i] != src[i])
			return false;
	}
	return true;
}
