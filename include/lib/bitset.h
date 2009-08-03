#ifndef LIB_BITSET_H
#define LIB_BITSET_H

#include <stdbool.h>

#include "vm/system.h"

struct bitset {
	unsigned long nr_bits;
	unsigned long size;
	unsigned long bits[];
};

struct bitset *alloc_bitset(unsigned long);
void set_bit(unsigned long *, unsigned long);
void clear_bit(unsigned long *, unsigned long);
void bitset_union_to(struct bitset *, struct bitset *);
void bitset_sub(struct bitset *, struct bitset *);
void bitset_copy_to(struct bitset *, struct bitset *);
bool bitset_equal(struct bitset *, struct bitset *);
void bitset_clear_all(struct bitset *);
void bitset_set_all(struct bitset *);
int bitset_ffs(struct bitset *);

static inline unsigned long bit_mask(unsigned long bit)
{
	return 1UL << (bit & (BITS_PER_LONG-1));
}

static inline int test_bit(unsigned long *bitset, unsigned long bit)
{
	unsigned long *addr, mask;

	addr = bitset + (bit / BITS_PER_LONG);
	mask = bit_mask(bit);

	return ((*addr & mask) != 0);
}

#endif
