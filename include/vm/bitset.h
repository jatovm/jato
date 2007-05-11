#ifndef __BITSET_H
#define __BITSET_H

#include <stdbool.h>

struct bitset {
	unsigned long nr_bits;
	unsigned long bits[];
};

struct bitset *alloc_bitset(unsigned long);
int test_bit(unsigned long *, unsigned long);
void set_bit(unsigned long *, unsigned long);
void bitset_union_to(struct bitset *, struct bitset *);
void bitset_sub(struct bitset *, struct bitset *);
void bitset_copy_to(struct bitset *, struct bitset *);
bool bitset_equal(struct bitset *, struct bitset *);
void bitset_clear_all(struct bitset *);
void bitset_set_all(struct bitset *);

#endif
