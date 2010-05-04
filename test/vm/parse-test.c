/*
 * Copyright (C) 2010  Pekka Enberg
 */

#include "lib/parse.h"

#include "libharness.h"

#include <stdlib.h>

void test_parse_bytes(void)
{
	assert_int_equals(32, parse_long("32"));
}

void test_parse_kilobytes(void)
{
	assert_int_equals(64*1024, parse_long("64k"));
}

void test_parse_megabytes(void)
{
	assert_int_equals(512*1024*1024, parse_long("512m"));
}

void test_parse_gigabytes(void)
{
	assert_int_equals(1*1024*1024*1024, parse_long("1g"));
}
