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
#include "arch/stack-frame.h"

#include "lib/buffer.h"

#include "vm/die.h"

#include <inttypes.h>
#include <stdint.h>

struct x86_insn {
	uint64_t		flags;
	uint8_t			rex_prefix;
	uint8_t			opcode;
	uint8_t			opc_ext;
	uint8_t			mod_rm;
	uint8_t			sib;
	int32_t			imm;
	int32_t			disp;
};

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
	MOD_RM			= (1ULL << 8),
	DIR_REVERSED		= (1ULL << 9),

	/* Source operand */
	SRC_NONE		= (1ULL << 11),

	SRC_IMM			= (1ULL << 12),
	SRC_IMM8		= (1ULL << 13),
	IMM_MASK		= SRC_IMM,
	IMM8_MASK		= SRC_IMM8,

#define SRC_REL			SRC_IMM
	REL_MASK		= SRC_IMM,

	SRC_REG			= (1ULL << 14),
	SRC_ACC			= (1ULL << 15),
	SRC_MEM			= (1ULL << 16),
	SRC_MEM_DISP_BYTE	= (1ULL << 17),
	SRC_MEM_DISP_FULL	= (1ULL << 18),
	SRC_MEMDISP		= (1ULL << 19),
	SRC_MEMLOCAL		= (1ULL << 20),
	SRC_MASK		= SRC_NONE|IMM_MASK|REL_MASK|SRC_REG|SRC_ACC|SRC_MEM|SRC_MEM_DISP_BYTE|SRC_MEM_DISP_FULL|SRC_MEMDISP|SRC_MEMLOCAL,

	/* Destination operand */
	DST_NONE		= (1ULL << 21),
	DST_REG			= (1ULL << 22),
	DST_ACC			= (1ULL << 23),	/* AL/AX */
	DST_MEM			= (1ULL << 24),
	DST_MEM_DISP_BYTE	= (1ULL << 25),	/* 8 bits */
	DST_MEM_DISP_FULL	= (1ULL << 26),	/* 16 bits or 32 bits */
	DST_MEMDISP		= (1ULL << 27),
	DST_MEMLOCAL		= (1ULL << 28),
	DST_MASK		= DST_REG|DST_ACC|DST_MEM|DST_MEM_DISP_BYTE|DST_MEM_DISP_FULL|DST_MEMDISP|DST_MEMLOCAL,

	MEM_DISP_MASK		= SRC_MEM_DISP_BYTE|SRC_MEM_DISP_FULL|DST_MEM_DISP_BYTE|DST_MEM_DISP_FULL,

	ESCAPE_OPC_BYTE		= (1ULL << 29),	/* Escape opcode byte */
	REPNE_PREFIX		= (1ULL << 30),	/* REPNE/REPNZ or SSE prefix */
	REPE_PREFIX		= (1ULL << 31),	/* REP/REPE/REPZ or SSE prefix */
	OPC_EXT			= (1ULL << 32),	/* The reg field of ModR/M byte provides opcode extension */
	OPC_REG			= (1ULL << 33), /* The opcode byte also provides operand register */

	NO_REX_W		= (1ULL << 34), /* No REX W prefix needed */
	INDEX			= (1ULL << 35),
	OPERAND_SIZE_PREFIX	= (1ULL << 36), /* Operand-size override prefix */

	/* Operand sizes */
#define WIDTH_BYTE			0UL

	WIDTH_FULL		= (1ULL << 37),	/* 16 bits or 32 bits */
	WIDTH_64		= (1ULL << 38),
	WIDTH_MASK		= WIDTH_BYTE|WIDTH_FULL|WIDTH_64,
};

/*
 *	Addressing modes
 */
