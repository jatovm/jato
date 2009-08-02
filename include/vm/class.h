#ifndef _VM_CLASS_H
#define _VM_CLASS_H

#include <assert.h>
#include <pthread.h>

#include "vm/field.h"
#include "vm/itable.h"
#include "vm/method.h"
#include "vm/static.h"
#include "vm/types.h"
#include "vm/vm.h"

#include "jit/vtable.h"

struct vm_object;

enum vm_class_state {
	VM_CLASS_LOADED,
	VM_CLASS_LINKED,
	VM_CLASS_INITIALIZING,
	VM_CLASS_ERRONEOUS,
	VM_CLASS_INITIALIZED,
};

enum vm_class_kind {
	VM_CLASS_KIND_PRIMITIVE,
	VM_CLASS_KIND_ARRAY,
	VM_CLASS_KIND_REGULAR
};

struct vm_class {
	enum vm_class_kind kind;
	const struct cafebabe_class *class;
	enum vm_class_state state;
	char *name;

	pthread_mutex_t mutex;

	struct vm_class *super;
	unsigned int nr_interfaces;
	struct vm_class **interfaces;
	struct vm_field *fields;
	struct vm_method *methods;

	unsigned int object_size;
	unsigned int static_size;

	unsigned int vtable_size;
	struct vtable vtable;

	/* The java.lang.Class object representing this class */
	struct vm_object *object;

	/* This is an array of all the values of the static members of this
	 * class. */
	uint8_t *static_values;

	struct list_head static_fixup_site_list;

	const char *source_file_name;

	union {
		/* For primitve type classes this holds a vm_type
		   represented by this class. */
		enum vm_type primitive_vm_type;

		/* For array classes this points to array element's class */
		struct vm_class *array_element_class;
	};

	void *itable[VM_ITABLE_SIZE];
};

int vm_class_link(struct vm_class *vmc, const struct cafebabe_class *class);
int vm_class_link_primitive_class(struct vm_class *vmc, const char *class_name);
int vm_class_link_array_class(struct vm_class *vmc, const char *class_name);
int vm_class_init(struct vm_class *vmc);

static inline int vm_class_ensure_init(struct vm_class *vmc)
{
	if (vmc->state == VM_CLASS_INITIALIZED)
		return 0;

	return vm_class_init(vmc);
}

static inline bool vm_class_is_interface(const struct vm_class *vmc)
{
	return vmc->class->access_flags & CAFEBABE_CLASS_ACC_INTERFACE;
}

static inline bool vm_class_is_abstract(const struct vm_class *vmc)
{
	return vmc->class->access_flags & CAFEBABE_CLASS_ACC_ABSTRACT;
}

static inline bool vm_class_is_array_class(const struct vm_class *vmc)
{
	return vmc->kind == VM_CLASS_KIND_ARRAY;
}

static inline bool vm_class_is_primitive_class(const struct vm_class *vmc)
{
	return vmc->kind == VM_CLASS_KIND_PRIMITIVE;
}

struct vm_class *vm_class_resolve_class(const struct vm_class *vmc, uint16_t i);

struct vm_field *vm_class_get_field(const struct vm_class *vmc,
	const char *name, const char *type);
struct vm_field *vm_class_get_field_recursive(const struct vm_class *vmc,
	const char *name, const char *type);

int vm_class_resolve_field(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type);
struct vm_field *vm_class_resolve_field_recursive(const struct vm_class *vmc,
	uint16_t i);

struct vm_method *vm_class_get_method(const struct vm_class *vmc,
	const char *name, const char *type);
struct vm_method *vm_class_get_method_recursive(const struct vm_class *vmc,
	const char *name, const char *type);

int vm_class_resolve_method(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type);
struct vm_method *vm_class_resolve_method_recursive(const struct vm_class *vmc,
	uint16_t i);

int vm_class_resolve_interface_method(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type);
struct vm_method *vm_class_resolve_interface_method_recursive(
	const struct vm_class *vmc, uint16_t i);

bool vm_class_is_assignable_from(const struct vm_class *vmc, const struct vm_class *from);
bool vm_class_is_primitive_type_name(const char *class_name);
char *vm_class_get_array_element_class_name(const char *class_name);
struct vm_class *vm_class_get_array_element_class(const struct vm_class *array_class);
enum vm_type vm_class_get_storage_vmtype(const struct vm_class *class);
struct vm_class *vm_class_get_class_from_class_object(struct vm_object *clazz);

static inline void
vm_field_set_int32(const struct vm_field *field, int32_t value)
{
	assert(vm_field_is_static(field));

	*(int32_t *) &field->class->static_values[field->offset] = value;
}

static inline int32_t
vm_field_get_int32(const struct vm_field *field)
{
	assert(vm_field_is_static(field));

	return *(int32_t *) &field->class->static_values[field->offset];
}

static inline void
vm_field_set_int64(const struct vm_field *field, int64_t value)
{
	assert(vm_field_is_static(field));

	*(int64_t *) &field->class->static_values[field->offset] = value;
}

static inline int64_t
vm_field_get_int64(const struct vm_field *field)
{
	assert(vm_field_is_static(field));

	return *(int64_t *) &field->class->static_values[field->offset];
}

static inline void
vm_field_set_float(const struct vm_field *field, float value)
{
	assert(vm_field_is_static(field));

	*(float *) &field->class->static_values[field->offset] = value;
}

static inline float
vm_field_get_float(const struct vm_field *field)
{
	assert(vm_field_is_static(field));

	return *(float *) &field->class->static_values[field->offset];
}

static inline void
vm_field_set_object(const struct vm_field *field, struct vm_object *value)
{
	assert(vm_field_is_static(field));

	*(void **) &field->class->static_values[field->offset] = value;
}

static inline struct vm_object *
vm_field_get_object(const struct vm_field *field)
{
	assert(vm_field_is_static(field));

	return *(struct vm_object **)
		&field->class->static_values[field->offset];
}

#endif /* __CLASS_H */
