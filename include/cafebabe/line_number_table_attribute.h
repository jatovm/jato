/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2009  Tomasz Grabiec <tgrabiec@gmail.com>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

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
