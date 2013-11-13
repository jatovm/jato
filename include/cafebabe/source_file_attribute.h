/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2009  Tomasz Grabiec <tgrabiec@gmail.com>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__SOURCEFILE_ATTRIBUTE_H
#define CAFEBABE__SOURCEFILE_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

/**
 * The SourceFile attribute.
 *
 * See also section 4.7.7 of The Java Virtual Machine Specification.
 */
struct cafebabe_source_file_attribute {
	uint16_t sourcefile_index;
};

int cafebabe_source_file_attribute_init(
	struct cafebabe_source_file_attribute *a, struct cafebabe_stream *s);
void cafebabe_source_file_attribute_deinit(
	struct cafebabe_source_file_attribute *a);

#endif
