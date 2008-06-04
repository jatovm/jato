#pragma once

#ifndef CAFEBABE__ATTRIBUTE_INFO_H
#define CAFEBABE__ATTRIBUTE_INFO_H

#include <stdint.h>

struct cafebabe_stream;

struct cafebabe_attribute_info {
	uint16_t attribute_name_index;
	uint32_t attribute_length;
	uint8_t *info;
};

int cafebabe_attribute_info_init(struct cafebabe_attribute_info *a,
	struct cafebabe_stream *s);
void cafebabe_attribute_info_deinit(struct cafebabe_attribute_info *a);

#endif
