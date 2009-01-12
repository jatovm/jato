/*
 * Copyright (c) 2009  Pekka Enberg
 * 
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */
#include <vm/bytecode.h>
#include <vm/bytecodes.h>

int8_t bytecode_read_s8(struct bytecode_buffer *buffer)
{
	return (int8_t) bytecode_read_u8(buffer);
}

int16_t bytecode_read_s16(struct bytecode_buffer *buffer)
{
	return (int16_t) bytecode_read_u16(buffer);
}

int32_t bytecode_read_s32(struct bytecode_buffer *buffer)
{
	return (int32_t) bytecode_read_u32(buffer);
}

uint8_t bytecode_read_u8(struct bytecode_buffer *buffer)
{
	return buffer->buffer[buffer->pos++];
}

uint16_t bytecode_read_u16(struct bytecode_buffer *buffer)
{
	uint16_t result;

	result  = (uint16_t) bytecode_read_u8(buffer) << 8;
	result |= (uint16_t) bytecode_read_u8(buffer);

	return result;
}

uint32_t bytecode_read_u32(struct bytecode_buffer *buffer)
{
	uint32_t result;

	result  = (uint32_t) bytecode_read_u8(buffer) << 24;
	result |= (uint32_t) bytecode_read_u8(buffer) << 16;
	result |= (uint32_t) bytecode_read_u8(buffer) << 8;
	result |= (uint32_t) bytecode_read_u8(buffer);

	return result;
}

int32_t bytecode_read_branch_target(unsigned char opc,
				    struct bytecode_buffer *buffer)
{
	if (bc_is_wide(opc))
		return bytecode_read_s32(buffer);

	return bytecode_read_s16(buffer);
}
