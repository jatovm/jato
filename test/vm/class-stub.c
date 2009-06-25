#include <stdint.h>

#include <vm/class.h>
#include <vm/field.h>
#include <vm/method.h>

struct vm_class *vm_class_resolve_class(struct vm_class *vmc, uint16_t i)
{
	NOT_IMPLEMENTED;
	return NULL;
}

struct vm_field *vm_class_resolve_field_recursive(struct vm_class *vmc,
	uint16_t i)
{
	NOT_IMPLEMENTED;
	return NULL;
}

struct vm_method *vm_class_resolve_method_recursive(struct vm_class *vmc,
	uint16_t i)
{
	/* NOTE: For the tests, instead of going through the constant pool,
	 * we assume that "methodref" is actually an index into the class'
	 * own method table. */
	return &vmc->methods[i];
}

bool vm_class_is_assignable_from(struct vm_class *vmc, struct vm_class *from)
{
	return false;
}
