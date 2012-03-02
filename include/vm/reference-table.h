#ifndef JATO_VM_REFERENCE_TABLE_H
#define JATO_VM_REFERENCE_TABLE_H

struct gc_reference_map {
	/* top of the list */
	vm_object** top;

	/* bottom of the list */
	vm_object** bottom;

	/* no. of entries we can accomodate */
	int left_entrie;

	/* maximum no. of entries */
	int max_entries;
};

int gc_init_reftable(struct gc_reference_map* ref_t, int init_count, int max_count);

void gc_clear_reftable(struct gc_reference_table* ref_t);

#define gc_reftable_entries(ref_t) ref_t->top - ref_t->bottom;
#define is_gc_reftable_full(ref_t) gc_reftable_entries(ref_t) == (size_t)ref_t->left_entries;
