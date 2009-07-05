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

#ifndef CAFEBABE__LINE_NUMBER_TABLE_ATTRIBUTE_H
#define CAFEBABE__LINE_NUMBER_TABLE_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

struct cafebabe_line_number_table_entry {
	uint16_t start_pc;
	uint16_t line_number;
};

/**
 * The LineNumberTable attribute.
 *
 * See also section 4.7.8 of The Java Virtual Machine Specification.
 */
struct cafebabe_line_number_table_attribute {
	uint16_t line_number_table_length;
	struct cafebabe_line_number_table_entry *line_number_table;
};

int cafebabe_line_number_table_attribute_init(struct cafebabe_line_number_table_attribute *a, struct cafebabe_stream *s);
void cafebabe_line_number_table_attribute_deinit(struct cafebabe_line_number_table_attribute *a);
int cafebabe_read_line_number_table_attribute(
	const struct cafebabe_class *class,
	const struct cafebabe_attribute_array *attributes,
	struct cafebabe_line_number_table_attribute *line_number_table_attrib);

#endif
