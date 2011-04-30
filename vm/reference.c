/*
 * Copyright (c) 2010 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "lib/hash-map.h"

#include "vm/reference.h"
#include "vm/call.h"
#include "vm/errors.h"
#include "vm/object.h"

/*
 * Maps object pointer to the list of all struct vm_reference
 * referencing that object.
 */
static struct hash_map *reference_map;

static inline struct vm_object *vm_reference_get_lock(void)
{
	vm_class_ensure_init(vm_java_lang_ref_Reference);
	return static_field_get_object(vm_java_lang_ref_Reference_lock);
}

static inline void vm_reference_lock(void)
{
	vm_object_lock(vm_reference_get_lock());
}

static inline void vm_reference_unlock(void)
{
	vm_object_unlock(vm_reference_get_lock());
}

void vm_reference_init(void)
{
	reference_map = alloc_hash_map(&pointer_key);
}

static void vm_reference_clear(struct vm_reference *ref)
{
	vm_reference_lock();

	if (ref->referent) {
		ref->referent = NULL;
		list_del(&ref->node);
	}

	vm_reference_unlock();
}

struct vm_reference *
vm_reference_alloc(struct vm_object *referent, enum vm_reference_type type)
{
	struct vm_reference *ref;

	if (type == VM_REFERENCE_STRONG)
		ref = vm_alloc(sizeof *ref);
	else
		ref = malloc(sizeof *ref);

	if (!ref)
		return throw_oom_error();

	ref->referent	= referent;
	ref->object	= NULL;
	ref->type	= type;
	INIT_LIST_HEAD(&ref->node);

	vm_reference_lock();

	struct list_head *ref_list;
	if (hash_map_get(reference_map, referent, (void **) &ref_list)) {
		ref_list = malloc(sizeof(struct list_head));
		if (!ref_list) {
			throw_oom_error();
			goto out_free_ref;
		}

		INIT_LIST_HEAD(ref_list);

		if (hash_map_put(reference_map, referent, ref_list)) {
			throw_oom_error();
			goto out_free_ref_list;
		}

		/* Force finalizer execution for this object */
		gc_register_finalizer(referent, vm_object_finalizer);
	}

	list_add(&ref->node, ref_list);
	vm_reference_unlock();
	return ref;

 out_free_ref_list:
	free(ref_list);
 out_free_ref:
	if (type == VM_REFERENCE_STRONG)
		vm_free(ref);
	else
		free(ref);

	vm_reference_unlock();
	return NULL;
}

void vm_reference_free(struct vm_reference *reference)
{
	vm_reference_clear(reference);

	if (reference->type == VM_REFERENCE_STRONG)
		vm_free(reference);
	else
		free(reference);
}

struct vm_object *vm_reference_get(const struct vm_reference *reference)
{
	struct vm_object *ref;

	vm_reference_lock();

	if (reference->type == VM_REFERENCE_PHANTOM)
		ref = NULL;
	else
		ref = reference->referent;

	vm_reference_unlock();

	return ref;
}

static struct vm_reference *vm_reference_from_object(struct vm_object *object)
{
	return (struct vm_reference *) field_get_object(object, vm_java_lang_ref_Reference_referent);
}

/*
 * Called when java.lang.ref.Reference is collected
 */
void vm_reference_collect(struct vm_object *object)
{
	struct vm_reference *ref;

	vm_reference_lock();
	ref = vm_reference_from_object(object);

	if (!ref) {
		vm_reference_unlock();
		return;
	}

	vm_reference_free(ref);

	/* We clear the .referent field because even if @object
	 * is beeing finalized now, it may have been resurected
	 * in vm_reference_collect_for_object(). */
	field_set_object(object, vm_java_lang_ref_Reference_referent, NULL);

	vm_reference_unlock();
}

/*
 * Called on finalization of @object to clear all weaker references
 * to that object.
 */
void vm_reference_collect_for_object(struct vm_object *object)
{
	vm_reference_lock();

	struct list_head *ref_list;
	if (hash_map_get(reference_map, object, (void **) &ref_list)) {
		vm_reference_unlock();
		return;
	}

	if (hash_map_remove(reference_map, object))
		error("hash_map_remove");

	struct vm_reference *this, *next;
	list_for_each_entry_safe(this, next, ref_list, node) {
		assert(this->type != VM_REFERENCE_STRONG);

		if (this->object == NULL) {
			vm_reference_clear(this);
			continue;
		}

		vm_call_method_this(vm_java_lang_ref_Reference_clear, this->object);
		exception_print_and_clear();

		/* This may resurect this->object is it was collected
		 * in this cycle. */
		vm_call_method_this(vm_java_lang_ref_Reference_enqueue, this->object);
		exception_print_and_clear();
	}

	vm_reference_unlock();
	free(ref_list);
}

static enum vm_reference_type vm_reference_type_for_object(struct vm_object *reference)
{
	if (vm_object_is_instance_of(reference, vm_java_lang_ref_WeakReference))
		return  VM_REFERENCE_WEAK;

	if (vm_object_is_instance_of(reference, vm_java_lang_ref_SoftReference))
		return VM_REFERENCE_SOFT;

	if (vm_object_is_instance_of(reference, vm_java_lang_ref_PhantomReference))
		return VM_REFERENCE_PHANTOM;

	return VM_REFERENCE_STRONG;
}

/*
 * Called by JIT for putfield on java.lang.ref.Reference.referent
 * Assume Reference.lock is held.
 */
void put_referent(struct vm_object *reference, struct vm_object *referent)
{
	struct vm_reference *ref = vm_reference_from_object(reference);

	if (ref)
		vm_reference_clear(ref);

	if (!referent)
		return;

	enum vm_reference_type type = vm_reference_type_for_object(reference);

	assert(type != VM_REFERENCE_STRONG);

	ref = vm_reference_alloc(referent, type);
	if (!ref) {
		throw_oom_error();
		return;
	}

	ref->object = reference;
	gc_register_finalizer(reference, vm_object_finalizer);
	field_set_object(reference, vm_java_lang_ref_Reference_referent, (struct vm_object *) ref);
}

/*
 * Called by JIT for getfield on java.lang.ref.Reference.referent
 * Assume Reference.lock is held.
 */
struct vm_object *get_referent(struct vm_object *reference)
{
	struct vm_reference *ref;

	ref = vm_reference_from_object(reference);
	if (!ref)
		return NULL;

	return vm_reference_get(ref);
}
