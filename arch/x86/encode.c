/*
 * x86 instruction encoding
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

#include "arch/instruction.h"

#include "lib/buffer.h"

#include "vm/die.h"

#include <stdint.h>

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

enum x86_insn_flags {
	MOD_RM			= (1U << 8),
	DIR_REVERSED		= (1U << 9),

	/* Operand sizes */
#define WIDTH_BYTE			0UL

	WIDTH_FULL		= (1U << 10),	/* 16 bits or 32 bits */
	WIDTH_MASK		= WIDTH_BYTE|WIDTH_FULL,

	/* Source operand */
	SRC_NONE		= (1U << 11),

	SRC_IMM			= (1U << 12),
	SRC_IMM8		= (1U << 13),
	IMM_MASK		= SRC_IMM|SRC_IMM8,

#define SRC_REL			SRC_IMM
	REL_MASK		= SRC_IMM,

	SRC_REG			= (1U << 14),
	SRC_ACC			= (1U << 15),
	SRC_MEM			= (1U << 16),
	SRC_MEM_DISP_BYTE	= (1U << 17),
	SRC_MEM_DISP_FULL	= (1U << 18),
	SRC_MASK		= SRC_NONE|IMM_MASK|REL_MASK|SRC_REG|SRC_ACC|SRC_MEM|SRC_MEM_DISP_BYTE|SRC_MEM_DISP_FULL,

	/* Destination operand */
#define DST_NONE			0UL

	DST_REG			= (1U << 19),
	DST_ACC			= (1U << 20),	/* AL/AX */
	DST_MEM			= (1U << 21),
	DST_MEM_DISP_BYTE	= (1U << 22),	/* 8 bits */
	DST_MEM_DISP_FULL	= (1U << 23),	/* 16 bits or 32 bits */
	DST_MASK		= DST_REG|DST_ACC|DST_MEM|DST_MEM_DISP_BYTE|DST_MEM_DISP_FULL,

	MEM_DISP_MASK		= SRC_MEM_DISP_BYTE|SRC_MEM_DISP_FULL|DST_MEM_DISP_BYTE|DST_MEM_DISP_FULL,

	ESCAPE_OPC_BYTE		= (1U << 24),	/* Escape opcode byte */
	REPNE_PREFIX		= (1U << 25),	/* REPNE/REPNZ or SSE prefix */
	REPE_PREFIX		= (1U << 26),	/* REP/REPE/REPZ or SSE prefix */
	OPC_EXT			= (1U << 27),	/* The reg field of ModR/M byte provides opcode extension */
};

/*
 *	Addressing modes
 */
enum x86_addmode {
	ADDMODE_ACC_MEM		= SRC_ACC|DST_MEM|DIR_REVERSED,	/* AL/AX -> memory */
	ADDMODE_ACC_REG		= SRC_ACC|DST_REG,		/* AL/AX -> reg */
	ADDMODE_IMM		= SRC_IMM,			/* immediate operand */
	ADDMODE_IMM8_RM		= SRC_IMM8|MOD_RM|DIR_REVERSED,	/* immediate -> register/memory */
	ADDMODE_IMM_ACC		= SRC_IMM|DST_ACC,		/* immediate -> AL/AX */
	ADDMODE_IMM_REG		= SRC_IMM|DST_REG|DIR_REVERSED,	/* immediate -> register */
	ADDMODE_IMPLIED		= SRC_NONE|DST_NONE,		/* no operands */
	ADDMODE_MEM_ACC		= SRC_ACC|DST_MEM,		/* memory -> AL/AX */
	ADDMODE_REG		= SRC_REG|DST_NONE,		/* register */
	ADDMODE_REG_REG		= SRC_REG|DST_REG|MOD_RM|DIR_REVERSED,	/* register -> register */
	ADDMODE_REG_RM		= SRC_REG|MOD_RM|DIR_REVERSED,	/* register -> register/memory */
	ADDMODE_REL		= SRC_REL|DST_NONE,		/* relative */
	ADDMODE_RM_REG		= DST_REG|MOD_RM,		/* register/memory -> register */
};

#define OPCODE(opc)		opc
#define OPCODE_MASK		0x000000ffUL
#define FLAGS_MASK		0x1fffff00UL

#define OPCODE_EXT_SHIFT	29
#define OPCODE_EXT(opc_ext)	((opc_ext << OPCODE_EXT_SHIFT) | OPC_EXT | MOD_RM)
#define OPCODE_EXT_MASK		(0xe0000000)

