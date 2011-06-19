#include "runtime/java_lang_reflect_VMField.h"

#include "vm/annotation.h"
#include "vm/reflection.h"
#include "vm/boxing.h"
#include "vm/errors.h"
#include "vm/class.h"

#include <stddef.h>
#include <stdlib.h>

static void *field_get_value(struct vm_field *vmf, struct vm_object *o)
{
	void *value_p;

	/*
	 * TODO: "If this Field enforces access control, your runtime
	 * context is evaluated, and you may have an
	 * IllegalAccessException if you could not access this field
	 * in similar compiled code". (java/lang/reflect/Field.java)
	 */

	if (vm_field_is_static(vmf)) {
		value_p = vmf->class->static_values + vmf->offset;
	} else {
		/*
		 * If o is null, you get a NullPointerException, and
		 * if it is incompatible with the declaring class of
		 * the field, you get an IllegalArgumentException.
		 */
		if (!o) {
			signal_new_exception(vm_java_lang_NullPointerException, NULL);
			return NULL;
		}

		if (!vm_object_is_instance_of(o, vmf->class)) {
			signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
			return NULL;
		}

		value_p = field_get_object_ptr(o, vmf->offset);
	}

	return value_p;
}

struct vm_object *java_lang_reflect_VMField_get(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	enum vm_type type;
	void *value_p;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return NULL;

	type	= vm_field_type(vmf);
	value_p	= field_get_value(vmf, o);
	if (!value_p)
		return NULL;

	return jvalue_to_object((union jvalue *) value_p, type);
}

static jlong to_jlong_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_BYTE:
		return value->b;
	case J_CHAR:
		return value->c;
	case J_SHORT:
		return value->s;
	case J_LONG:
		return value->j;
	case J_INT:
		return value->i;
	case J_BOOLEAN:
	case J_DOUBLE:
	case J_FLOAT:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}

static jint to_jint_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_BYTE:
		return value->b;
	case J_CHAR:
		return value->c;
	case J_SHORT:
		return value->s;
	case J_INT:
		return value->i;
	case J_LONG:
	case J_BOOLEAN:
	case J_DOUBLE:
	case J_FLOAT:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}

static jshort to_jshort_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_BYTE:
		return value->b;
	case J_SHORT:
		return value->s;
	case J_CHAR:
	case J_INT:
	case J_LONG:
	case J_BOOLEAN:
	case J_DOUBLE:
	case J_FLOAT:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}

static jdouble to_jdouble_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_BYTE:
		return value->b;
	case J_SHORT:
		return value->s;
	case J_CHAR:
		return value->c;
	case J_INT:
		return value->i;
	case J_DOUBLE:
		return value->d;
	case J_FLOAT:
		return value->f;
	case J_LONG:
		return value->j;
	case J_BOOLEAN:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}

static jfloat to_jfloat_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_BYTE:
		return value->b;
	case J_SHORT:
		return value->s;
	case J_CHAR:
		return value->c;
	case J_FLOAT:
		return value->f;
	case J_INT:
		return value->i;
	case J_LONG:
		return value->j;
	case J_BOOLEAN:
	case J_DOUBLE:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}

static jchar to_jchar_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_CHAR:
		return value->c;
	case J_BYTE:
	case J_SHORT:
	case J_INT:
	case J_LONG:
	case J_BOOLEAN:
	case J_DOUBLE:
	case J_FLOAT:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}

static jbyte to_jbyte_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_BYTE:
		return value->b;
	case J_SHORT:
	case J_CHAR:
	case J_INT:
	case J_LONG:
	case J_BOOLEAN:
	case J_DOUBLE:
	case J_FLOAT:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}

static jboolean to_jboolean_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_BOOLEAN:
		return value->z;
	case J_SHORT:
	case J_CHAR:
	case J_INT:
	case J_LONG:
	case J_BYTE:
	case J_DOUBLE:
	case J_FLOAT:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}


jlong java_lang_reflect_VMField_getLong(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jlong_value(value, type);
}

jint java_lang_reflect_VMField_getInt(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jint_value(value, type);
}

jshort java_lang_reflect_VMField_getShort(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jshort_value(value, type);
}

jfloat java_lang_reflect_VMField_getFloat(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jfloat_value(value, type);
}

jdouble java_lang_reflect_VMField_getDouble(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jdouble_value(value, type);
}

jbyte java_lang_reflect_VMField_getByte(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jbyte_value(value, type);
}

jchar java_lang_reflect_VMField_getChar(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jchar_value(value, type);
}

jboolean java_lang_reflect_VMField_getBoolean(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jboolean_value(value, type);
}

jint java_lang_reflect_VMField_getModifiersInternal(struct vm_object *this)
{
	struct vm_field *vmf;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	return vmf->field->access_flags;
}

void java_lang_reflect_VMField_set(struct vm_object *this, struct vm_object *o,
		      struct vm_object *value_obj)
{
	struct vm_field *vmf;

	if (!this) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return;
	}

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return;

	/*
	 * TODO: "If this Field enforces access control, your runtime
	 * context is evaluated, and you may have an
	 * IllegalAccessException if you could not access this field
	 * in similar compiled code". (java/lang/reflect/Field.java)
	 */

	enum vm_type type = vm_field_type(vmf);

	if (vm_field_is_static(vmf)) {
		object_to_jvalue(vmf->class->static_values + vmf->offset,
				     type, value_obj);
	} else {
		/*
		 * If o is null, you get a NullPointerException, and
		 * if it is incompatible with the declaring class of
		 * the field, you get an IllegalArgumentException.
		 */
		if (!o) {
			signal_new_exception(vm_java_lang_NullPointerException,
					     NULL);
			return;
		}

		if (o->class != vmf->class) {
			signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
			return;
		}

		object_to_jvalue(field_get_object_ptr(o, vmf->offset), type, value_obj);
	}
}

struct vm_object *java_lang_reflect_VMField_getType(struct vm_object *this)
{
	struct vm_field *vmf;

	if (!this) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return 0;
	}

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	struct vm_class *vmc = vm_type_to_class(vmf->class->classloader,
						&vmf->type_info);
	if (vm_class_ensure_init(vmc))
		return NULL;

	return vmc->object;
}

jobject java_lang_reflect_VMField_getAnnotation(jobject this, jobject annotation)
{
	struct vm_class *annotation_type;
	struct vm_field *vmf;
	unsigned int i;

	annotation_type = vm_object_to_vm_class(annotation);

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return rethrow_exception();

	if (!vmf->annotation_initialized)
		if (vm_field_init_annotation(vmf))
			return rethrow_exception();

	for (i = 0; i < vmf->nr_annotations; i++) {
		struct vm_annotation *vma = vmf->annotations[i];
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

jobject java_lang_reflect_VMField_getDeclaredAnnotations(jobject klass)
{
	struct vm_object *result;
	struct vm_field *vmf;
	unsigned int i;

	vmf = vm_object_to_vm_field(klass);
	if (!vmf)
		return rethrow_exception();

	if (!vmf->annotation_initialized)
		if (vm_field_init_annotation(vmf))
			return rethrow_exception();

	result = vm_object_alloc_array(vm_array_of_java_lang_annotation_Annotation, vmf->nr_annotations);
	if (!result)
		return rethrow_exception();

	for (i = 0; i < vmf->nr_annotations; i++) {
		struct vm_annotation *vma = vmf->annotations[i];
		struct vm_object *annotation;

		annotation = vm_annotation_to_object(vma);
		if (!annotation)
			return rethrow_exception();

		array_set_field_object(result, i, annotation);
	}

	return result;
}
