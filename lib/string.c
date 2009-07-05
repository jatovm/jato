/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "vm/string.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 100

struct string *alloc_str(void)
{
	int err;
	struct string *str = malloc(sizeof *str);

	if (!str)
		return NULL;

	memset(str, 0, sizeof *str);

	err = str_resize(str, INITIAL_CAPACITY);
	if (err) {
		free(str);
		return NULL;
	}
	memset(str->value, 0, str->capacity);

	return str;
}

void free_str(struct string *str)
{
	free(str->value);
	free(str);
}

int str_resize(struct string *str, unsigned long capacity)
{
	char *buffer;

	buffer = realloc(str->value, capacity);
	if (!buffer)
		return -ENOMEM;

	str->value = buffer;
	str->capacity = capacity;

	return 0;
}

static unsigned long str_remaining(struct string *str)
{
	return str->capacity - str->length;
}

static int str_vprintf(struct string *str, unsigned long offset,
		       const char *fmt, va_list args)
{
	unsigned long size;
	va_list tmp_args;
	int nr, err = 0;

  retry:
	size = str_remaining(str);
	va_copy(tmp_args, args);
	nr = vsnprintf(str->value + offset, size, fmt, tmp_args);
	va_end(tmp_args);

	if ((unsigned long)nr >= size) {
		int err;
	
		err = str_resize(str, str->capacity * 2);
		if (err)
			goto out;
	
		str->length = offset;
		goto retry;
	}
	str->length += nr;

  out:
	return err;
}

int str_printf(struct string *str, const char *fmt, ...)
{
	int err;
	va_list args;

	va_start(args, fmt);
	err = str_vprintf(str, 0, fmt, args);
	va_end(args);
	return err;
}

int str_vappend(struct string *str, const char *fmt, va_list args)
{
	return str_vprintf(str, str->length, fmt, args);
}

int str_append(struct string *str, const char *fmt, ...)
{
	int err;
	va_list args;

	va_start(args, fmt);
	err = str_vappend(str, fmt, args);
	va_end(args);
	return err;
}
