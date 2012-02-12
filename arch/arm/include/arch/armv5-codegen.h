#ifndef JATO_ARM_ARMV5_CODEGEN_H
#define JATO_ARM_ARMV5_CODEGEN_H

/*
 * General-Purpose Registers
 */

#define ARM_R0			0
#define ARM_R1			1
#define ARM_R2			2
#define ARM_R3			3
#define ARM_R4			4
#define ARM_R5			5
#define ARM_R6			6
#define ARM_R7			7
#define ARM_R8			8
#define ARM_R9			9
#define ARM_R10			10
#define ARM_R11			11
#define ARM_R12			12
#define ARM_SP			13
#define ARM_LR			14
#define ARM_PC			15

/*
 * The Condition Field
 */

#define ARM_COND_EQ		0b0000ULL
#define ARM_COND_NE		0b0001ULL
#define ARM_COND_CS_HS		0b0010ULL
#define ARM_COND_CC_LO		0b0011ULL
#define ARM_COND_MI		0b0100ULL
#define ARM_COND_PL		0b0101ULL
#define ARM_COND_VS		0b0110ULL
#define ARM_COND_VC		0b0111ULL
#define ARM_COND_HI		0b1000ULL
#define ARM_COND_LS		0b1001ULL
#define ARM_COND_GE		0b1010ULL
#define ARM_COND_LT		0b1011ULL
#define ARM_COND_GT		0b1100ULL
#define ARM_COND_LE		0b1101ULL
#define ARM_COND_AL		0b1110ULL

/*
 * Data processing
 */

#define ARM_DATA(Cond, I, Opcode, S, Rn, Rd, Operand2) \
	((Cond) << 28 | (I) << 25 | (Opcode) << 21 | (S) << 20 | (Rn) << 16 | (Rd) << 12 | (Operand2))

#define ARM_ASR		0b0101
#define ARM_LSL		0b0001
#define ARM_LSR		0b0011
#define ARM_ROR		0b0111

#define ARM_SHIFT_REG(Rn, shift, Rm) \
	((Rm) << 8 | (shift) << 4 | (Rn))

/* Multiply */
#define ARM_MUL(Cond, A, S, Rd, Rn, Rs, Rm)

/* Swap */
#define ARM_SWAP(Cond, Rn, Rd, Rm)

/* Load/Store Byte/Word */
#define ARM_LS_REG(Cond, P, U, B, W, L, Rn, Rd, shift_amount, shift, Rm) \
	((Cond) << 28 | 0b011 << 25 | (P) << 24 | (U) << 23 | (B) << 22 | (W) << 21 | (L) << 20 | (Rn) << 16 | (Rd) << 12 | (shift_amount) << 7 | (shift) << 5 | (Rm))

/* Load/Store Multiple */
#define ARM_LS_MULTIPLE(Cond, P, U, S, W, L, Rn, RegisterList) \
	((Cond) << 28 | 0b100 << 25 < (P) << 24 | (U) << 23 | (B) << 22 | (W) << 21 | (L) << 20 | (Rn) << 16 | (RegisterList))

/*
 * Branch
 */

#define ARM_BRANCH_SHIFT 2

#define ARM_BRANCH_OFFSET(Offset) \
	(((unsigned int) (Offset) >> ARM_BRANCH_SHIFT) & 0x00ffffffUL)

#define ARM_BRANCH(Cond, L, Offset) \
	((Cond) << 28 | 0b101 << 25 | (L) << 24 | (Offset))

/*
 * ARM instructions
 */

#define ARM_MOV(Rd, shifter_operand)	ARM_DATA(ARM_COND_AL, 0, 0b1101, 0, 0 , Rd, shifter_operand)
#define ARM_MOV_REG(Rd, Rm)		ARM_MOV(Rd, Rm)
#define ARM_ASR_REG(Rd, Rn, Rm)		ARM_MOV(Rd, ARM_SHIFT_REG(Rn, ARM_ASR, Rm))
#define ARM_LSL_REG(Rd, Rn, Rm)		ARM_MOV(Rd, ARM_SHIFT_REG(Rn, ARM_LSL, Rm))
#define ARM_LSR_REG(Rd, Rn, Rm)		ARM_MOV(Rd, ARM_SHIFT_REG(Rn, ARM_LSR, Rm))
#define ARM_ROR_REG(Rd, Rn, Rm)		ARM_MOV(Rd, ARM_SHIFT_REG(Rn, ARM_ROR, Rm))

#define ARM_ADC_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b0101, 0, Rn, Rd, Rm)
#define ARM_ADC_IMM8(Rd, Rn, imm)	ARM_DATA(ARM_COND_AL, 1, 0b0101, 0, Rn, Rd, imm)
#define ARM_ADD_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b0100, 0, Rn, Rd, Rm)
#define ARM_ADD_IMM8(Rd, Rn, imm)	ARM_DATA(ARM_COND_AL, 1, 0b0100, 0, Rn, Rd, imm)
#define ARM_ADDS_REG(Rd, Rn, Rm)	ARM_DATA(ARM_COND_AL, 0, 0b0100, 1, Rn, Rd, Rm)
#define ARM_BIC_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b1110, 0, Rn, Rd, Rm)
#define ARM_CMP_REG(Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b1010, 1, Rn, 0,  Rm)
#define ARM_CMN_REG(Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b1011, 1, Rn, 0,  Rm)
#define ARM_EOR_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b0001, 0, Rn, Rd, Rm)
#define ARM_AND_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b0000, 0, Rn, Rd, Rm)
#define ARM_MVN_REG(Rd, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b1111, 0, 0 , Rd, Rm)
#define ARM_ORR_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b1100, 0, Rn, Rd, Rm)
#define ARM_RSB_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b0011, 0, Rn, Rd, Rm)
#define ARM_RSC_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b0111, 0, Rn, Rd, Rm)
#define ARM_SBC_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b0110, 0, Rn, Rd, Rm)
#define ARM_SUB_REG(Rd, Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b0010, 0, Rn, Rd, Rm)
#define ARM_SUB_IMM8(Rd, Rn, imm)	ARM_DATA(ARM_COND_AL, 1, 0b0010, 0, Rn, Rd, imm)
#define ARM_SUBS_REG(Rd, Rn, Rm)	ARM_DATA(ARM_COND_AL, 0, 0b0010, 1, Rn, Rd, Rm)
#define ARM_TST_REG(Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b1000, 1, Rn, 0,  Rm)
#define ARM_TEQ_REG(Rn, Rm)		ARM_DATA(ARM_COND_AL, 0, 0b1001, 1, Rn, 0,  Rm)

#define ARM_B(Offset)			ARM_BRANCH(ARM_COND_AL, 0, Offset)
#define ARM_BL(Offset)			ARM_BRANCH(ARM_COND_AL, 1, Offset)

#endif /* JATO_ARM_ARMV5_CODEGEN_H */
