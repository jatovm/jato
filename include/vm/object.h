#ifndef __VM_OBJECT_H
#define __VM_OBJECT_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <vm/field.h>
#include <vm/vm.h>

struct vm_class;
enum vm_type;

struct vm_object {
	/* For arrays, this points to the array type, e.g. for int arrays,
	 * this points to the (artificial) class named "[I". */
	struct vm_class *class;

	/* For instances of java.lang.Class this points to the class
	   represented by this java.lang.Class instance. */
	struct vm_class *java_lang_Class_class;

	pthread_mutex_t mutex;

	unsigned int array_length;
	uint8_t fields[];
};

struct vm_object *vm_object_alloc(struct vm_class *class);
struct vm_object *vm_object_alloc_native_array(int type, int count);
struct vm_object *vm_object_alloc_multi_array(struct vm_class *class,
	int nr_dimensions, int *count);
struct vm_object *vm_object_alloc_array(struct vm_class *class, int count);

struct vm_object *
vm_object_alloc_string(const uint8_t bytes[], unsigned int length);

struct vm_object *new_exception(const char *class_name, const char *message);

bool vm_object_is_instance_of(const struct vm_object *obj, const struct vm_class *class);
void vm_object_check_null(struct vm_object *obj);
void vm_object_check_array(struct vm_object *obj, unsigned int index);
void vm_object_check_cast(struct vm_object *obj, struct vm_class *class);

void vm_object_lock(struct vm_object *obj);
void vm_object_unlock(struct vm_object *obj);

void array_store_check(struct vm_object *arrayref, struct vm_object *obj);
void array_store_check_vmtype(struct vm_object *arrayref, enum vm_type vm_type);
void array_size_check(int size);
void multiarray_size_check(int n, ...);
char *vm_string_to_cstr(const struct vm_object *string);

static inline void
field_set_int32(struct vm_object *obj, const struct vm_field *field, int32_t value)
{
	*(int32_t *) &obj->fields[field->offset] = value;
}

static inline int32_t
field_get_int32(const struct vm_object *obj, const struct vm_field *field)
{
	return *(int32_t *) &obj->fields[field->offset];
}

static inline void
field_set_object(struct vm_object *obj, const struct vm_field *field, struct vm_object *value)
{
	*(void **) &obj->fields[field->offset] = value;
}

static inline struct vm_object *
field_get_object(const struct vm_object *obj, const struct vm_field *field)
{
	return *(struct vm_object **) &obj->fields[field->offset];
}

#endif
