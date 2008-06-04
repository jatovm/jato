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

#ifndef CAFEBABE__CLASS_H
#define CAFEBABE__CLASS_H

#include <stdint.h>

struct cafebabe_attribute_info;
struct cafebabe_constant_pool;
struct cafebabe_field_info;
struct cafebabe_method_info;
struct cafebabe_stream;

struct cafebabe_class {
	uint32_t magic;
	uint16_t minor_version;
	uint16_t major_version;
	uint16_t constant_pool_count;
	struct cafebabe_constant_pool *constant_pool;
	uint16_t access_flags;
	uint16_t this_class;
	uint16_t super_class;
	uint16_t interfaces_count;
	uint16_t *interfaces;
	uint16_t fields_count;
	struct cafebabe_field_info *fields;
	uint16_t methods_count;
	struct cafebabe_method_info *methods;
	uint16_t attributes_count;
	struct cafebabe_attribute_info *attributes;
};

int cafebabe_class_init(struct cafebabe_class *c,
	struct cafebabe_stream *s);
void cafebabe_class_deinit(struct cafebabe_class *c);

#endif
