#ifndef __LIBHARNESS_H
#define __LIBHARNESS_H

#include <stdbool.h>

void print_test_suite_results(void);

#define assert_true(condition) __assert_true(__FILE__, __LINE__, (condition))

#define assert_false(condition) \
	__assert_false(__FILE__, __LINE__, (condition))

#define assert_not_null(ptr) \
	__assert_not_null(__FILE__, __LINE__, (ptr))

#define assert_int_equals(expected, actual) \
	__assert_int_equals(__FILE__, __LINE__, (expected), (actual))

#define assert_float_equals(expected, actual, delta) \
	__assert_float_equals(__FILE__, __LINE__, (expected), (actual), (delta))

#define assert_ptr_equals(expected, actual) \
	__assert_ptr_equals(__FILE__, __LINE__, (expected), (actual))

#define assert_mem_equals(expected, actual, size) \
	__assert_mem_equals(__FILE__, __LINE__, (expected), (actual), (size))

#define assert_str_equals(expected, actual) \
	__assert_str_equals(__FILE__, __LINE__, (expected), (actual))

void fail(const char *, int, const char *, ...);
void __assert_true(const char *, int, bool);
void __assert_false(const char *, int, bool);
void __assert_not_null(const char *, int, void *);
void __assert_int_equals(const char *, int, long long, long long);
void __assert_float_equals(const char *, int, long double, long double, long double);
void __assert_ptr_equals(const char *, int, void *, void *);
void __assert_mem_equals(const char *, int, const void *, const void *, unsigned long);
void __assert_str_equals(const char *, int, const char *, const char *);

#endif
