/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2011 Theo Dzierzbicki <theo.dz@gmail.com>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/attribute_info.h"
#include "cafebabe/stack_map_table_attribute.h"
#include "cafebabe/stream.h"

int cafebabe_stream_read_verification_type_info(struct cafebabe_stream *s, struct cafebabe_verification_type_info *info)
{
	uint8_t raw_tag;

	if (cafebabe_stream_read_uint8(s, &raw_tag))
		return 1;
	info->tag = raw_tag;

	switch(info->tag) {
	case CAFEBABE_VERIFICATION_TAG_OBJECT_VARIABLE_INFO:
		if (cafebabe_stream_read_uint16(s, &info->object.cpool_index))
			return 1;
		break;
	case CAFEBABE_VERIFICATION_TAG_UNINITIALIZED_VARIABLE_INFO:
		if (cafebabe_stream_read_uint16(s, &info->uninitialized.offset))
			return 1;
	default:
		break;
	}

	return 0;
}

int cafebabe_stack_map_table_attribute_init(
	struct cafebabe_stack_map_table_attribute *a,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->stack_map_frame_length))
		goto out;

	a->stack_map_frame = cafebabe_stream_malloc(s,
		sizeof(*a->stack_map_frame) * a->stack_map_frame_length);
	if (!a->stack_map_frame)
		goto out;

	for (uint16_t i = 0; i < a->stack_map_frame_length; i++) {
		struct cafebabe_stack_map_frame_entry *e
			= &a->stack_map_frame[i];
		uint8_t raw_tag;

		if (cafebabe_stream_read_uint8(s, &raw_tag))
			goto out_stack_map_frame;

		/* SAME_FRAME */
		if (raw_tag < 64) {
			e->tag = CAFEBABE_STACK_MAP_TAG_SAME_FRAME;
			e->offset_delta = raw_tag;
		}

		/* SAME_LOCALS_1_STACK_ITEM_FRAME */
		else if (raw_tag < 128) {
			e->tag = CAFEBABE_STACK_MAP_TAG_SAME_LOCAlS_1_STACK_ITEM_FRAME;
			e->offset_delta = raw_tag - 64;

			if (cafebabe_stream_read_verification_type_info(s, &e->same_locals_1_stack_item_frame.stack[0]))
				goto out_stack_map_frame;
		}

		/* SAME_LOCALS_1_STACK_ITEM_FRAME_EXTENDED */
		else if (raw_tag == 247) {
			e->tag = CAFEBABE_STACK_MAP_TAG_SAME_LOCAlS_1_STACK_ITEM_FRAME;

			if (cafebabe_stream_read_uint16(s, &e->offset_delta))
				goto out_stack_map_frame;

			if (cafebabe_stream_read_verification_type_info(s, &e->same_locals_1_stack_item_frame.stack[0]))
				goto out_stack_map_frame;
		}

		/* CHOP_FRAME */
		else if (raw_tag >= 248 && raw_tag < 251) {
			e->tag = CAFEBABE_STACK_MAP_TAG_CHOP_FRAME;
			e->chop_frame.chopped = 251 - raw_tag;

			if (cafebabe_stream_read_uint16(s, &e->offset_delta))
				goto out_stack_map_frame;
		}

		/* SAME_FRAME_EXTENDED */
		else if (raw_tag == 251) {
			e->tag = CAFEBABE_STACK_MAP_TAG_SAME_FRAME;

			if (cafebabe_stream_read_uint16(s, &e->offset_delta))
				goto out_stack_map_frame;
		}

		/* APPEND_FRAME */
		else if (raw_tag >= 252 && raw_tag < 255) {
			e->tag = CAFEBABE_STACK_MAP_TAG_APPEND_FRAME;
			e->append_frame.nr_locals = raw_tag - 251;

			if (cafebabe_stream_read_uint16(s, &e->offset_delta))
				goto out_stack_map_frame;

			e->append_frame.locals = cafebabe_stream_malloc(s,
				sizeof(*e->append_frame.locals) * e->append_frame.nr_locals);
			if (!e->append_frame.locals)
				goto out_stack_map_frame;

			for (uint16_t j = 0; j < e->append_frame.nr_locals; j++) {
				if (cafebabe_stream_read_verification_type_info(s, &e->append_frame.locals[j])) {
					free(e->append_frame.locals);
					goto out_stack_map_frame;
				}
			}
		}

		/* FULL_FRAME */
		else if (raw_tag == 255) {
			e->tag = CAFEBABE_STACK_MAP_TAG_FULL_FRAME;

			if (cafebabe_stream_read_uint16(s, &e->offset_delta))
				goto out_stack_map_frame;

			if (cafebabe_stream_read_uint16(s, &e->full_frame.nr_locals))
				goto out_stack_map_frame;

			e->full_frame.locals = cafebabe_stream_malloc(s,
				sizeof(*e->full_frame.locals) * e->full_frame.nr_locals);
			if (!e->full_frame.locals)
				goto out_stack_map_frame;

			for (uint16_t j = 0; j < e->full_frame.nr_locals; j++) {
				if (cafebabe_stream_read_verification_type_info(s, &e->full_frame.locals[j])) {
					free(e->full_frame.locals);
					goto out_stack_map_frame;
				}
			}

			if (cafebabe_stream_read_uint16(s, &e->full_frame.nr_stack_items))
				goto out_stack_map_frame;

			e->full_frame.stack = cafebabe_stream_malloc(s,
				sizeof(*e->full_frame.stack) * e->full_frame.nr_stack_items);
			if (!e->full_frame.stack)
				goto out_stack_map_frame;

			for (uint16_t j = 0; j < e->full_frame.nr_stack_items; j++) {
				if (cafebabe_stream_read_verification_type_info(s, &e->full_frame.stack[j])) {
					free(e->full_frame.locals);
					free(e->full_frame.stack);
					goto out_stack_map_frame;
				}
			}
		}

		/* Unknown frame type */
		else {
			s->cafebabe_errno = CAFEBABE_ERROR_INVALID_STACK_FRAME_TAG;
			goto out_stack_map_frame;
		}
	}

	if (!cafebabe_stream_eof(s)) {
		s->cafebabe_errno = CAFEBABE_ERROR_EXPECTED_EOF;
		goto out_stack_map_frame;
	}

	/* Success */
	return 0;

out_stack_map_frame:
	free(a->stack_map_frame);
out:
	return 1;
}

void cafebabe_stack_map_table_attribute_deinit(struct cafebabe_stack_map_table_attribute *a)
{
	free(a->stack_map_frame);
}

int cafebabe_read_stack_map_table_attribute(
	const struct cafebabe_class *class,
	const struct cafebabe_attribute_array *attributes,
	struct cafebabe_stack_map_table_attribute *attrib)
{
	unsigned int stack_map_table_index = 0;
	if (cafebabe_attribute_array_get(attributes,
		"StackMapTable", class, &stack_map_table_index))
	{
		attrib->stack_map_frame_length = 0;
		attrib->stack_map_frame = NULL;
		return 0;
	}

	const struct cafebabe_attribute_info *attribute
		= &attributes->array[stack_map_table_index];

	struct cafebabe_stream stream;
	cafebabe_stream_open_buffer(&stream,
		attribute->info, attribute->attribute_length);

	if (cafebabe_stack_map_table_attribute_init(attrib, &stream))
		return -1;

	cafebabe_stream_close_buffer(&stream);
	return 0;
}
