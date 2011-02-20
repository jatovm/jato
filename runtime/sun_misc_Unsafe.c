/*
 * Copyright (c) 2009, 2011 Pekka Enberg
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

#include "runtime/sun_misc_Unsafe.h"

#include "jit/exception.h"
#include "arch/atomic.h"

#include "vm/reflection.h"
#include "vm/preload.h"
#include "vm/object.h"
#include "vm/class.h"
#include "vm/jni.h"

jint sun_misc_Unsafe_arrayBaseOffset(jobject this, jobject class)
{
	return VM_ARRAY_ELEMS_OFFSET;
}

jint sun_misc_Unsafe_arrayIndexScale(jobject this, jobject class)
{
	struct vm_class *array_class;
	struct vm_class *elem_class;
	int elem_type;

	array_class	= vm_class_get_class_from_class_object(class);

	elem_class	= vm_class_get_array_element_class(array_class);

	elem_type	= vm_class_get_storage_vmtype(elem_class);

	return vmtype_get_size(elem_type);
}

jint sun_misc_Unsafe_getIntVolatile(jobject this, jobject obj, jlong offset)
{
	jint *value_p = (void *) obj + offset;
	jint ret;

	ret		= *value_p;

	mb();

	return ret;
}

jlong sun_misc_Unsafe_getLongVolatile(jobject this, jobject obj, jlong offset)
{
	jlong *value_p = (void *) obj + offset;
	jlong ret;

	ret		= *value_p;

	mb();

	return ret;
}

jobject sun_misc_Unsafe_getObjectVolatile(jobject this, jobject obj, jlong offset)
{
	struct vm_object **value_p = (void *) obj + offset;
	jobject ret;

	ret		= *value_p;

	mb();

	return ret;
}

jlong sun_misc_Unsafe_objectFieldOffset(jobject this, jobject field)
{
	struct vm_field *vmf;

	if (vm_java_lang_reflect_VMField != NULL) {	/* Classpath 0.98 */
		field	= field_get_object(field, vm_java_lang_reflect_Field_f);
	}

	vmf = vm_object_to_vm_field(field);
	if (!vmf)
		return 0;

	return VM_OBJECT_FIELDS_OFFSET + vmf->offset;
}

void sun_misc_Unsafe_putIntVolatile(jobject this, jobject obj, jlong offset, jint value)
{
	jint *value_p = (void *) obj + offset;

	mb();

	*value_p	= value;
}

void sun_misc_Unsafe_putLong(jobject this, jobject obj, jlong offset, jlong value)
{
	jlong *value_p = (void *) obj + offset;

	*value_p	= value;
}

void sun_misc_Unsafe_putLongVolatile(jobject this, jobject obj, jlong offset, jlong value)
{
	jlong *value_p = (void *) obj + offset;

	mb();

	*value_p	= value;
}

void sun_misc_Unsafe_putObject(jobject this, jobject obj, jlong offset, jobject value)
{
	struct vm_object **value_p = (void *) obj + offset;

	*value_p	= value;
}

void sun_misc_Unsafe_putObjectVolatile(jobject this, jobject obj, jlong offset, jobject value)
{
	struct vm_object **value_p = (void *) obj + offset;

	mb();

	*value_p	= value;
}

jint native_unsafe_compare_and_swap_int(struct vm_object *this,
					struct vm_object *obj, jlong offset,
					jint expect, jint update)
{
	void *p = (void *) obj + offset;

	return cmpxchg_32(p, (uint32_t)expect, (uint32_t)update) == (uint32_t)expect;
}

jint native_unsafe_compare_and_swap_long(struct vm_object *this,
					 struct vm_object *obj, jlong offset,
					 jlong expect, jlong update)
{
	void *p = (void *) obj + offset;

	return cmpxchg_64(p, (uint64_t)expect, (uint64_t)update) == (uint64_t)expect;
}

jint native_unsafe_compare_and_swap_object(struct vm_object *this,
					   struct vm_object *obj,
					   jlong offset,
					   struct vm_object *expect,
					    struct vm_object *update)
{
	void *p = (void *) obj + offset;

	return cmpxchg_ptr(p, expect, update) == expect;
}

void native_unsafe_park(struct vm_object *this, jboolean isAbsolute,
			jlong timeout)
{
	struct vm_thread *self = vm_thread_self();
	struct timespec timespec;


	pthread_mutex_lock(&self->park_mutex);

	if (self->unpark_called) {
		self->unpark_called = false;
		pthread_mutex_unlock(&self->park_mutex);
		return;
	}

	if (timeout == 0) {
		pthread_cond_wait(&self->park_cond, &self->park_mutex);
	} else {
		/* If isAbsolute == true then timeout is in
		 * miliseconds otherwise in nanoseconds. */
		if (isAbsolute) {
			timespec.tv_sec  = timeout / 1000l;
			timespec.tv_nsec = timeout % 1000l;
		} else {
			clock_gettime(CLOCK_REALTIME, &timespec);
			timespec.tv_sec  += timeout / 1000000000l;
			timespec.tv_nsec += timeout % 1000000000l;
		}

		pthread_cond_timedwait(&self->park_cond, &self->park_mutex, &timespec);
	}

	if (self->unpark_called) {
		self->unpark_called = false;
	}

	pthread_mutex_unlock(&self->park_mutex);
}

void native_unsafe_unpark(struct vm_object *this, struct vm_object *vmthread)
{
	struct vm_thread *thread;

	thread = vm_thread_from_java_thread(vmthread);

	pthread_mutex_lock(&thread->park_mutex);
	thread->unpark_called = true;
	pthread_cond_signal(&thread->park_cond);
	pthread_mutex_unlock(&thread->park_mutex);
}