enum x86_addmode {
	ADDMODE_ACC_MEM		= SRC_ACC|DST_MEM|DIR_REVERSED,		/* AL/AX -> memory */
	ADDMODE_ACC_REG		= SRC_ACC|DST_REG,			/* AL/AX -> reg */
	ADDMODE_IMM		= SRC_IMM,				/* immediate operand */
	ADDMODE_IMM8_REG	= SRC_IMM8|DST_REG|DIR_REVERSED,	/* immediate8 -> register */
	ADDMODE_IMM8_RM		= SRC_IMM8|MOD_RM|DIR_REVERSED,		/* immediate -> register/memory */
	ADDMODE_IMM_ACC		= SRC_IMM|DST_ACC,			/* immediate -> AL/AX */
	ADDMODE_IMM_REG		= SRC_IMM|DST_REG|DIR_REVERSED,		/* immediate -> register */
	ADDMODE_IMPLIED		= SRC_NONE|DST_NONE,			/* no operands */
	ADDMODE_MEMDISP_REG	= SRC_MEMDISP|DST_REG|MOD_RM,		/* memdisp -> register */
	ADDMODE_MEMLOCAL	= SRC_MEMLOCAL|DST_MEMLOCAL|MOD_RM,	/* memlocal */
	ADDMODE_MEMLOCAL_REG	= SRC_MEMLOCAL|DST_REG|MOD_RM,		/* memlocal -> register */
	ADDMODE_MEM_ACC		= SRC_ACC|DST_MEM,			/* memory -> AL/AX */
	ADDMODE_REG		= SRC_REG|DST_REG,			/* register */
	ADDMODE_REG_MEMDISP	= SRC_REG|DST_MEMDISP|MOD_RM|DIR_REVERSED,	/* memdisp -> register */
	ADDMODE_REG_MEMLOCAL	= SRC_REG|DST_MEMLOCAL|MOD_RM|DIR_REVERSED,	/* register -> register/memory */
	ADDMODE_REG_REG		= SRC_REG|DST_REG|MOD_RM,		/* register -> register */
	ADDMODE_REG_RM		= SRC_REG|MOD_RM|DIR_REVERSED,		/* register -> register/memory */
	ADDMODE_REL		= SRC_REL|DST_NONE,			/* relative */
	ADDMODE_RM		= SRC_REG|MOD_RM|DST_NONE|DIR_REVERSED,	/* register -> register/memory */
	ADDMODE_RM_REG		= DST_REG|MOD_RM,			/* register/memory -> register */
};

#define OPCODE(opc)		opc
#define OPCODE_MASK		0x00000000000000ffULL
#define FLAGS_MASK		0x1fffffffffffff00ULL

#define OPCODE_EXT_SHIFT	61
#define OPCODE_EXT(opc_ext)	(((uint64_t)opc_ext << OPCODE_EXT_SHIFT) | OPC_EXT | MOD_RM)
#define OPCODE_EXT_MASK		(0xe000000000000000ULL)

