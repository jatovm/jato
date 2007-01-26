#ifndef __BITSET_H
#define __BITSET_H

struct bitset {
	unsigned long nr_bits;
	unsigned long bits[];
};

struct bitset *alloc_bitset(unsigned long);
int test_bit(unsigned long *, unsigned long);
void set_bit(unsigned long *, unsigned long);
void bitset_union_to(struct bitset *, struct bitset *);

#endif
