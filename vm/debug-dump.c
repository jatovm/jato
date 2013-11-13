/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "vm/debug-dump.h"

#include <stdio.h>

void print_buffer(unsigned char *buffer, unsigned long size)
{
	unsigned long i, col;

	col = 0;
	for (i = 0; i < size; i++) {
		printf("%02x ", buffer[i]);
		if (col++ == 15) {
			printf("\n");
			col = 0;
		}
	}
	printf("\n");
}
