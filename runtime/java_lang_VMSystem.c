#include "runtime/java_lang_VMSystem.h"

#include "jit/exception.h"

#include "vm/preload.h"
#include "vm/object.h"
#include "vm/class.h"

void java_lang_VMSystem_arraycopy(jobject src, jint src_start, jobject dest, jint dest_start, jint len)
{
	const struct vm_class *src_elem_class;
	const struct vm_class *dest_elem_class;
	enum vm_type elem_type;
	int elem_size;

	if (!src || !dest || !src->class || !dest->class) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return;
	}

	if (!vm_class_is_array_class(src->class) ||
	    !vm_class_is_array_class(dest->class)) {
		signal_new_exception(vm_java_lang_ArrayStoreException, NULL);
		return;
	}

	src_elem_class = vm_class_get_array_element_class(src->class);
	dest_elem_class = vm_class_get_array_element_class(dest->class);
	if (!src_elem_class || !dest_elem_class) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return;
	}

	elem_type = vm_class_get_storage_vmtype(src_elem_class);
	if (elem_type != vm_class_get_storage_vmtype(dest_elem_class)) {
		signal_new_exception(vm_java_lang_ArrayStoreException, NULL);
		return;
	}

	if (len < 0 ||
	    src_start < 0 || src_start + len > vm_array_length(src) ||
	    dest_start < 0 || dest_start + len > vm_array_length(dest)) {
		signal_new_exception(
			vm_java_lang_ArrayIndexOutOfBoundsException, NULL);
		return;
	}

	elem_size = vmtype_get_size(elem_type);
	memmove(vm_array_elems(dest) + dest_start * elem_size,
		vm_array_elems(src) + src_start * elem_size,
		len * elem_size);

	return;
}

static int32_t hash_ptr_to_int32(void *p)
{
#ifdef CONFIG_64_BIT
	int64_t key = (int64_t) p;

	key = (~key) + (key << 18);
	key = key ^ (key >> 31);
	key = key * 21;
	key = key ^ (key >> 11);
	key = key + (key << 6);
	key = key ^ (key >> 22);

	return key;
#else
	return (int32_t) p;
#endif
}

jint java_lang_VMSystem_identityHashCode(struct vm_object *obj)
{
	return hash_ptr_to_int32(obj);
}