static uint32_t encode_table[NR_INSN_TYPES] = {
	[INSN_ADC_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(2)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_ADC_MEMBASE_REG]		= OPCODE(0x13) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_ADC_REG_REG]		= OPCODE(0x11) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_ADD_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(0)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_ADD_MEMBASE_REG]		= OPCODE(0x03) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_ADD_REG_REG]		= OPCODE(0x01) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_AND_MEMBASE_REG]		= OPCODE(0x23) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_AND_REG_REG]		= OPCODE(0x21) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_CMP_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(7)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_CMP_MEMBASE_REG]		= OPCODE(0x3b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_CMP_REG_REG]		= OPCODE(0x39) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_MOV_64_MEMBASE_XMM]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_RM_REG | WIDTH_FULL,
	[INSN_MOV_64_XMM_MEMBASE]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x11) | ADDMODE_REG_RM | WIDTH_FULL,
	[INSN_MOV_MEMBASE_REG]		= OPCODE(0x8b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_MOV_MEMBASE_XMM]		= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_RM_REG | WIDTH_FULL,
	[INSN_MOV_REG_MEMBASE]		= OPCODE(0x89) | ADDMODE_REG_RM  | WIDTH_FULL,
	[INSN_MOV_REG_REG]		= OPCODE(0x89) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_MOV_XMM_MEMBASE]		= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x11) | ADDMODE_REG_RM | WIDTH_FULL,
	[INSN_OR_MEMBASE_REG]		= OPCODE(0x0b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_OR_REG_REG]		= OPCODE(0x09) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_RET]			= OPCODE(0xc3) | ADDMODE_IMPLIED,
	[INSN_SBB_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(3)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_SBB_MEMBASE_REG]		= OPCODE(0x1b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_SBB_REG_REG]		= OPCODE(0x19) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_SUB_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(5)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_SUB_MEMBASE_REG]		= OPCODE(0x2b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_SUB_REG_REG]		= OPCODE(0x29) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_TEST_MEMBASE_REG]		= OPCODE(0x85) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_XOR_MEMBASE_REG]		= OPCODE(0x33) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_XOR_REG_REG]		= OPCODE(0x31) | ADDMODE_REG_REG | WIDTH_FULL,
};

static inline bool is_imm_8(long imm)
{
	return (imm >= -128) && (imm <= 127);
}

static inline void emit(struct buffer *buf, unsigned char c)
{
	int err;

	err = append_buffer(buf, c);

	if (err)
		die("out of buffer space");
}

static void emit_imm32(struct buffer *buf, int imm)
{
	union {
		int val;
		unsigned char b[4];
	} imm_buf;

	imm_buf.val = imm;
	emit(buf, imm_buf.b[0]);
	emit(buf, imm_buf.b[1]);
	emit(buf, imm_buf.b[2]);
	emit(buf, imm_buf.b[3]);
}

static void emit_imm(struct buffer *buf, long imm)
{
	if (is_imm_8(imm))
		emit(buf, imm);
	else
		emit_imm32(buf, imm);
}

static unsigned char encode_reg(struct use_position *reg)
{
	return x86_encode_reg(mach_reg(reg));
}

static inline uint32_t mod_src_encode(uint32_t flags)
{
	switch (flags & SRC_MASK) {
	case SRC_MEM:
		return 0x00;
	case SRC_MEM_DISP_BYTE:
		return 0x01;
	case SRC_MEM_DISP_FULL:
		return 0x02;
	case SRC_REG:
		return 0x03;
	default:
		break;
	}

	die("unrecognized flags %x", flags);
}

static inline uint32_t mod_dest_encode(uint32_t flags)
{
	switch (flags & DST_MASK) {
	case DST_MEM:
		return 0x00;
	case DST_MEM_DISP_BYTE:
		return 0x01;
	case DST_MEM_DISP_FULL:
		return 0x02;
	case DST_REG:
		return 0x03;
	default:
		break;
	}

	die("unrecognized flags %x", flags & DST_MASK);
}

static void insn_encode_sib(struct insn *self, struct buffer *buffer, uint32_t flags)
{
	uint8_t base;
	uint8_t sib;

	if (flags & DIR_REVERSED)
		base	= encode_reg(&self->dest.base_reg);
	else
		base	= encode_reg(&self->src.base_reg);

	sib		= x86_encode_sib(0x00, 0x04, base);

	emit(buffer, sib);
}

static bool insn_need_sib(struct insn *self, uint32_t flags)
{
	if (flags & OPC_EXT)
		return false;

	if (flags & DIR_REVERSED)
		return mach_reg(&self->dest.base_reg) == MACH_REG_xSP;

	return mach_reg(&self->src.base_reg) == MACH_REG_xSP;
}

