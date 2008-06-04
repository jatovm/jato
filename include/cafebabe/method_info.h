#pragma once

#ifndef CAFEBABE__METHOD_INFO_H
#define CAFEBABE__METHOD_INFO_H

#include <stdint.h>

struct cafebabe_attribute_info;
struct cafebabe_stream;

struct cafebabe_method_info {
	uint16_t access_flags;
	uint16_t name_index;
	uint16_t descriptor_index;
	uint16_t attributes_count;
	struct cafebabe_attribute_info *attributes;
};

int cafebabe_method_info_init(struct cafebabe_method_info *m,
	struct cafebabe_stream *s);
void cafebabe_method_info_deinit(struct cafebabe_method_info *m);

#endif
