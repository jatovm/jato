/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include "lib/string.h"

#include <libharness.h>
#include <string.h>

static void assert_string_equals(const char *expected_value,
				 unsigned long expected_len,
				 struct string *str)
{
	assert_str_equals(expected_value, str->value);
	assert_int_equals(expected_len, str->length);
	assert_int_equals(expected_len, strlen(str->value));
}

void test_should_be_zero_length_initially(void)
{
	struct string *str = alloc_str();

	assert_string_equals("", 0, str);

	free_str(str);
}

void test_should_print_formatted_string_to_value(void)
{
	struct string *str = alloc_str();

	str_printf(str, "%s, %s!", "Hello", "World");
	assert_string_equals("Hello, World!", 13, str);

	free_str(str);
}

void test_should_resize_string_if_output_exceeds_capacity(void)
{
	struct string *str = alloc_str();

	str_resize(str, 5);
	str_printf(str, "0123456789");

	assert_string_equals("0123456789", 10, str);
	assert_true(str->capacity > 10);

	free_str(str);
}

void test_should_append_formatted_string_to_value(void)
{
	struct string *str = alloc_str();

	str_append(str, "%s, ", "Hello");
	assert_string_equals("Hello, ", 7, str);

	str_append(str, "%s!", "World");
	assert_string_equals("Hello, World!", 13, str);
	
	free_str(str);
}