static uint64_t encode_table[NR_INSN_TYPES] = {
	[INSN_ADC_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(2)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_ADC_MEMBASE_REG]		= OPCODE(0x13) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_ADC_REG_REG]		= OPCODE(0x11) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_ADDSD_MEMDISP_XMM]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x58) | ADDMODE_MEMDISP_REG | WIDTH_64,
	[INSN_ADDSD_XMM_XMM]		= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x58) | ADDMODE_REG_REG | WIDTH_64,
	[INSN_ADDSS_XMM_XMM]		= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x58) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_ADD_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(0)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_ADD_MEMBASE_REG]		= OPCODE(0x03) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_ADD_REG_REG]		= OPCODE(0x01) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_AND_MEMBASE_REG]		= OPCODE(0x23) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_AND_REG_REG]		= OPCODE(0x21) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_CALL_REG]			= OPCODE(0xFF) | OPCODE_EXT(2)   | ADDMODE_RM | WIDTH_FULL,
	[INSN_CLTD_REG_REG]		= OPCODE(0x99) | WIDTH_FULL,
	[INSN_CMP_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(7)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_CMP_MEMBASE_REG]		= OPCODE(0x3b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_CMP_REG_REG]		= OPCODE(0x39) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_DIVSD_XMM_XMM]		= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x5e) | ADDMODE_REG_REG | WIDTH_64,
	[INSN_DIVSS_XMM_XMM]		= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x5e) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_FSTP_64_MEMLOCAL]		= OPCODE(0xdd) | OPCODE_EXT(3)   | ADDMODE_MEMLOCAL | WIDTH_64,
	[INSN_FSTP_MEMLOCAL]		= OPCODE(0xd9) | OPCODE_EXT(3)   | ADDMODE_MEMLOCAL | WIDTH_FULL,
	[INSN_JMP_MEMBASE]		= OPCODE(0xff) | OPCODE_EXT(4)   | ADDMODE_RM | WIDTH_FULL,
	[INSN_JMP_MEMINDEX]		= OPCODE(0xff) | OPCODE_EXT(4)   | ADDMODE_RM | INDEX | WIDTH_FULL,
	[INSN_MOVSD_MEMBASE_XMM]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_RM_REG | WIDTH_64,
	[INSN_MOVSD_MEMDISP_XMM]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_MEMDISP_REG | WIDTH_64,
	[INSN_MOVSD_MEMLOCAL_XMM]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_MEMLOCAL_REG | WIDTH_64,
	[INSN_MOVSD_XMM_MEMBASE]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x11) | ADDMODE_REG_RM | WIDTH_64,
	[INSN_MOVSD_XMM_MEMDISP]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x11) | ADDMODE_REG_MEMDISP | WIDTH_64,
	[INSN_MOVSD_XMM_MEMLOCAL]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x11) | ADDMODE_REG_MEMLOCAL | WIDTH_64,
	[INSN_MOVSD_XMM_XMM]		= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_REG_REG | WIDTH_64,
	[INSN_MOVSS_MEMBASE_XMM]	= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_RM_REG | WIDTH_FULL,
	[INSN_MOVSS_MEMDISP_XMM]	= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_MEMDISP_REG | WIDTH_64,
	[INSN_MOVSS_MEMLOCAL_XMM]	= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_MEMLOCAL_REG | WIDTH_FULL,
	[INSN_MOVSS_XMM_MEMBASE]	= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x11) | ADDMODE_REG_RM | WIDTH_FULL,
	[INSN_MOVSS_XMM_MEMDISP]	= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x11) | ADDMODE_REG_MEMDISP | WIDTH_64,
	[INSN_MOVSS_XMM_MEMLOCAL]	= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x11) | ADDMODE_REG_MEMLOCAL | WIDTH_FULL,
	[INSN_MOVSS_XMM_XMM]		= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x10) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_MOVSX_16_MEMBASE_REG]	= OPCODE(0xbf) | ESCAPE_OPC_BYTE | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_MOVSX_16_REG_REG]		= OPCODE(0xbf) | ESCAPE_OPC_BYTE | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_MOVSX_8_MEMBASE_REG]	= OPCODE(0xbe) | ESCAPE_OPC_BYTE | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_MOVSX_8_REG_REG]		= OPCODE(0xbe) | ESCAPE_OPC_BYTE | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_MOVZX_16_REG_REG]		= OPCODE(0xb7) | ESCAPE_OPC_BYTE | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_MOV_MEMBASE_REG]		= OPCODE(0x8b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_MOV_REG_MEMBASE]		= OPCODE(0x89) | ADDMODE_REG_RM  | WIDTH_FULL,
	[INSN_MOV_REG_REG]		= OPCODE(0x89) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_MULSD_MEMDISP_XMM]	= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x59) | ADDMODE_MEMDISP_REG | WIDTH_64,
	[INSN_MULSD_XMM_XMM]		= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x59) | ADDMODE_REG_REG | WIDTH_64,
	[INSN_MULSS_XMM_XMM]		= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x59) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_NEG_REG]			= OPCODE(0xf7) | OPCODE_EXT(3)   | ADDMODE_REG     | DIR_REVERSED | WIDTH_FULL,
	[INSN_OR_MEMBASE_REG]		= OPCODE(0x0b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_OR_REG_REG]		= OPCODE(0x09) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_POP_MEMLOCAL]		= OPCODE(0x8f) | OPCODE_EXT(0)   | ADDMODE_MEMLOCAL| WIDTH_FULL | NO_REX_W,
	[INSN_POP_REG]			= OPCODE(0x58) | OPC_REG         | ADDMODE_REG     | DIR_REVERSED | WIDTH_FULL | NO_REX_W,
	[INSN_PUSH_MEMLOCAL]		= OPCODE(0xff) | OPCODE_EXT(6)   | ADDMODE_MEMLOCAL| WIDTH_FULL | NO_REX_W,
	[INSN_PUSH_REG]			= OPCODE(0x50) | OPC_REG         | ADDMODE_REG     | WIDTH_FULL | NO_REX_W,
	[INSN_RET]			= OPCODE(0xc3) | ADDMODE_IMPLIED,
	[INSN_SAR_IMM_REG]		= OPCODE(0xc1) | OPCODE_EXT(7)   | ADDMODE_IMM8_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_SAR_REG_REG]		= OPCODE(0xd3) | OPCODE_EXT(7)   | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_SBB_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(3)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_SBB_MEMBASE_REG]		= OPCODE(0x1b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_SBB_REG_REG]		= OPCODE(0x19) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_SHL_REG_REG]		= OPCODE(0xd3) | OPCODE_EXT(4)   | ADDMODE_REG_REG|DIR_REVERSED | WIDTH_FULL,
	[INSN_SHR_REG_REG]		= OPCODE(0xd3) | OPCODE_EXT(5)   | ADDMODE_REG_REG|DIR_REVERSED | WIDTH_FULL,
	[INSN_SUBSD_XMM_XMM]		= REPNE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x5c) | ADDMODE_REG_REG | WIDTH_64,
	[INSN_SUBSS_XMM_XMM]		= REPE_PREFIX  | ESCAPE_OPC_BYTE | OPCODE(0x5c) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_SUB_IMM_REG]		= OPCODE(0x81) | OPCODE_EXT(5)   | ADDMODE_IMM_REG | WIDTH_FULL,
	[INSN_SUB_MEMBASE_REG]		= OPCODE(0x2b) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_SUB_REG_REG]		= OPCODE(0x29) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
	[INSN_TEST_MEMBASE_REG]		= OPCODE(0x85) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_XORPD_XMM_XMM]		= OPERAND_SIZE_PREFIX | ESCAPE_OPC_BYTE | OPCODE(0x57) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_XORPS_XMM_XMM]		= ESCAPE_OPC_BYTE | OPCODE(0x57) | ADDMODE_REG_REG | WIDTH_FULL,
	[INSN_XOR_MEMBASE_REG]		= OPCODE(0x33) | ADDMODE_RM_REG  | WIDTH_FULL,
	[INSN_XOR_REG_REG]		= OPCODE(0x31) | ADDMODE_REG_REG | DIR_REVERSED | WIDTH_FULL,
};

