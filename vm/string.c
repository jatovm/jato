/*
 * Copyright (c) 2009 Tomasz Grabiec
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

#include "vm/string.h"

#include "jit/exception.h"

#include "vm/preload.h"
#include "vm/errors.h"
#include "vm/object.h"
#include "vm/die.h"
#include "vm/reference.h"

#include "lib/hash-map.h"

#include <pthread.h>
#include <memory.h>

static struct hash_map *literals;
static pthread_mutex_t literals_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Compare key1 string and key2 weak reference to string.
 */
static bool string_obj_equals(const void *key1, const void *key2)
{
	struct vm_object *array1, *array2;
	jint offset1, offset2;
	jint count1, count2;

	count1 = field_get_int(key1, vm_java_lang_String_count);
	count2 = field_get_int(key2, vm_java_lang_String_count);

	if (count1 != count2)
		return false;

	offset1 = field_get_int(key1, vm_java_lang_String_offset);
	offset2 = field_get_int(key2, vm_java_lang_String_offset);

	array1 = field_get_object(key1, vm_java_lang_String_value);
	array2 = field_get_object(key2, vm_java_lang_String_value);

	int fsize = vmtype_get_size(J_CHAR);

	return memcmp(vm_array_elems(array1) + offset1 * fsize,
		      vm_array_elems(array2) + offset2 * fsize,
		      count1 * fsize) == 0;
}

static unsigned long string_obj_hash(const void *key)
{
	struct vm_object *array;
	unsigned long hash;
	jint offset;
	jint count;

	offset = field_get_int(key, vm_java_lang_String_offset);
	count = field_get_int(key, vm_java_lang_String_count);
	array = field_get_object(key, vm_java_lang_String_value);

	hash = 0;

	for (jint i = 0; i < count; i++)
		hash += 31 * hash + array_get_field_char(array, i + offset);

	return hash;
}

static struct key_operations string_obj_key_ops = {
	.hash	= string_obj_hash,
	.equals	= string_obj_equals
};

void init_literals_hash_map(void)
{
	literals = alloc_hash_map(1000, &string_obj_key_ops);
	if (!literals)
		error("failed to initialize literals hash map");
}

struct vm_object *vm_string_intern(struct vm_object *string)
{
	struct vm_reference *intern;
	struct vm_object *result;

	pthread_mutex_lock(&literals_mutex);

	if (hash_map_get(literals, string, (void **) &intern) == 0) {
		pthread_mutex_unlock(&literals_mutex);
		result = vm_reference_get(intern);
		return result;
	}

	result = string;
	intern = vm_reference_alloc(result, VM_REFERENCE_STRONG);
	if (!intern) {
		result = throw_oom_error();
		goto out;
	}

	if (hash_map_put(literals, string, intern))
		result = throw_oom_error();

 out:
	pthread_mutex_unlock(&literals_mutex);
	return result;
}
