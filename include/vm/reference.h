#ifndef JATO_VM_REFERENCE_H
#define JATO_VM_REFERENCE_H

#include "vm/preload.h"
#include "vm/class.h"
#include "vm/object.h"

#include "lib/list.h"

enum vm_reference_type {
	VM_REFERENCE_STRONG,
	VM_REFERENCE_WEAK,
	VM_REFERENCE_SOFT,
	VM_REFERENCE_PHANTOM
};

struct vm_reference {
	enum vm_reference_type type;

	struct vm_object *referent;

	/* Points to java.lang.ref.Reference instance for this reference.
	 * Holds NULL when reference created from VM code. */
	struct vm_object *object;

	/* Node in list of references pointing to referent. */
	struct list_head node;
};

void vm_reference_collect_for_object(struct vm_object *object);
void vm_reference_collect(struct vm_object *object);
void vm_reference_init(void);

struct vm_reference *vm_reference_alloc(struct vm_object *referent,
					enum vm_reference_type type);
void vm_reference_free(struct vm_reference *reference);
struct vm_object *vm_reference_get(const struct vm_reference *reference);

/*
 * Called by JIT for putfield on java.lang.ref.Reference.referent
 */
void put_referent(struct vm_object *reference, struct vm_object *referent);

/*
 * Called by JIT for getfield on java.lang.ref.Reference.referent
 */
struct vm_object *get_referent(struct vm_object *reference);

static inline struct vm_reference *
vm_reference_alloc_strong(struct vm_object *referent)
{
	return vm_reference_alloc(referent, VM_REFERENCE_STRONG);
}

static inline struct vm_reference *
vm_reference_alloc_weak(struct vm_object *referent)
{
	return vm_reference_alloc(referent, VM_REFERENCE_WEAK);
}

#endif /* JATO_VM_REFERENCE_H */
