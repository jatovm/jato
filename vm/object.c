#include "vm/object.h"

#include "jit/exception.h"

#include "vm/classloader.h"
#include "vm/preload.h"
#include "vm/errors.h"
#include "vm/stdlib.h"
#include "vm/string.h"
#include "vm/class.h"
#include "vm/types.h"
#include "vm/call.h"
#include "vm/utf8.h"
#include "vm/die.h"
#include "vm/gc.h"

#include "lib/string.h"

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

static pthread_mutexattr_t obj_mutexattr;

int init_vm_objects(void)
{
	int err;

	err = pthread_mutexattr_init(&obj_mutexattr);
	if (err)
		return -err;

	err = pthread_mutexattr_settype(&obj_mutexattr,
		PTHREAD_MUTEX_RECURSIVE);
	if (err)
		return -err;

	return 0;
}

struct vm_object *vm_object_alloc(struct vm_class *class)
{
	struct vm_object *res;

	if (vm_class_ensure_init(class))
		return rethrow_exception();

	res = gc_alloc(sizeof(*res) + class->object_size);
	if (!res)
		return throw_oom_error();

	res->class = class;

	if (vm_monitor_init(&res->monitor))
		return throw_internal_error();

	return res;
}

struct vm_object *vm_object_alloc_primitive_array(int type, int count)
{
	struct vm_object *res;
	int vm_type;

	vm_type = bytecode_type_to_vmtype(type);
	assert(vm_type != J_VOID);

	res = gc_alloc(sizeof(*res) + get_vmtype_size(vm_type) * count);
	if (!res)
		return throw_oom_error();

	switch (type) {
	case T_BOOLEAN:
		res->class = classloader_load(NULL, "[Z");
		break;
	case T_CHAR:
		res->class = classloader_load(NULL, "[C");
		break;
	case T_FLOAT:
		res->class = classloader_load(NULL, "[F");
		break;
	case T_DOUBLE:
		res->class = classloader_load(NULL, "[D");
		break;
	case T_BYTE:
		res->class = classloader_load(NULL, "[B");
		break;
	case T_SHORT:
		res->class = classloader_load(NULL, "[S");
		break;
	case T_INT:
		res->class = classloader_load(NULL, "[I");
		break;
	case T_LONG:
		res->class = classloader_load(NULL, "[J");
		break;
	default:
		return throw_internal_error();
	}

	if (!res->class)
		return throw_internal_error();

	if (vm_class_ensure_init(res->class))
		return throw_internal_error();

	res->array_length = count;

	if (vm_monitor_init(&res->monitor))
		return throw_internal_error();

	return res;
}

struct vm_object *vm_object_alloc_multi_array(struct vm_class *class,
	int nr_dimensions, int *counts)
{
	struct vm_class *elem_class;
	struct vm_object *res;
	int elem_size;

	assert(nr_dimensions > 0);

	if (vm_class_ensure_init(class))
		return rethrow_exception();

	elem_class = vm_class_get_array_element_class(class);
	elem_size  = get_vmtype_size(vm_class_get_storage_vmtype(elem_class));

	res = gc_alloc(sizeof(*res) + elem_size * counts[0]);
	if (!res)
		return throw_oom_error();

	if (vm_monitor_init(&res->monitor))
		return throw_internal_error();

	res->array_length = counts[0];
	res->class = class;

	if (nr_dimensions == 1)
		return res;

	struct vm_object **elems = (struct vm_object **) (res + 1);

	for (int i = 0; i < counts[0]; ++i) {
		elems[i] = vm_object_alloc_multi_array(elem_class,
						nr_dimensions - 1, counts + 1);
	}

	return res;
}

struct vm_object *vm_object_alloc_array(struct vm_class *class, int count)
{
	struct vm_object *res;

	if (vm_class_ensure_init(class))
		return rethrow_exception();

	res = gc_alloc(sizeof(*res) + sizeof(struct vm_object *) * count);
	if (!res)
		return throw_oom_error();

	if (vm_monitor_init(&res->monitor)) {
		signal_new_exception(vm_java_lang_InternalError, NULL);
		return NULL;
	}

	res->array_length = count;

	struct vm_object **elems = (struct vm_object **) (res + 1);
	for (int i = 0; i < count; ++i)
		elems[i] = NULL;

	res->class = class;

	return res;
}

static struct vm_object *clone_regular(struct vm_object *obj)
{
	struct vm_class *vmc = obj->class;
	struct vm_object *new = vm_object_alloc(vmc);