static void insn_encode_mod_rm(struct insn *self, struct buffer *buffer, uint32_t flags, uint8_t opc_ext)
{
	uint8_t mod_rm, mod, reg_opcode, rm;
	bool need_sib;

	need_sib	= insn_need_sib(self, flags);

	if (flags & DIR_REVERSED)
		mod		= mod_dest_encode(flags);
	else
		mod		= mod_src_encode(flags);

	if (flags & OPC_EXT) {
		reg_opcode		= opc_ext;
	} else {
		if (flags & DIR_REVERSED)
			reg_opcode	= encode_reg(&self->src.reg);
		else
			reg_opcode	= encode_reg(&self->dest.reg);
	}

	if (need_sib)
		rm		= 0x04;
	else {
		if (flags & DIR_REVERSED)
			rm		= encode_reg(&self->dest.reg);
		else
			rm		= encode_reg(&self->src.reg);
	}

	mod_rm		= x86_encode_mod_rm(mod, reg_opcode, rm);

	emit(buffer, mod_rm);
}

#ifdef CONFIG_X86_64
static inline bool operand_is_reg_high(struct operand *operand)
{
	enum machine_reg reg;

	if (!operand_is_reg(operand))
		return false;

	reg		=  mach_reg(&operand->reg);

	return reg & 0x8;
}

static inline bool operand_is_reg_64(struct operand *operand)
{
	if (!operand_is_reg(operand))
		return false;

	return true;
}

static uint8_t insn_rex_prefix(struct insn *self)
{
	uint8_t ret = 0;

	if (operand_is_reg_64(&self->src) || operand_is_reg_64(&self->dest))
		ret	|= REX_W;

	if (operand_is_reg_high(&self->src) || operand_is_reg_high(&self->dest))
		ret	|= REX_B;

	return ret;
}
#else
static uint8_t insn_rex_prefix(struct insn *self)
{
	return 0;
}
#endif

static uint32_t insn_flags(struct insn *self, uint32_t encode)
{
	uint32_t flags;

	flags	= encode & FLAGS_MASK;

	if (flags & DIR_REVERSED) {
		if (flags & DST_REG)
			return flags;

		if (self->dest.disp == 0)
			flags	|= DST_MEM;
		else if (is_imm_8(self->dest.disp))
			flags	|= DST_MEM_DISP_BYTE;
		else
			flags	|= DST_MEM_DISP_FULL;
	} else {
		if (flags & SRC_REG)
			return flags;

		if (self->src.disp == 0)
			flags	|= SRC_MEM;
		else if (is_imm_8(self->src.disp))
			flags	|= SRC_MEM_DISP_BYTE;
		else
			flags	|= SRC_MEM_DISP_FULL;
	}

	return flags;
}

static uint8_t insn_opc_ext(struct insn *self, uint32_t encode)
{
	return (encode & OPCODE_EXT_MASK) >> OPCODE_EXT_SHIFT;
}

static uint8_t insn_opcode(struct insn *self, uint32_t encode)
{
	return encode & OPCODE_MASK;
}

void insn_encode(struct insn *self, struct buffer *buffer, struct basic_block *bb)
{
	uint8_t rex_prefix;
	uint32_t encode;
	uint8_t opc_ext;
	uint32_t flags;
	uint8_t opcode;

	encode		= encode_table[self->type];

	if (encode == 0)
		die("unrecognized instruction type %d", self->type);

	opcode		= insn_opcode(self, encode);

	opc_ext		= insn_opc_ext(self, encode);

	flags		= insn_flags(self, encode);

	if (flags & REPNE_PREFIX)
		emit(buffer, 0xf2);

	if (flags & REPE_PREFIX)
		emit(buffer, 0xf3);

	if (flags & ESCAPE_OPC_BYTE)
		emit(buffer, 0x0f);

	rex_prefix	= insn_rex_prefix(self);
	if (rex_prefix)
		emit(buffer, rex_prefix);

	emit(buffer, opcode);

	if (flags & MOD_RM) {
		bool need_sib;

		need_sib	= insn_need_sib(self, flags);

		insn_encode_mod_rm(self, buffer, flags, opc_ext);

		if (need_sib)
			insn_encode_sib(self, buffer, flags);
	}

	if (flags & IMM_MASK) {
		if (flags & DIR_REVERSED)
			emit_imm32(buffer, self->src.imm);
		else
			emit_imm32(buffer, self->dest.imm);
	}

	if (flags & MEM_DISP_MASK) {
		if (flags & DIR_REVERSED)
			emit_imm(buffer, self->dest.disp);
		else
			emit_imm(buffer, self->src.disp);
	}
}
