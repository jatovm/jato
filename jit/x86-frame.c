/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for managing the native IA-32 function stack
 * frame.
 */

#include <jit/expression.h>
#include <jit/x86-frame.h>

#define LOCAL_START (2 * sizeof(unsigned long))

unsigned long frame_local_offset(struct expression *local)
{
	return (local->local_index * sizeof(unsigned long)) + LOCAL_START;
}
