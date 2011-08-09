/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "lib/bitset.h"
#include "vm/system.h"
#include "vm/stdlib.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_PER_LONG sizeof(unsigned long)
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

	size = ALIGN(nr_bits, BITS_PER_LONG) / BITS_PER_BYTE;
	bitset = zalloc(sizeof(struct bitset) + size);
	if (bitset) {
		bitset->nr_bits = nr_bits;
		bitset->size = size;
	}
	return bitset;
}

void set_bit(unsigned long *bitset, unsigned long bit)
{
	unsigned long *addr, mask;
	
	addr = bitset + (bit / BITS_PER_LONG);
	mask = bit_mask(bit);

	*addr |= mask;
}

void clear_bit(unsigned long *bitset, unsigned long bit)
{
	unsigned long *addr, mask;

	addr = bitset + (bit / BITS_PER_LONG);
	mask = bit_mask(bit);

	*addr &= ~mask;
}

void bitset_union_to(struct bitset *from, struct bitset *to)
{
	unsigned long *src, *dest;
	unsigned long i, size;

	dest = to->bits;
	src = from->bits;
	size = max(from->size, to->size);

	for (i = 0; i < size / BYTES_PER_LONG; i++)
		dest[i] |= src[i];
}

void bitset_copy_to(struct bitset *from, struct bitset *to)
{
	unsigned long *src, *dest;
	unsigned long i, size;

	dest = to->bits;
	src = from->bits;
	size = max(from->size, to->size);

	for (i = 0; i < size / BYTES_PER_LONG; i++)
		dest[i] = src[i];
}

void bitset_sub(struct bitset *from, struct bitset *to)
{
	unsigned long *src, *dest;
	unsigned long i, size;

	dest = to->bits;
	src = from->bits;
	size = max(from->size, to->size);

	for (i = 0; i < size / BYTES_PER_LONG; i++)
		dest[i] &= ~src[i];
}

bool bitset_equal(struct bitset *from, struct bitset *to)
{
	unsigned long *src, *dest;
	unsigned long i, size;

	dest = to->bits;
	src = from->bits;
	size = max(from->size, to->size);

	for (i = 0; i < size / BYTES_PER_LONG; i++) {
		if (dest[i] != src[i])
			return false;
	}
	return true;
}

void bitset_clear_all(struct bitset *bitset)
{
	memset(bitset->bits, 0, bitset->size);
}

void bitset_set_all(struct bitset *bitset)
{
	memset(bitset->bits, 0xff, bitset->size);
}

/**
 *      bitset_ffs_from - find first set bit starting from an index
 *      ndx - index from which to start looking
 *      @bitset:        the bitset to search
 *
 *      This function returns the index of the first set bit starting
 *      from a given index. If no bits are set, returns a negative
 *      integer.
 */
int bitset_ffs_from(struct bitset *bitset, int ndx)
{
	unsigned long unaligned;
	unsigned long *bits;
	unsigned long start;
	unsigned long mask;
	unsigned long tmp;
	unsigned long ret;

	unaligned	= ndx % BITS_PER_LONG;
	start		= ndx & ~(BITS_PER_LONG - 1);

	bits = bitset->bits + start / BITS_PER_LONG;

	mask = (1UL << unaligned) - 1;

	tmp = *bits & ~mask;
	if (tmp)
		goto found;

	start += BITS_PER_LONG;
	bits++;

	for (; start < bitset->nr_bits; start += BITS_PER_LONG) {
		tmp = *bits;
		if (tmp)
			goto found;

		bits++;
	}

	return -1;
found:
	ret = ffsl(tmp);
	ret += start - 1;
	if (ret < bitset->nr_bits)
		return ret;

	return -1;
}
