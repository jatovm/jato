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

static void vm_object_get_class(void)
{
}

static void test_setup(void)
{
	vm_register_native("java/lang/VMClass", "isInstance", vm_class_is_instance);
	vm_register_native("java/lang/VMClass", "isAssignableFrom", vm_class_is_assignable_from);
	vm_register_native("java/lang/VMObject", "getClass", vm_object_get_class);
}

static void test_teardown(void)
{
	vm_unregister_natives();
}

static void assert_method_ptr_for(void *expected_ptr, const char *class_name, const char *method_name)
{
	assert_ptr_equals(expected_ptr, vm_lookup_native(class_name, method_name));
}

void test_should_find_function_by_class_and_method_name(void)
{
	test_setup();

	assert_method_ptr_for(vm_class_is_instance, "java/lang/VMClass", "isInstance");
	assert_method_ptr_for(vm_class_is_assignable_from, "java/lang/VMClass", "isAssignableFrom");
	assert_method_ptr_for(vm_object_get_class, "java/lang/VMObject", "getClass");

	test_teardown();
}

void test_should_fail_if_class_name_is_not_found(void)
{
	test_setup();

	assert_method_ptr_for(NULL, "java/lang/VMUnknown", "isInstance");

	test_teardown();
}

void test_should_fail_if_method_name_is_not_found(void)
{
	test_setup();

	assert_method_ptr_for(NULL, "java/lang/VMClass", "unknown");

	test_teardown();
}
