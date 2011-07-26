#ifndef JATO__ARM_CONSTANT_POOL_H
#define JATO__ARM_CONSTANT_POOL_H

/* An entry of constant pool */

struct compilation_unit;

struct lp_entry {
	struct lp_entry	*next;
	unsigned long	index;	/* Index starting from zero */
	unsigned long	value;	/* Immediate value in pool */
};

/*
 * These functions are used by constant literal pool implementation
 */
struct lp_entry *alloc_literal_pool_entry(struct compilation_unit *, unsigned long);
struct lp_entry *search_literal_pool(struct lp_entry *, unsigned long);
void free_constant_pool(struct lp_entry *);

#endif /* JATO__ARM_CONSTANT_POOL_H */
