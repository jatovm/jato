/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2009  Tomasz Grabiec <tgrabiec@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

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
