#include <stdint.h>
#include <errno.h>
#include <stdio.h>

#include "vm/class.h"
#include "vm/field.h"
#include "vm/method.h"

int vm_class_init(struct vm_class *vmc)
{
	NOT_IMPLEMENTED;
	return 0;
}

int vm_class_ensure_object(struct vm_class *vmc)
{
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

int vm_class_resolve_method(const struct vm_class *vmc, uint16_t i,
			struct vm_class **r_vmc, char **r_name, char **r_type)
{
	return -EINVAL;
}

int vm_class_resolve_interface_method(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type)
{
	return -EINVAL;
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

struct vm_method *vm_class_get_method(const struct vm_class *vmc,
	const char *name, const char *type)
{
	return NULL;
}
