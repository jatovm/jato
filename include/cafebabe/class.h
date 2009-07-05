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

#include "cafebabe/attribute_array.h"

struct cafebabe_attribute_info;
struct cafebabe_constant_info_utf8;
struct cafebabe_constant_info_class;
struct cafebabe_constant_info_field_ref;
struct cafebabe_constant_info_method_ref;
struct cafebabe_constant_info_interface_method_ref;
struct cafebabe_constant_info_name_and_type;
struct cafebabe_constant_pool;
struct cafebabe_field_info;
struct cafebabe_method_info;
struct cafebabe_stream;

#define CAFEBABE_CLASS_ACC_PUBLIC	0x0001
#define CAFEBABE_CLASS_ACC_FINAL	0x0010
#define CAFEBABE_CLASS_ACC_SUPER	0x0020
#define CAFEBABE_CLASS_ACC_INTERFACE	0x0200
#define CAFEBABE_CLASS_ACC_ABSTRACT	0x0400

/**
 * A java class file.
 *
 * See also section 4.1 of The Java Virtual Machine Specification.
 */
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
	struct cafebabe_attribute_array attributes;
};

int cafebabe_class_init(struct cafebabe_class *c,
	struct cafebabe_stream *s);
void cafebabe_class_deinit(struct cafebabe_class *c);

int cafebabe_class_constant_index_invalid(const struct cafebabe_class *c,
	uint16_t i);
int cafebabe_class_constant_get_utf8(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_utf8 **r);
int cafebabe_class_constant_get_class(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_class **r);
int cafebabe_class_constant_get_field_ref(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_field_ref **r);
int cafebabe_class_constant_get_method_ref(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_method_ref **r);
int cafebabe_class_constant_get_interface_method_ref(
	const struct cafebabe_class *c, uint16_t i,
	const struct cafebabe_constant_info_interface_method_ref **r);
int cafebabe_class_constant_get_name_and_type(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_name_and_type **r);

int cafebabe_class_get_field(const struct cafebabe_class *c,
	const char *name, const char *descriptor,
	unsigned int *r);
int cafebabe_class_get_method(const struct cafebabe_class *c,
	const char *name, const char *descriptor,
	unsigned int *r);
char *cafebabe_class_get_source_file_name(const struct cafebabe_class *class);

#endif
