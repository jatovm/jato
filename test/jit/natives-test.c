/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <vm/natives.h>

static void vm_class_is_instance(void)
{
}

static void vm_class_is_assignable_from(void)
{
}

static struct vm_method vm_class[] = {
	{ "isInstance",		vm_class_is_instance },
	{ "isAssignableFrom",	vm_class_is_assignable_from },
	{ NULL, 		NULL },
};

static void vm_object_get_class(void)
{
}

static struct vm_method vm_object[] = {
	{ "getClass",		vm_object_get_class },
	{ NULL,			NULL },
};

static struct vm_class native_methods[] = {
	{ "java/lang/VMClass",  vm_class  },
	{ "java/lang/VMObject", vm_object },
	{ NULL,			NULL },
};

static void assert_method_ptr_for(void *expected_ptr, const char *class_name, const char *method_name)
{
	assert_ptr_equals(expected_ptr, vm_lookup_native(native_methods, class_name, method_name));
}

void test_should_find_function_by_class_and_method_name(void)
{
	assert_method_ptr_for(vm_class_is_instance, "java/lang/VMClass", "isInstance");
	assert_method_ptr_for(vm_class_is_assignable_from, "java/lang/VMClass", "isAssignableFrom");
	assert_method_ptr_for(vm_object_get_class, "java/lang/VMObject", "getClass");
}

void test_should_fail_if_class_name_is_not_found(void)
{
	assert_method_ptr_for(NULL, "java/lang/VMUnknown", "isInstance");
}

void test_should_fail_if_method_name_is_not_found(void)
{
	assert_method_ptr_for(NULL, "java/lang/VMClass", "unknown");
}
