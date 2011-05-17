#include "runtime/java_lang_reflect_VMMethod.h"

#include "vm/annotation.h"
#include "vm/reflection.h"
#include "vm/errors.h"
#include "vm/class.h"

#include <stddef.h>

jobject java_lang_reflect_VMMethod_getAnnotation(jobject this, jobject annotation)
{
	struct vm_class *annotation_type;
	struct vm_method *vmm;
	unsigned int i;

	annotation_type = vm_object_to_vm_class(annotation);

	vmm = vm_object_to_vm_method(this);
	if (!vmm)
		return rethrow_exception();

	for (i = 0; i < vmm->nr_annotations; i++) {
		struct vm_annotation *vma = vmm->annotations[i];
		struct vm_object *annotation;

		if (vm_annotation_get_type(vma) != annotation_type)
			continue;

		annotation = vm_annotation_to_object(vma);
		if (!annotation)
			return rethrow_exception();

		return annotation;
	}

	return NULL;
}

jobject java_lang_reflect_VMMethod_getDeclaredAnnotations(jobject klass)
{
	struct vm_object *result;
	struct vm_method *vmm;
	unsigned int i;

	vmm = vm_object_to_vm_method(klass);
	if (!vmm)
		return rethrow_exception();

	result = vm_object_alloc_array(vm_array_of_java_lang_annotation_Annotation, vmm->nr_annotations);
	if (!result)
		return rethrow_exception();

	for (i = 0; i < vmm->nr_annotations; i++) {
		struct vm_annotation *vma = vmm->annotations[i];
		struct vm_object *annotation;

		annotation = vm_annotation_to_object(vma);
		if (!annotation)
			return rethrow_exception();

		array_set_field_object(result, i, annotation);
	}

	return result;
}
