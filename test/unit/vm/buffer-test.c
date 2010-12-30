/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include "lib/buffer.h"
#include <libharness.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

static int    nr_expand_called;
static size_t expand_size;
static int    nr_free_called;

static int stub_expand(struct buffer *buf)
{
	size_t size;

	nr_expand_called++;

	size = buf->size + 1;
	expand_size = size;

	buf->buf = realloc(buf->buf, size);
	if (!buf->buf)
		return -ENOMEM;

	buf->size = size;
	return 0;
}

static void stub_free(struct buffer *buf)
{
	nr_free_called++;
	free(buf->buf);
}

static struct buffer_operations stub_ops = {
	.expand = stub_expand,
	.free   = stub_free,
};

static void setup(void)
{
	expand_size      = ~0UL;
	nr_expand_called = 0;
	nr_free_called   = 0;
}

void test_should_init_buffer_as_zero_length(void)
{
	struct buffer *buffer;

	setup();

	assert_int_equals(0, nr_expand_called);
	buffer = __alloc_buffer(&stub_ops);
	assert_int_equals(0, nr_expand_called);

	assert_int_equals(0, buffer->size);

	free_buffer(buffer);
}

void test_should_free_buffer_with_ops(void)
{
	struct buffer *buffer;

	setup();

	buffer = __alloc_buffer(&stub_ops);

	assert_int_equals(0, nr_free_called);
	free_buffer(buffer);
	assert_int_equals(1, nr_free_called);
}

void test_should_expand_upon_append(void)
{
	struct buffer *buf;

	setup();

	buf = __alloc_buffer(&stub_ops);

	append_buffer(buf, 'A');
	assert_int_equals(1, nr_expand_called);
	append_buffer(buf, 'B');
	assert_int_equals(2, nr_expand_called);
	append_buffer(buf, 'C');
	assert_int_equals(3, nr_expand_called);
	append_buffer(buf, 'D');
	assert_int_equals(4, nr_expand_called);

	assert_mem_equals(buf->buf, "ABCD", buf->size);

	free_buffer(buf);
}
