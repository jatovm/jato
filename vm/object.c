#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <jit/exception.h>

#include <vm/class.h>
#include <vm/classloader.h>
#include <vm/die.h>
#include <vm/java_lang.h>
#include <vm/object.h>
#include <vm/stdlib.h>
#include <vm/string.h>
#include <vm/utf8.h>

struct vm_object *vm_object_alloc(struct vm_class *class)
{
	struct vm_object *res;

	res = zalloc(sizeof(*res) + class->object_size);
	if (res) {
		res->class = class;
	}

	if (pthread_mutex_init(&res->mutex, NULL))
		NOT_IMPLEMENTED;

	return res;
}

struct vm_object *vm_object_alloc_native_array(int type, int count)
{
	struct vm_object *res;

	/* XXX: Use the right size... */
	res = zalloc(sizeof(*res) + 8 * count);
	if (!res) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	switch (type) {
	case T_BOOLEAN:
		res->class = classloader_load("[Z");
		break;
	case T_CHAR:
		res->class = classloader_load("[C");
		break;
	case T_FLOAT:
		res->class = classloader_load("[F");
		break;
	case T_DOUBLE:
		res->class = classloader_load("[D");
		break;
	case T_BYTE:
		res->class = classloader_load("[B");
		break;
	case T_SHORT:
		res->class = classloader_load("[S");
		break;
	case T_INT:
		res->class = classloader_load("[I");
		break;
	case T_LONG:
		res->class = classloader_load("[J");
		break;
	default:
		NOT_IMPLEMENTED;
	}

	if (!res->class) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	res->array_length = count;

	if (pthread_mutex_init(&res->mutex, NULL))
		NOT_IMPLEMENTED;

	return res;
}

struct vm_object *vm_object_alloc_multi_array(struct vm_class *class,
	int nr_dimensions, int *counts)
{
	assert(nr_dimensions > 0);

	struct vm_object *res;

	res = zalloc(sizeof(*res) + sizeof(struct vm_object *) * counts[0]);
	if (!res) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	if (pthread_mutex_init(&res->mutex, NULL))
		NOT_IMPLEMENTED;

	res->array_length = counts[0];

	struct vm_object **elems = (struct vm_object **) (res + 1);

	if (nr_dimensions == 1) {
		for (int i = 0; i < counts[0]; ++i)
			elems[i] = NULL;
	} else {
		for (int i = 0; i < counts[0]; ++i) {
			elems[i] = vm_object_alloc_multi_array(class,
				nr_dimensions - 1, counts + 1);
		}
	}

	res->class = class;

	return res;
}

struct vm_object *vm_object_alloc_array(struct vm_class *class, int count)
{
	struct vm_object *res;

	res = zalloc(sizeof(*res) + sizeof(struct vm_object *) * count);
	if (!res) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	if (pthread_mutex_init(&res->mutex, NULL))
		NOT_IMPLEMENTED;

	res->array_length = count;

	struct vm_object **elems = (struct vm_object **) (res + 1);
	for (int i = 0; i < count; ++i)
		elems[i] = NULL;

	res->class = class;

	return res;
}

void vm_object_lock(struct vm_object *obj)
{
	if (pthread_mutex_lock(&obj->mutex))
		NOT_IMPLEMENTED;
}

void vm_object_unlock(struct vm_object *obj)
{
	if (pthread_mutex_unlock(&obj->mutex))
		NOT_IMPLEMENTED;
}

struct vm_object *
vm_object_alloc_string(const uint8_t bytes[], unsigned int length)
{
	struct vm_object *array = utf8_to_char_array(bytes, length);
	if (!array) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	struct vm_object *string = vm_object_alloc(vm_java_lang_String);
	if (!string) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	*(int32_t *) &string->fields[vm_java_lang_String_offset->offset] = 0;
	*(int32_t *) &string->fields[vm_java_lang_String_count->offset]
		= array->array_length;
	*(void **) &string->fields[vm_java_lang_String_value->offset] = array;

	return string;
}

