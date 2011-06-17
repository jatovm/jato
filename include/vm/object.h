#ifndef __VM_OBJECT_H
#define __VM_OBJECT_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "vm/monitor.h"
#include "vm/system.h"
#include "vm/field.h"
#include "vm/jni.h"
#include "vm/vm.h"

#include "arch/atomic.h"

struct vm_class;
enum vm_type;

struct vm_object {
	/* For arrays, this points to the array type, e.g. for int arrays,
	 * this points to the (artificial) class named "[I". We actually rely
	 * on this being the first field in the struct, because this way we
	 * don't need a null-pointer check for accessing this object whenever
	 * we access ->class first. */
	struct vm_class		*class;
	void			*monitor_record;
};

struct vm_array {
	struct vm_object	object;
	jsize			array_length;
	/* Array elements start here. */
};

static inline struct vm_array *vm_object_to_array(struct vm_object *self)
{
	return container_of(self, struct vm_array, object);
}

static inline jsize vm_array_length(struct vm_object *self)
{
	struct vm_array *array = vm_object_to_array(self);

	return array->array_length;
}

#define VM_OBJECT_FIELDS_OFFSET sizeof(struct vm_object)

static inline uint8_t *vm_object_fields(const struct vm_object *obj)
{
	return (uint8_t *) obj + VM_OBJECT_FIELDS_OFFSET;
}

#define VM_ARRAY_ELEMS_OFFSET	sizeof(struct vm_array)

static inline void *vm_array_elems(const struct vm_object *obj)
{
	return (void *) obj + VM_ARRAY_ELEMS_OFFSET;
}

/* XXX: BUILD_BUG_ON(offsetof(vm_object, class) != 0); */

int init_vm_objects(void);

struct vm_object *vm_object_alloc(struct vm_class *class);
struct vm_object *vm_object_alloc_array_raw(struct vm_class *class, size_t elem_size, int count);
struct vm_object *vm_object_alloc_primitive_array(int type, int count);
struct vm_object *vm_object_alloc_multi_array(struct vm_class *class,
	int nr_dimensions, int *count);
struct vm_object *vm_object_alloc_array(struct vm_class *class, int count);
struct vm_object *vm_object_alloc_array_of(struct vm_class *elem_class, int count);

struct vm_object *vm_object_clone(struct vm_object *obj);

struct vm_object *
vm_object_alloc_string_from_utf8(const uint8_t bytes[], unsigned int length);
struct vm_object *vm_object_alloc_string_from_c(const char *bytes);
bool vm_object_is_instance_of(const struct vm_object *obj, const struct vm_class *class);
void vm_object_check_null(struct vm_object *obj);
void vm_object_check_array(struct vm_object *obj, jsize index);
void vm_object_check_cast(struct vm_object *obj, struct vm_class *class);
void vm_object_finalizer(struct vm_object *object);

void array_store_check(struct vm_object *arrayref, struct vm_object *obj);
void array_store_check_vmtype(struct vm_object *arrayref, enum vm_type vm_type);
void array_size_check(int size);
void multiarray_size_check(int n, ...);
char *vm_string_to_cstr(const struct vm_object *string);

