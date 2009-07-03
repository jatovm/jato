#include <stdint.h>
#include <stdio.h>

#include <vm/class.h>
#include <vm/field.h>
#include <vm/method.h>

int vm_class_init(struct vm_class *vmc)
{
	NOT_IMPLEMENTED;
	return 0;
}

struct vm_class *vm_class_resolve_class(const struct vm_class *vmc, uint16_t i)
{
	NOT_IMPLEMENTED;
	return NULL;
}

struct vm_field *vm_class_resolve_field_recursive(const struct vm_class *vmc,
	uint16_t i)
{
	NOT_IMPLEMENTED;
	return NULL;
}

struct vm_method *vm_class_resolve_method_recursive(const struct vm_class *vmc,
	uint16_t i)
{
	/* NOTE: For the tests, instead of going through the constant pool,
	 * we assume that "methodref" is actually an index into the class'
	 * own method table. */
	return &vmc->methods[i];
}

struct vm_method *vm_class_resolve_interface_method_recursive(
	const struct vm_class *vmc, uint16_t i)
{
	/* See vm_class_resolve_method_recursive() */
	return &vmc->methods[i];
}

bool vm_class_is_assignable_from(const struct vm_class *vmc,
	const struct vm_class *from)
{
	return false;
}
