/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file contains helper functions for writing unit tests.
 */

#include <libharness.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <vm/backtrace.h>

static unsigned long nr_asserts;
static unsigned long nr_failed;

void print_test_suite_results(void)
{
	if (nr_failed)
		printf("FAILED (%lu assertions, %lu failed)\n", nr_asserts,
		       nr_failed);
	else
		printf("OK (%lu assertions)\n", nr_asserts);
}

void fail(const char *file, int line, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	printf("Assertion failed at %s:%d: ", file, line);
	vprintf(fmt, args);
	printf("\n");
	va_end(args);

	print_trace();
	nr_failed++;
}

void __assert_true(const char *file, int line, bool condition)
{
	nr_asserts++;
	if (!condition)
		fail(file, line, "Expected true, but was false");
}

void __assert_false(const char *file, int line, bool condition)
{
	nr_asserts++;
	if (condition)
		fail(file, line, "Expected false, but was true");
}

void __assert_not_null(const char *file, int line, void *ptr)
{
	nr_asserts++;
	if (!ptr)
		fail(file, line, "Expected not null, but was null");
}

void __assert_int_equals(const char *file, int line, long long expected,
			 long long actual)
{
	nr_asserts++;
	if (expected != actual)
		fail(file, line, "Expected %lld, but was %lld\n", expected,
		     actual);
}

void __assert_float_equals(const char *file, int line, long double expected,
			   long double actual, long double delta)
{
	nr_asserts++;
	if (fabs(expected - actual) > delta)
		fail(file, line, "Expected %i, but was %i\n", expected, actual);
}

void __assert_ptr_equals(const char *file, int line, void *expected,
			 void *actual)
{
	nr_asserts++;
	if (expected != actual)
		fail(file, line, "Expected %p, but was %p\n", expected, actual);
}

static void format_mem(char *str, unsigned long str_size, const void *buffer,
		       unsigned long buffer_size)
{
	int i = 0;
	const unsigned char *current = buffer;

	while (str_size && buffer_size) {
		i += sprintf(str+i, "%02x ", *current++);
		str_size -= i;
		buffer_size--;
	}
}

void __assert_mem_equals(const char *file, int line, const void *expected,
			 const void *actual, unsigned long size)
{
	nr_asserts++;
	if (memcmp(expected, actual, size)) {
		char expected_str[256];
		char actual_str[256];
		format_mem(expected_str, 256, expected, size);
		format_mem(actual_str, 256, actual, size);
		fail(file, line, "Expected %s, but was %s.\n", expected_str, actual_str);
	}
}

void __assert_str_equals(const char *file, int line, const char *expected,
			 const char *actual)
{
	nr_asserts++;
	if (strcmp(expected, actual))
		fail(file, line, "Expected %s, but was %s\n", expected, actual);
}