static inline bool is_imm_8(int32_t imm)
{
	return (imm >= -128) && (imm <= 127);
}

static inline void emit(struct buffer *buf, uint8_t c)
{
	int err;

	err = append_buffer(buf, c);

	if (err)
		die("out of buffer space");
}

static void emit_imm32(struct buffer *buf, int32_t imm)
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

static void emit_imm(struct buffer *buf, int32_t imm)
{
	if (is_imm_8(imm))
		emit(buf, imm);
	else
		emit_imm32(buf, imm);
}

static unsigned char encode_reg(struct use_position *reg)
{
	return x86_encode_reg(mach_reg(reg)) & 0x7;
}

static inline uint32_t mod_src_encode(uint64_t flags)
{
	switch (flags & SRC_MASK) {
	case SRC_MEMDISP:
	case SRC_MEM:
		return 0x00;
	case SRC_MEM_DISP_BYTE:
		return 0x01;
	case SRC_MEM_DISP_FULL:
	case SRC_MEMLOCAL:
		return 0x02;
	case SRC_REG:
		return 0x03;
	default:
		break;
	}

	die("unrecognized flags %" PRIx64, flags & SRC_MASK);
}

static inline uint32_t mod_dest_encode(uint64_t flags)
{
	switch (flags & DST_MASK) {
	case DST_MEMDISP:
	case DST_MEM:
		return 0x00;
	case DST_MEM_DISP_BYTE:
		return 0x01;
	case DST_MEM_DISP_FULL:
	case DST_MEMLOCAL:
		return 0x02;
	case DST_REG:
		return 0x03;
	default:
		break;
	}

	die("unrecognized flags %" PRIx64, flags & DST_MASK);
}

