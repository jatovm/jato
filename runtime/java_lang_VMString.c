/*
 * Copyright (c) 2011 Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "runtime/java_lang_VMString.h"

#include "vm/string.h"

jobject java_lang_VMString_intern(jobject str)
{
	return vm_string_intern(str);
}