typedef void (*exception_init_fn)(struct vm_object *, struct vm_object *);

struct vm_object *new_exception(const char *class_name, const char *message)
{
	struct vm_object *message_str;
	exception_init_fn init;
	struct vm_method *mb;
	struct vm_object *obj;
	struct vm_class *e_class;

	/* XXX: load_and_init? We'd better preload exception classes. */
	e_class = classloader_load(class_name);
	if (!e_class)
		return NULL;

	obj = vm_object_alloc(e_class);
	if (!obj)
		return NULL;

	if (message == NULL)
		message_str = NULL;
	else
		message_str = vm_object_alloc_string(message, strlen(message));

	mb = vm_class_get_method(e_class,
		"<init>", "(Ljava/lang/String;)V");
	if (!mb)
		die("%s: constructor not found for class %s\n",
		    __func__, class_name);

	init = vm_method_trampoline_ptr(mb);
	init(obj, message_str);

	return obj;
}

bool vm_object_is_instance_of(struct vm_object *obj, struct vm_class *class)
{
	if (!obj)
		return false;

	return vm_class_is_assignable_from(class, obj->class);;
}

void vm_object_check_null(struct vm_object *obj)
{
	NOT_IMPLEMENTED;
}

void vm_object_check_array(struct vm_object *obj, unsigned int index)
{
	unsigned int array_len;
	struct vm_class *cb;
	char index_str[32];

	cb = obj->class;

	if (!vm_class_is_array_class(cb)) {
		signal_new_exception("java/lang/RuntimeException",
				     "object is not an array");
		goto throw;
	}

	array_len = obj->array_length;

	if (index < array_len)
		return;

	sprintf(index_str, "%d > %d", index, array_len - 1);
	signal_new_exception("java/lang/ArrayIndexOutOfBoundsException",
			     index_str);

 throw:
	throw_from_native(sizeof(struct vm_object *) + sizeof(unsigned int));
}

void array_store_check(struct vm_object *arrayref, struct vm_object *obj)
{
	NOT_IMPLEMENTED;

#if 0
	struct vm_class *cb;
	struct string *str;
	int err;

	cb = arrayref->class;

	if (!IS_ARRAY(cb)) {
		signal_new_exception("java/lang/RuntimeException",
				     "object is not an array");
		goto throw;
	}

	if (obj == NULL || isInstanceOf(cb->element_class, obj->class))
		return;

	str = alloc_str();
	if (str == NULL) {
		err = -ENOMEM;
		goto error;
	}

	err = str_append(str, slash2dots(obj->class->name));
	if (err)
		goto error;

	signal_new_exception("java/lang/ArrayStoreException", str->value);
	free_str(str);

 throw:
	throw_from_native(2 * sizeof(struct vm_object *));
	return;

 error:
	if (str)
		free_str(str);

	if (err == -ENOMEM) /* TODO: throw OutOfMemoryError */
		die("%s: out of memory", __func__);

	die("%s: error %d", __func__, err);
#endif
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
		goto throw;

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

	signal_new_exception("java/lang/ClassCastException", str->value);
	free_str(str);
 throw:
	throw_from_native(2 * sizeof(struct vm_object *));
	return;

 error:
	if (str)
		free_str(str);

	if (err == -ENOMEM) /* TODO: throw OutOfMemoryError */
		die("%s: out of memory", __func__);

	die("%s: error %d", __func__, err);
}

void array_size_check(int size)
{
	if (size < 0) {
		signal_new_exception(
			"java/lang/NegativeArraySizeException", NULL);
		throw_from_native(sizeof(int));
	}
}

void multiarray_size_check(int n, ...)
{
	va_list ap;
	int i;

	va_start(ap, n);

	for (i = 0; i < n; i++) {
		if (va_arg(ap, int) >= 0)
			continue;

		signal_new_exception("java/lang/NegativeArraySizeException",
				     NULL);
		va_end(ap);
		throw_from_native(sizeof(int) * (n + 1));
		return;
	}

	va_end(ap);
	return;
}