#ifdef CONFIG_X86_64
static inline bool operand_uses_reg(struct operand *operand, enum machine_reg reg)
{
	if (!operand_is_reg(operand))
		return false;

	return mach_reg(&operand->reg) == reg;
}

static inline bool insn_uses_reg(struct insn *self, enum machine_reg reg)
{
	return operand_uses_reg(&self->src, reg) || operand_uses_reg(&self->dest, reg);
}
#endif

static inline bool insn_need_disp(struct insn *self)
{
#ifdef CONFIG_X86_64
	if (insn_uses_reg(self, MACH_REG_R13) || insn_uses_reg(self, MACH_REG_RBP))
		return true;
#endif
	return false;
}

static bool insn_need_sib(struct insn *self, uint64_t flags)
{
	enum machine_reg reg;

	if (flags & INDEX)
		return true;
#ifdef CONFIG_X86_64
	if (flags & DST_NONE && insn_uses_reg(self, MACH_REG_R12)) /* DST_NONE? */
		return true;
#endif
	if (flags & OPC_EXT)
		return false;

	if (flags & DIR_REVERSED) {
		if (flags & (DST_MEMDISP|DST_MEMLOCAL))
			return false;

		reg	= mach_reg(&self->dest.base_reg);
	} else {
		if (flags & (SRC_MEMDISP|SRC_MEMLOCAL))
			return false;

		reg	= mach_reg(&self->src.base_reg);
	}
	return reg == MACH_REG_xSP;
}

static uint8_t insn_encode_sib(struct insn *self, uint64_t flags)
{
	uint8_t shift, index, base;
	uint8_t sib;

	if (!insn_need_sib(self, flags))
		return 0;

	if (flags & INDEX) {
		if (flags & DIR_REVERSED) {
			shift	= self->dest.shift;
			index	= encode_reg(&self->dest.index_reg);
		} else {
			shift	= self->src.shift;
			index	= encode_reg(&self->src.index_reg);
		}
	} else {
		shift	= 0x00;
		index	= 0x04;
	}

	if (flags & DIR_REVERSED)
		base	= encode_reg(&self->dest.base_reg);
	else
		base	= encode_reg(&self->src.base_reg);

	sib		= x86_encode_sib(shift, index, base);

	return sib;
}

static uint8_t insn_encode_rm(struct insn *self, uint64_t flags)
{
	uint8_t rm;

	if (flags & DIR_REVERSED) {
		if (flags & DST_MEMDISP)
			rm		= 0x05;
		else if (flags & DST_MEMLOCAL)
			rm		= x86_encode_reg(MACH_REG_xBP);
		else
			rm		= encode_reg(&self->dest.reg);
	} else {
		if (flags & SRC_MEMDISP)
			rm		= 0x05;
		else if (flags & SRC_MEMLOCAL)
			rm		= x86_encode_reg(MACH_REG_xBP);
		else
			rm		= encode_reg(&self->src.reg);
	}
	return rm;
}