	/* XXX: What do we do about exceptions? */
	if (new)
		memcpy(new + 1, obj + 1, vmc->object_size);

	return new;
}

static struct vm_object *clone_array(struct vm_object *obj)
{
	struct vm_class *vmc = obj->class;
	struct vm_class *e_vmc = vmc->array_element_class;
	int count = obj->array_length;

	if (e_vmc->kind == VM_CLASS_KIND_PRIMITIVE) {
		enum vm_type vmtype;
		int type;
		struct vm_object *new;

		vmtype = e_vmc->primitive_vm_type;
		type = vmtype_to_bytecode_type(vmtype);

		/* XXX: This could be optimized by not doing the memset() in
		 * object_alloc_primitive_array. */
		new = vm_object_alloc_primitive_array(type, count);
		if (new) {
			memcpy(new + 1, obj + 1,
				get_vmtype_size(vmtype) * count);
		}

		return new;
	} else {
		struct vm_object *new;

		new = vm_object_alloc_array(vmc, count);
		if (new) {
			memcpy(new + 1, obj + 1,
				sizeof(struct vm_object *) * count);
		}

		return new;
	}
}

static struct vm_object *clone_primitive(struct vm_object *obj)
{
	/* XXX: Is it even possible to create instances of primitive classes?
	 * I suspect it will be just the same as a normal object clone,
	 * though. Let's do that for now... */
	NOT_IMPLEMENTED;
	return clone_regular(obj);
}

struct vm_object *vm_object_clone(struct vm_object *obj)
{
	assert(obj);

	/* (In order of likelyhood:) */
	switch (obj->class->kind) {
	case VM_CLASS_KIND_REGULAR:
		return clone_regular(obj);
	case VM_CLASS_KIND_ARRAY:
		return clone_array(obj);
	case VM_CLASS_KIND_PRIMITIVE:
		return clone_primitive(obj);
	}

	/* Shouldn't happen. */
	assert(0);
	return NULL;
}

void vm_object_lock(struct vm_object *obj)
{
	vm_monitor_lock(&obj->monitor);
}

void vm_object_unlock(struct vm_object *obj)
{
	vm_monitor_unlock(&obj->monitor);
}

struct vm_object *
vm_object_alloc_string_from_utf8(const uint8_t bytes[], unsigned int length)
{
	struct vm_object *array = utf8_to_char_array(bytes, length);
	if (!array)
		return rethrow_exception();

	struct vm_object *string = vm_object_alloc(vm_java_lang_String);
	if (!string)
		return rethrow_exception();

	field_set_int(string, vm_java_lang_String_offset, 0);
	field_set_int(string, vm_java_lang_String_count, array->array_length);
	field_set_object(string, vm_java_lang_String_value, array);

	return vm_string_intern(string);
}

struct vm_object *
vm_object_alloc_string_from_c(const char *bytes)
{
	struct vm_object *string = vm_object_alloc(vm_java_lang_String);
	if (!string)
		return rethrow_exception();

	unsigned int n = strlen(bytes);
	struct vm_object *array
		= vm_object_alloc_primitive_array(T_CHAR, n);
	if (!array)
		return rethrow_exception();

#if 0
	/* XXX: Need to handle code points >= 0x80 */
	NOT_IMPLEMENTED;
#endif

	for (unsigned int i = 0; i < n; ++i) {
		array_set_field_char(array, i, bytes[i]);
	}

	field_set_int(string, vm_java_lang_String_offset, 0);
	field_set_int(string, vm_java_lang_String_count, array->array_length);
	field_set_object(string, vm_java_lang_String_value, array);

	return vm_string_intern(string);
}

typedef void (*exception_init_fn)(struct vm_object *, struct vm_object *);

struct vm_object *new_exception(struct vm_class *vmc, const char *message)
{
	struct vm_object *message_str;
	struct vm_method *mb;
	struct vm_object *obj;

	obj = vm_object_alloc(vmc);
	if (!obj)
		return rethrow_exception();

	if (message == NULL)
		message_str = NULL;
	else {
		message_str = vm_object_alloc_string_from_c(message);
		if (!message_str)
			return rethrow_exception();
	}

	mb = vm_class_get_method(vmc, "<init>", "(Ljava/lang/String;)V");
	if (!mb)
		error("constructor not found");

	vm_call_method(mb, obj, message_str);
	return obj;
}

