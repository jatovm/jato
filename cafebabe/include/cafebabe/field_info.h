/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
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

#ifndef CAFEBABE__FIELD_INFO_H
#define CAFEBABE__FIELD_INFO_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"

struct cafebabe_attribute_info;
struct cafebabe_stream;

#define CAFEBABE_FIELD_ACC_PUBLIC		0x0001
#define CAFEBABE_FIELD_ACC_PRIVATE		0x0002
#define CAFEBABE_FIELD_ACC_PROTECTED		0x0004
#define CAFEBABE_FIELD_ACC_STATIC		0x0008
#define CAFEBABE_FIELD_ACC_FINAL		0x0010
#define CAFEBABE_FIELD_ACC_VOLATILE		0x0040
#define CAFEBABE_FIELD_ACC_TRANSIENT		0x0080

/**
 * A java class field.
 *
 * See also section 4.5 of The Java Virtual Machine Specification.
 */
struct cafebabe_field_info {
	uint16_t access_flags;
	uint16_t name_index;
	uint16_t descriptor_index;
	struct cafebabe_attribute_array attributes;
};

int cafebabe_field_info_init(struct cafebabe_field_info *fi,
	struct cafebabe_stream *s);
void cafebabe_field_info_deinit(struct cafebabe_field_info *fi);

#endif