static uint8_t insn_encode_reg(struct insn *self, uint64_t flags)
{
	uint8_t reg_opcode;

	if (flags & DIR_REVERSED)
		reg_opcode	= encode_reg(&self->src.reg);
	else
		reg_opcode	= encode_reg(&self->dest.reg);

	return reg_opcode;
}

static uint8_t insn_encode_mod_rm(struct insn *self, uint64_t flags, uint8_t opc_ext)
{
	uint8_t mod_rm, mod, reg_opcode, rm;
	bool need_sib;

	need_sib	= insn_need_sib(self, flags);

	if (flags & INDEX) {
		mod	= 0x00;
	} else {
		if (flags & DIR_REVERSED)
			mod		= mod_dest_encode(flags);
		else
			mod		= mod_src_encode(flags);
	}

	if (flags & OPC_EXT) {
		reg_opcode		= opc_ext;
	} else {
		reg_opcode		= insn_encode_reg(self, flags);
	}

	if (need_sib)
		rm		= 0x04;
	else {
		rm		= insn_encode_rm(self, flags);
	}

	mod_rm		= x86_encode_mod_rm(mod, reg_opcode, rm);

	return mod_rm;
}

#ifdef CONFIG_X86_64
static inline bool operand_is_reg_high(struct operand *operand)
{
	enum machine_reg reg;
	uint8_t reg_num;

	if (!operand_is_reg(operand))
		return false;

	reg		= mach_reg(&operand->reg);

	reg_num		= x86_encode_reg(reg);

	return reg_num & 0x8;
}

static inline bool operand_is_reg_64(struct operand *operand)
{
	if (!operand_is_reg(operand))
		return false;

	return !operand_is_xmm_reg(operand);
}

static uint8_t insn_rex_operand_64(struct insn *self, uint64_t flags)
{
	if (insn_is_call(self) || insn_is_branch(self))
		return 0;

	if (flags & NO_REX_W)
		return 0;

	if (operand_is_reg_64(&self->src) || operand_is_reg_64(&self->dest))
		return REX_W;

	return 0;
}

static uint8_t insn_rex_prefix(struct insn *self, uint64_t flags)
{
	uint8_t ret;

	if (flags & (SRC_MEMLOCAL|DST_MEMLOCAL))
		return 0;

	ret	= insn_rex_operand_64(self, flags);

	if (flags & DIR_REVERSED) {
		if (operand_is_reg_high(&self->src))
			ret	|= REX_R;
		if (operand_is_reg_high(&self->dest))
			ret	|= REX_B;
	} else {
		if (operand_is_reg_high(&self->src))
			ret	|= REX_B;
		if (operand_is_reg_high(&self->dest))
			ret	|= REX_R;
	}

	return ret;
}
#else
static uint8_t insn_rex_prefix(struct insn *self, uint64_t flags)
{
	return 0;
}
#endif

static int32_t insn_slot_offset(struct x86_insn *insn, struct stack_slot *slot)
{
	if (insn->flags & WIDTH_64)
		return slot_offset_64(slot);

	return slot_offset(slot);
}

