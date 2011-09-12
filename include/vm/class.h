#ifndef _VM_CLASS_H
#define _VM_CLASS_H

#include <assert.h>
#include <pthread.h>

#include "vm/field.h"
#include "vm/itable.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/static.h"
#include "vm/types.h"
#include "vm/vm.h"

#include "jit/vtable.h"

#include "cafebabe/enclosing_method_attribute.h"

struct vm_object;
struct vm_thread;
struct vm_annotation;

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

/*
 * NOTE NOTE NOTE! You must update supertype_cache_test() if you change this
 * constant.
 */
#define SUPERTYPE_CACHE_SIZE           2

struct vm_class {
	enum vm_class_kind kind;
	const struct cafebabe_class *class;
	enum vm_class_state state;
	uint16_t access_flags;
	uint16_t inner_class_access_flags;
	char *name;

	pthread_mutex_t mutex;

	struct vm_class *super;
	unsigned int nr_interfaces;
	struct vm_class **interfaces;
	unsigned int nr_fields;
	struct vm_field *fields;
	unsigned int nr_methods;
	struct vm_method *methods;
	unsigned int nr_annotations;
	struct vm_annotation **annotations;

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

	/* Reference to a classloader which loaded this class. Can be
	   NULL for default classloader. */
	struct vm_object *classloader;

	void *itable[VM_ITABLE_SIZE];

	unsigned int nr_inner_classes;
	uint16_t *inner_classes; /* class indices */

	struct vm_class *declaring_class;
	struct vm_class *enclosing_class;

	struct cafebabe_enclosing_method_attribute enclosing_method_attribute;

	/*
	 * The supertype cache contains pointers to 'struct vm_class' of
	 * recently accessed supertypes of this class. The purpose of this
	 * cache is to speed up subtype checks.
	 */
	const struct vm_class			*supertype_cache[SUPERTYPE_CACHE_SIZE];
	unsigned int				supertype_cache_ndx;
};

int vm_class_link(struct vm_class *vmc, const struct cafebabe_class *class);
int vm_class_link_primitive_class(struct vm_class *vmc, const char *class_name);
int vm_class_link_array_class(struct vm_class *vmc, struct vm_class *elem_class, const char *class_name);
int vm_class_init(struct vm_class *vmc);
int vm_class_ensure_object(struct vm_class *vmc);
int vm_class_setup_object(struct vm_class *vmc);

static inline int vm_class_ensure_init(struct vm_class *vmc)
{
	return vm_class_init(vmc);
}

static inline bool vm_class_is_public(const struct vm_class *vmc)
{
	return vmc->access_flags & CAFEBABE_CLASS_ACC_PUBLIC;
}

static inline bool vm_class_is_private(const struct vm_class *vmc)
{
	return vmc->access_flags & CAFEBABE_CLASS_ACC_PRIVATE;
}

static inline bool vm_class_is_protected(const struct vm_class *vmc)
{
	return vmc->access_flags & CAFEBABE_CLASS_ACC_PROTECTED;
}

static inline bool vm_class_is_static(const struct vm_class *vmc)
{
	return vmc->access_flags & CAFEBABE_CLASS_ACC_STATIC;
}

static inline bool vm_class_is_abstract(const struct vm_class *vmc)
{
	return vmc->access_flags & CAFEBABE_CLASS_ACC_ABSTRACT;
}

static inline bool vm_class_is_final(const struct vm_class *vmc)
{
	return vmc->access_flags & CAFEBABE_CLASS_ACC_FINAL;
}

static inline bool vm_class_is_interface(const struct vm_class *vmc)
{
	return vmc->access_flags & CAFEBABE_CLASS_ACC_INTERFACE;
}

static inline bool vm_class_is_array_class(const struct vm_class *vmc)
{
	return vmc->kind == VM_CLASS_KIND_ARRAY;
}

static inline bool vm_class_is_primitive_class(const struct vm_class *vmc)
{
	return vmc->kind == VM_CLASS_KIND_PRIMITIVE;
}

static inline bool vm_class_is_regular_class(const struct vm_class *vmc)
{
	return vmc->kind == VM_CLASS_KIND_REGULAR;
}

bool vm_class_is_anonymous(const struct vm_class *vmc);

