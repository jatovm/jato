/*
 * Copyright (c) 2009  Pekka Enberg
 * 
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "jit/perf-map.h"
#include "vm/die.h"

#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

static pthread_mutex_t perf_mutex = PTHREAD_MUTEX_INITIALIZER; 
static FILE *perf_file;

void perf_map_open(void)
{
	char filename[32];
	pid_t pid;

	pid = getpid();
	sprintf(filename, "/tmp/perf-%d.map", pid);

	perf_file = fopen(filename, "w");
	if (!perf_file)
		die("fopen");
}

void perf_map_append(const char *symbol, unsigned long addr, unsigned long size)
{
	if (perf_file == NULL)
		return;

	pthread_mutex_lock(&perf_mutex);
	fprintf(perf_file, "%lx %lx %s\n", addr, size, symbol);
	pthread_mutex_unlock(&perf_mutex);
}