static void insn_disp(struct insn *self, struct x86_insn *insn)
{
	if (insn->flags & (SRC_MEMLOCAL|DST_MEMLOCAL)) {
		if (insn->flags & DIR_REVERSED)
			insn->disp	= insn_slot_offset(insn, self->dest.slot);
		else
			insn->disp	= insn_slot_offset(insn, self->src.slot);

		return;
	}

	if (insn->flags & (SRC_MEMDISP|DST_MEMDISP)) {
		/* Callers pass the displacement in ->imm rather than ->disp. */
		if (insn->flags & DIR_REVERSED)
			insn->disp	= self->dest.imm;
		else
			insn->disp	= self->src.imm;
		return;
	}

	if (insn->flags & DIR_REVERSED) {
		if (insn->flags & DST_REG)
			return;

		insn->disp	= self->dest.disp;

		if (insn->disp == 0 && !insn_need_disp(self))
			insn->flags	|= DST_MEM;
		else if (is_imm_8(insn->disp))
			insn->flags	|= DST_MEM_DISP_BYTE;
		else
			insn->flags	|= DST_MEM_DISP_FULL;
	} else {
		if (insn->flags & SRC_REG)
			return;

		insn->disp	= self->src.disp;

		if (insn->disp == 0 && !insn_need_disp(self))
			insn->flags	|= SRC_MEM;
		else if (is_imm_8(insn->disp))
			insn->flags	|= SRC_MEM_DISP_BYTE;
		else
			insn->flags	|= SRC_MEM_DISP_FULL;
	}
}

static uint64_t insn_flags(struct insn *self, uint64_t encode)
{
	return encode & FLAGS_MASK;
}

static uint8_t insn_opc_ext(struct insn *self, uint64_t encode)
{
	return (encode & OPCODE_EXT_MASK) >> OPCODE_EXT_SHIFT;
}

static uint8_t insn_opcode(struct insn *self, uint64_t encode)
{
	return encode & OPCODE_MASK;
}

static void x86_insn_encode(struct x86_insn *insn, struct buffer *buffer)
{
	if (insn->flags & REPNE_PREFIX)
		emit(buffer, 0xf2);

	if (insn->flags & REPE_PREFIX)
		emit(buffer, 0xf3);

	if (insn->flags & OPERAND_SIZE_PREFIX)
		emit(buffer, 0x66);

	if (insn->rex_prefix)
		emit(buffer, insn->rex_prefix);

	if (insn->flags & ESCAPE_OPC_BYTE)
		emit(buffer, 0x0f);

	emit(buffer, insn->opcode);

	if (insn->flags & MOD_RM)
		emit(buffer, insn->mod_rm);

	if (insn->sib)
		emit(buffer, insn->sib);

	if (insn->flags & MEM_DISP_MASK)
		emit_imm(buffer, insn->disp);

	if (insn->flags & (SRC_MEMDISP|DST_MEMDISP|SRC_MEMLOCAL|DST_MEMLOCAL))
		emit_imm32(buffer, insn->disp);

	if (insn->flags & IMM8_MASK)
		emit(buffer, insn->imm);

	if (insn->flags & IMM_MASK)
		emit_imm32(buffer, insn->imm);
}

void insn_encode(struct insn *self, struct buffer *buffer, struct basic_block *bb)
{
	struct x86_insn insn;
	uint64_t encode;

	encode		= encode_table[self->type];

	if (encode == 0)
		die("unrecognized instruction type %d", self->type);

	insn	= (struct x86_insn) {
		.opcode		= insn_opcode(self, encode),
		.opc_ext	= insn_opc_ext(self, encode),
		.flags		= insn_flags(self, encode),
	};

	insn_disp(self, &insn);

	insn.rex_prefix	= insn_rex_prefix(self, insn.flags);

	if (insn.flags & OPC_REG) {
		if (insn.flags & DIR_REVERSED)
			insn.opcode	+= encode_reg(&self->dest.reg);
		else
			insn.opcode	+= encode_reg(&self->src.reg);
	}

	if (insn.flags & MOD_RM) {
		insn.mod_rm	= insn_encode_mod_rm(self, insn.flags, insn.opc_ext);

		insn.sib	= insn_encode_sib(self, insn.flags);
	}

	if (insn.flags & (IMM8_MASK|IMM_MASK)) {
		if (insn.flags & DIR_REVERSED)
			insn.imm	= self->src.imm;
		else
			insn.imm	= self->dest.imm;
	}

	x86_insn_encode(&insn, buffer);
}
