#ifndef __VM_OBJECT_H
#define __VM_OBJECT_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "vm/jni.h"
#include "vm/field.h"
#include "vm/thread.h"
#include "vm/vm.h"

struct vm_class;
enum vm_type;

struct vm_monitor {
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	/* Holds the owner of this monitor or NULL when not locked. */
	struct vm_thread *owner;

	/*
	 * owner field might be read by other threads even if lock on
	 * @mutex is not held by them. For example vm_monitor_unlock()
	 * checks if the current thread is the owner of monitor. In
	 * SMP systems we must either ensure atomicity and use memory
	 * barriers or use a mutex. The latter approach is choosed.
	 */
	pthread_mutex_t owner_mutex;

	/*
	 * Holds the number of recursive locks on this monitor. This
	 * field is protected by @mutex.
	 */
	int lock_count;
};

struct vm_object {
	/* For arrays, this points to the array type, e.g. for int arrays,
	 * this points to the (artificial) class named "[I". We actually rely
	 * on this being the first field in the struct, because this way we
	 * don't need a null-pointer check for accessing this object whenever
	 * we access ->class first. */
	struct vm_class *class;

	struct vm_monitor monitor;

	int array_length;
	uint8_t fields[];
};

/* XXX: BUILD_BUG_ON(offsetof(vm_object, class) != 0); */

int init_vm_objects(void);

struct vm_object *vm_object_alloc(struct vm_class *class);
struct vm_object *vm_object_alloc_primitive_array(int type, int count);
struct vm_object *vm_object_alloc_multi_array(struct vm_class *class,
	int nr_dimensions, int *count);
struct vm_object *vm_object_alloc_array(struct vm_class *class, int count);

struct vm_object *vm_object_clone(struct vm_object *obj);

struct vm_object *
vm_object_alloc_string_from_utf8(const uint8_t bytes[], unsigned int length);
struct vm_object *vm_object_alloc_string_from_c(const char *bytes);
struct vm_object *new_exception(struct vm_class *vmc, const char *message);
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

int vm_monitor_init(struct vm_monitor *mon);
int vm_monitor_lock(struct vm_monitor *mon);
int vm_monitor_unlock(struct vm_monitor *mon);
int vm_monitor_wait(struct vm_monitor *mon);
int vm_monitor_timed_wait(struct vm_monitor *mon, long long ms, int ns);
int vm_monitor_notify(struct vm_monitor *mon);
int vm_monitor_notify_all(struct vm_monitor *mon);
struct vm_thread *vm_monitor_get_owner(struct vm_monitor *mon);
void vm_monitor_set_owner(struct vm_monitor *mon, struct vm_thread *owner);

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
field_set_int64(struct vm_object *obj, const struct vm_field *field, int64_t value)
{
	*(int64_t *) &obj->fields[field->offset] = value;
}

static inline int64_t
field_get_int64(const struct vm_object *obj, const struct vm_field *field)
{
	return *(int64_t *) &obj->fields[field->offset];
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

static inline void
array_set_field_char(struct vm_object *obj, int index, uint16_t value)
{
	*(uint16_t *) &obj->fields[index * get_vmtype_size(J_CHAR)] = value;
}

static inline uint16_t
array_get_field_char(struct vm_object *obj, int index)
{
	return *(uint16_t *) &obj->fields[index * get_vmtype_size(J_CHAR)];
}

static inline uint8_t
array_get_field_byte(struct vm_object *obj, int index)
{
	return *(uint8_t *) &obj->fields[index * get_vmtype_size(J_BYTE)];
}

static inline void
array_set_field_byte(struct vm_object *obj, int index, uint8_t value)
{
	*(uint8_t *) &obj->fields[index * get_vmtype_size(J_BYTE)] = value;
}

static inline void
array_set_field_boolean(struct vm_object *obj, int index, jboolean value)
{
	*(jboolean *) &obj->fields[index * get_vmtype_size(J_BOOLEAN)] = value;
}

static inline jboolean
array_get_field_boolean(struct vm_object *obj, int index)
{
	return *(jboolean *) &obj->fields[index * get_vmtype_size(J_BOOLEAN)];
}

static inline void
array_set_field_short(struct vm_object *obj, int index, jshort value)
{
	*(jshort *) &obj->fields[index * get_vmtype_size(J_SHORT)] = value;
}

static inline jshort
array_get_field_short(struct vm_object *obj, int index)
{
	return *(jshort *) &obj->fields[index * get_vmtype_size(J_SHORT)];
}

static inline void
array_set_field_int(struct vm_object *obj, int index, jint value)
{
	*(jint *) &obj->fields[index * get_vmtype_size(J_INT)] = value;
}

static inline jint
array_get_field_int(struct vm_object *obj, int index)
{
	return *(jint *) &obj->fields[index * get_vmtype_size(J_INT)];
}

static inline void
array_set_field_long(struct vm_object *obj, int index, jlong value)
{
	*(jlong *) &obj->fields[index * get_vmtype_size(J_LONG)] = value;
}

static inline jlong
array_get_field_long(struct vm_object *obj, int index)
{
	return *(jlong *) &obj->fields[index * get_vmtype_size(J_LONG)];
}

static inline void
array_set_field_float(struct vm_object *obj, int index, jfloat value)
{
	*(jfloat *) &obj->fields[index * get_vmtype_size(J_FLOAT)] = value;
}

static inline float
array_get_field_float(struct vm_object *obj, int index)
{
	return *(float *) &obj->fields[index * get_vmtype_size(J_FLOAT)];
}

static inline void
array_set_field_double(struct vm_object *obj, int index, double value)
{
	*(double *) &obj->fields[index * get_vmtype_size(J_DOUBLE)] = value;
}

static inline double
array_get_field_double(struct vm_object *obj, int index)
{
	return *(double *) &obj->fields[index * get_vmtype_size(J_DOUBLE)];
}

static inline void
array_set_field_ptr(struct vm_object *obj, int index, void *value)
{
	*(void **) &obj->fields[index * get_vmtype_size(J_REFERENCE)] = value;
}

static inline void *
array_get_field_ptr(struct vm_object *obj, int index)
{
	return *(void **) &obj->fields[index * get_vmtype_size(J_REFERENCE)];
}

#endif