bool vm_object_is_instance_of(const struct vm_object *obj,
	const struct vm_class *class)
{
	if (!obj)
		return false;

	return vm_class_is_assignable_from(class, obj->class);
}

void vm_object_check_null(struct vm_object *obj)
{
	NOT_IMPLEMENTED;
}

void vm_object_check_array(struct vm_object *obj, jsize index)
{
	jsize array_len;
	struct vm_class *cb;
	char index_str[32];

	cb = obj->class;

	if (!vm_class_is_array_class(cb)) {
		signal_new_exception(vm_java_lang_RuntimeException,
				     "object is not an array");
		return;
	}

	array_len = obj->array_length;

	if (index < array_len)
		return;

	sprintf(index_str, "%d > %d", index, array_len - 1);
	signal_new_exception(vm_java_lang_ArrayIndexOutOfBoundsException,
			     index_str);
}

void array_store_check(struct vm_object *arrayref, struct vm_object *obj)
{
	struct vm_class *class;
	struct vm_class *element_class;
	struct string *str;
	int err;

	if (obj == NULL)
		return;

	class = arrayref->class;

	if (!vm_class_is_array_class(class)) {
		signal_new_exception(vm_java_lang_RuntimeException,
				     "object is not an array");
		return;
	}

	element_class = vm_class_get_array_element_class(class);
	if (vm_class_is_assignable_from(element_class, obj->class))
		return;

	str = alloc_str();
	if (str == NULL) {
		err = -ENOMEM;
		goto error;
	}

	err = str_append(str, slash2dots(obj->class->name));
	if (err)
		goto error;

	signal_new_exception(vm_java_lang_ArrayStoreException, str->value);
	free_str(str);
	return;
 error:
	if (str)
		free_str(str);

	if (err == -ENOMEM) /* TODO: throw OutOfMemoryError */
		die("out of memory");

	die("error %d", err);
}

void array_store_check_vmtype(struct vm_object *arrayref, enum vm_type vm_type)
{
	/* TODO: Implement assignment compatibility checking described
	   in chapter "2.6.7 Assignment Conversion" of The Java VM
	   Specification - Second Edition. */
}

void vm_object_check_cast(struct vm_object *obj, struct vm_class *class)
{
	struct string *str;
	int err;

	if (!obj || vm_object_is_instance_of(obj, class))
		return;

	if (exception_occurred())
		return;

	str = alloc_str();
	if (str == NULL) {
		err = -ENOMEM;
		goto error;
	}

	err = str_append(str, slash2dots(obj->class->name));
	if (err)
		goto error;

	err = str_append(str, " cannot be cast to ");
	if (err)
		goto error;

	err = str_append(str, slash2dots(class->name));
	if (err)
		goto error;

	signal_new_exception(vm_java_lang_ClassCastException, str->value);
	free_str(str);
	return;
 error:
	if (str)
		free_str(str);

	if (err == -ENOMEM) /* TODO: throw OutOfMemoryError */
		die("out of memory");

	die("error %d", err);
}

void array_size_check(int size)
{
	if (size >= 0)
		return;

	signal_new_exception(vm_java_lang_NegativeArraySizeException, NULL);
}

void multiarray_size_check(int n, ...)
{
	va_list ap;
	int i;

	va_start(ap, n);

	for (i = 0; i < n; i++) {
		if (va_arg(ap, int) >= 0)
			continue;

		signal_new_exception(vm_java_lang_NegativeArraySizeException,
				     NULL);
		va_end(ap);
		return;
	}

	va_end(ap);
	return;
}

char *vm_string_to_cstr(const struct vm_object *string_obj)
{
	struct vm_object *array_object;
	struct string *str;
	int32_t offset;
	int32_t count;
	char *result;

	offset = field_get_int(string_obj, vm_java_lang_String_offset);
	count = field_get_int(string_obj, vm_java_lang_String_count);
	array_object = field_get_object(string_obj, vm_java_lang_String_value);

	str = alloc_str();
	if (!str)
		return NULL;

	result = NULL;

	for (int32_t i = 0; i < count; ++i) {
		int16_t ch;
		int err;

		ch = array_get_field_char(array_object, offset + i);

		if (ch < 128)
			err = str_append(str, "%c", ch);
		else
			err = str_append(str, "<%d>", ch);

		if (err)
			goto exit;
	}

	result = strdup(str->value);

 exit:
	free_str(str);
	return result;
}
