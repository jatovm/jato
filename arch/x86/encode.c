/*
 * x86 instrunction encoding
 *
 * Copyright (C) 2006-2010 Pekka Enberg
 * Copyright (C) 2008-2009 Arthur Huillet
 * Copyright (C) 2009 Eduard - Gabriel Munteanu
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

#include "arch/encode.h"

#include "vm/die.h"

static uint8_t x86_register_numbers[] = {
	[MACH_REG_xAX]		= 0x00,
	[MACH_REG_xCX]		= 0x01,
	[MACH_REG_xDX]		= 0x02,
	[MACH_REG_xBX]		= 0x03,
	[MACH_REG_xSP]		= 0x04,
	[MACH_REG_xBP]		= 0x05,
	[MACH_REG_xSI]		= 0x06,
	[MACH_REG_xDI]		= 0x07,
#ifdef CONFIG_X86_64
	[MACH_REG_R8]		= 0x08,
	[MACH_REG_R9]		= 0x09,
	[MACH_REG_R10]		= 0x0A,
	[MACH_REG_R11]		= 0x0B,
	[MACH_REG_R12]		= 0x0C,
	[MACH_REG_R13]		= 0x0D,
	[MACH_REG_R14]		= 0x0E,
	[MACH_REG_R15]		= 0x0F,
#endif
	/* XMM registers */
	[MACH_REG_XMM0]		= 0x00,
	[MACH_REG_XMM1]		= 0x01,
	[MACH_REG_XMM2]		= 0x02,
	[MACH_REG_XMM3]		= 0x03,
	[MACH_REG_XMM4]		= 0x04,
	[MACH_REG_XMM5]		= 0x05,
	[MACH_REG_XMM6]		= 0x06,
	[MACH_REG_XMM7]		= 0x07,
#ifdef CONFIG_X86_64
	[MACH_REG_XMM8]		= 0x08,
	[MACH_REG_XMM9]		= 0x09,
	[MACH_REG_XMM10]	= 0x0A,
	[MACH_REG_XMM11]	= 0x0B,
	[MACH_REG_XMM12]	= 0x0C,
	[MACH_REG_XMM13]	= 0x0D,
	[MACH_REG_XMM14]	= 0x0E,
	[MACH_REG_XMM15]	= 0x0F,
#endif
};

/*
 *	x86_encode_reg: Encode register for an x86 instrunction.
 *	@reg: Register to encode.
 *
 *	This function returns register number in the format used by the ModR/M
 *	and SIB bytes of an x86 instruction.
 */
uint8_t x86_encode_reg(enum machine_reg reg)
{
	if (reg == MACH_REG_UNASSIGNED)
		die("unassigned register during code emission");

	if (reg < 0 || reg >= ARRAY_SIZE(x86_register_numbers))
		die("unknown register %d", reg);

	return x86_register_numbers[reg];
}
