#ifndef __BITMAP_H
#define __BITMAP_H

unsigned long *alloc_bitmap(unsigned long);
int test_bit(unsigned long *, unsigned long);
void set_bit(unsigned long *, unsigned long);

#endif