#define DECLARE_FIELD_SETTER(type)					\
static inline void							\
field_set_ ## type (struct vm_object *obj, const struct vm_field *field,\
		    j ## type value)					\
{									\
	uint8_t *fields = vm_object_fields(obj);			\
									\
	*(j ## type *) &fields[field->offset] = value;			\
}

#define DECLARE_FIELD_GETTER(type)					\
static inline j ## type							\
field_get_ ## type (const struct vm_object *obj, const struct vm_field *field)\
{									\
	uint8_t *fields = vm_object_fields(obj);			\
									\
	return *(j ## type *) &fields[field->offset];			\
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
field_set_byte(struct vm_object *obj, const struct vm_field *field, jbyte value)
{
	uint8_t *fields = vm_object_fields(obj);

	*(long *) &fields[field->offset] = value;
}

static inline void
field_set_short(struct vm_object *obj, const struct vm_field *field,
		jshort value)
{
	uint8_t *fields = vm_object_fields(obj);

	*(long *) &fields[field->offset] = value;
}

static inline void
field_set_boolean(struct vm_object *obj, const struct vm_field *field,
		  jboolean value)
{
	uint8_t *fields = vm_object_fields(obj);

	*(unsigned long *) &fields[field->offset] = value;
}

static inline void
field_set_char(struct vm_object *obj, const struct vm_field *field, jchar value)
{
	uint8_t *fields = vm_object_fields(obj);

	*(unsigned long *) &fields[field->offset] = value;
}

static inline void*
field_get_object_ptr(struct vm_object *obj, jlong offset)
{
	uint8_t *fields = vm_object_fields(obj);

	return &fields[offset];
}

DECLARE_FIELD_SETTER(double);
DECLARE_FIELD_SETTER(float);
DECLARE_FIELD_SETTER(int);
DECLARE_FIELD_SETTER(long);
DECLARE_FIELD_SETTER(object);

DECLARE_FIELD_GETTER(byte);
DECLARE_FIELD_GETTER(boolean);
DECLARE_FIELD_GETTER(char);
DECLARE_FIELD_GETTER(double);
DECLARE_FIELD_GETTER(float);
DECLARE_FIELD_GETTER(int);
DECLARE_FIELD_GETTER(long);
DECLARE_FIELD_GETTER(object);
DECLARE_FIELD_GETTER(short);

#define DECLARE_ARRAY_FIELD_SETTER(type, vmtype)			\
static inline void							\
array_set_field_ ## type(struct vm_object *obj, int index,		\
			 j ## type value)				\
{									\
	uint8_t *fields = vm_array_elems(obj);				\
									\
	*(j ## type *) &fields[index * vmtype_get_size(vmtype)] = value; \
}

#define DECLARE_ARRAY_FIELD_GETTER(type, vmtype)			\
static inline j ## type							\
array_get_field_ ## type(const struct vm_object *obj, int index)	\
{									\
	uint8_t *fields = vm_array_elems(obj);				\
									\
	return *(j ## type *) &fields[index * vmtype_get_size(vmtype)]; \
}

static inline void
array_set_field_byte(struct vm_object *obj, int index, jbyte value)
{
	uint8_t *fields = vm_array_elems(obj);

	*(long *) &fields[index * vmtype_get_size(J_BYTE)] = value;
}

static inline void
array_set_field_short(struct vm_object *obj, int index, jshort value)
{
	uint8_t *fields = vm_array_elems(obj);

	*(long *) &fields[index * vmtype_get_size(J_SHORT)] = value;
}

static inline void
array_set_field_boolean(struct vm_object *obj, int index, jboolean value)
{
	uint8_t *fields = vm_array_elems(obj);

	*(unsigned long *) &fields[index * vmtype_get_size(J_BOOLEAN)] = value;
}

static inline void
array_set_field_char(struct vm_object *obj, int index, jchar value)
{
	uint8_t *fields = vm_array_elems(obj);

	*(unsigned long *) &fields[index * vmtype_get_size(J_CHAR)] = value;
}

DECLARE_ARRAY_FIELD_SETTER(double, J_DOUBLE);
DECLARE_ARRAY_FIELD_SETTER(float, J_FLOAT);
DECLARE_ARRAY_FIELD_SETTER(int, J_INT);
DECLARE_ARRAY_FIELD_SETTER(long, J_LONG);
DECLARE_ARRAY_FIELD_SETTER(object, J_REFERENCE);

DECLARE_ARRAY_FIELD_GETTER(byte, J_BYTE);
DECLARE_ARRAY_FIELD_GETTER(boolean, J_BOOLEAN);
DECLARE_ARRAY_FIELD_GETTER(char, J_CHAR);
DECLARE_ARRAY_FIELD_GETTER(double, J_DOUBLE);
DECLARE_ARRAY_FIELD_GETTER(float, J_FLOAT);
DECLARE_ARRAY_FIELD_GETTER(int, J_INT);
DECLARE_ARRAY_FIELD_GETTER(long, J_LONG);
DECLARE_ARRAY_FIELD_GETTER(object, J_REFERENCE);
DECLARE_ARRAY_FIELD_GETTER(short, J_SHORT);

static inline void
array_set_field_ptr(struct vm_object *obj, int index, void *value)
{
	uint8_t *fields = vm_array_elems(obj);

	*(void **) &fields[index * vmtype_get_size(J_NATIVE_PTR)] = value;
}

static inline void *
array_get_field_ptr(struct vm_object *obj, int index)
{
	uint8_t *fields = vm_array_elems(obj);

	return *(void **) &fields[index * vmtype_get_size(J_NATIVE_PTR)];
}

#endif