struct vm_method *vm_class_get_enclosing_method(const struct vm_class *vmc);

static inline bool vm_class_is_local(const struct vm_class *vmc)
{
	return !vm_class_is_anonymous(vmc) && vm_class_get_enclosing_method(vmc);
}

static inline bool vm_class_is_member(const struct vm_class *vmc)
{
	return vmc->declaring_class;
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
	uint16_t i, uint16_t access_flags);

int vm_class_resolve_interface_method(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type);
struct vm_method *vm_class_resolve_interface_method_recursive(
	const struct vm_class *vmc, uint16_t i);

bool vm_class_is_assignable_from_slow(struct vm_class *vmc, const struct vm_class *from);

static inline bool supertype_cache_test(const struct vm_class *vmc, const struct vm_class *super)
{
	return vmc->supertype_cache[0] == super || vmc->supertype_cache[1] == super;
}

static inline bool vm_class_is_assignable_from(struct vm_class *vmc, const struct vm_class *from)
{
	return supertype_cache_test(vmc, from) || vm_class_is_assignable_from_slow(vmc, from);
}

bool vm_class_is_primitive_type_name(const char *class_name);
char *vm_class_get_array_element_class_name(const char *class_name);
struct vm_class *vm_class_get_array_element_class(const struct vm_class *array_class);
enum vm_type vm_class_get_storage_vmtype(const struct vm_class *class);
struct vm_class *vm_class_get_class_from_class_object(struct vm_object *clazz);
struct vm_class *vm_class_get_array_class(struct vm_class *element_class);
struct vm_class *vm_class_define(struct vm_object *classloader, const char *name, uint8_t *data, unsigned long len);

#define DECLARE_STATIC_FIELD_GETTER(type)				\
static inline j ## type							\
static_field_get_ ## type (const struct vm_field *field)		\
{									\
	assert(vm_field_is_static(field));				\
									\
	return *(j ## type *) &field->class->static_values[field->offset]; \
}

DECLARE_STATIC_FIELD_GETTER(byte);
DECLARE_STATIC_FIELD_GETTER(boolean);
DECLARE_STATIC_FIELD_GETTER(char);
DECLARE_STATIC_FIELD_GETTER(double);
DECLARE_STATIC_FIELD_GETTER(float);
DECLARE_STATIC_FIELD_GETTER(int);
DECLARE_STATIC_FIELD_GETTER(long);
DECLARE_STATIC_FIELD_GETTER(object);
DECLARE_STATIC_FIELD_GETTER(short);

#define DECLARE_STATIC_FIELD_SETTER(type)				\
static inline void							\
static_field_set_ ## type (const struct vm_field *field,		\
			   j ## type value)				\
{									\
	assert(vm_field_is_static(field));				\
									\
	*(j ## type *) &field->class->static_values[field->offset] = value; \
}

/*
 * We can not use generic setters for types of size less than machine
 * word. We currently load/store whole machine words therefore we must
 * set higher bits too, with sign extension for signed types.
 *
 * This should be fixed when register allocator finally supports
 * register constraints so that 8-bit and 16-bit load and stores can
 * be implemented in instruction selector.
 */

static inline void
static_field_set_byte(const struct vm_field *field, jbyte value)
{
	assert(vm_field_is_static(field));

	*(long *) &field->class->static_values[field->offset] = value;
}

static inline void
static_field_set_short(const struct vm_field *field,
		jshort value)
{
	assert(vm_field_is_static(field));

	*(long *) &field->class->static_values[field->offset] = value;
}

static inline void
static_field_set_boolean(const struct vm_field *field,
		  jboolean value)
{
	assert(vm_field_is_static(field));

	*(unsigned long *) &field->class->static_values[field->offset] = value;
}

static inline void
static_field_set_char(const struct vm_field *field, jchar value)
{
	assert(vm_field_is_static(field));

	*(unsigned long *) &field->class->static_values[field->offset] = value;
}

DECLARE_STATIC_FIELD_SETTER(double);
DECLARE_STATIC_FIELD_SETTER(float);
DECLARE_STATIC_FIELD_SETTER(int);
DECLARE_STATIC_FIELD_SETTER(long);
DECLARE_STATIC_FIELD_SETTER(object);

#endif /* __CLASS_H */
