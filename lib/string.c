/*
 * Copyright (c) 2006, 2011 Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/string.h"

#include "lib/hash-map.h"

#include "vm/die.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define INITIAL_CAPACITY 100

static struct hash_map		*literals;
static pthread_mutex_t		literals_mutex = PTHREAD_MUTEX_INITIALIZER;

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

struct string *string_from_cstr(char *s)
{
	struct string *str = malloc(sizeof *str);

	if (!str)
		return NULL;

	str->length = str->capacity = strlen(s);

	str->value = s;

	return str;
}

struct string *string_from_cstr_dup(const char *s)
{
	char *dup = strdup(s);

	if (!dup)
		return NULL;

	return string_from_cstr(dup);
}

struct string *string_intern_cstr(const char *s)
{
        struct string *result;

        pthread_mutex_lock(&literals_mutex);

        if (!hash_map_get(literals, s, (void **) &result))
		goto out;

        result = string_from_cstr_dup(s);
        if (!result)
                goto out;

        if (hash_map_put(literals, result->value, result)) {
		free_str(result);
		result = NULL;
	}
 out:
        pthread_mutex_unlock(&literals_mutex);

        return result;
}

void init_string_intern(void)
{
	literals = alloc_hash_map(&string_key);
	if (!literals)
		die("Unable to initialize string literal hash map");
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

static unsigned long str_remaining(struct string *str, int offset)
{
	return str->capacity - offset;
}

static int str_vprintf(struct string *str, unsigned long offset,
		       const char *fmt, va_list args)
{
	unsigned long size;
	va_list tmp_args;
	int nr, err = 0;

  retry:
	size = str_remaining(str, offset);
	va_copy(tmp_args, args);
	nr = vsnprintf(str->value + offset, size, fmt, tmp_args);
	va_end(tmp_args);

	if ((unsigned long)nr >= size) {
		err = str_resize(str, str->capacity * 2);
		if (err)
			goto out;
	
		goto retry;
	}
	str->length = offset + nr;

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
