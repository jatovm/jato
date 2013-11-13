/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "jit/compiler.h"
#include "lib/string.h"
#include "vm/thread.h"
#include "vm/trace.h"
#include "vm/die.h"

static pthread_mutex_t trace_mutex = PTHREAD_MUTEX_INITIALIZER;

static void setup_trace_buffer(void)
{
	if (vm_get_exec_env()->trace_buffer)
		return;

	vm_get_exec_env()->trace_buffer = alloc_str();
	if (!vm_get_exec_env()->trace_buffer)
		error("out of memory");
}

int trace_printf(const char *fmt, ...)
{
	va_list args;
	int err;

	setup_trace_buffer();

	va_start(args, fmt);
	err = str_vappend(vm_get_exec_env()->trace_buffer, fmt, args);
	va_end(args);

	return err;
}

void trace_flush(void)
{
	struct vm_thread *self;
	char *thread_name;
	char *line;
	char *next;

	setup_trace_buffer();

	self = vm_thread_self();
	if (self)
		thread_name = vm_thread_get_name(self);
	else
		thread_name = strdup("unknown");

	pthread_mutex_lock(&trace_mutex);

	line = vm_get_exec_env()->trace_buffer->value;
	next = index(line, '\n');
	while (next) {
		*next = 0;

		fprintf(stderr, "[%s] %s\n", thread_name, line);

		line = next + 1;
		next = index(line, '\n');
	}

	/* Leave the rest of characters, which are not ended by '\n' */
	memmove(vm_get_exec_env()->trace_buffer->value, line, strlen(line) + 1);
	vm_get_exec_env()->trace_buffer->length = strlen(line);

	pthread_mutex_unlock(&trace_mutex);

	if (self)
		free(thread_name);
}
