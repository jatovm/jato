#ifndef __BYTECODE_ARITHMETIC_CONVERTERS_H
#define __BYTECODE_ARITHMETIC_CONVERTERS_H

extern int convert_iadd(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_ladd(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_fadd(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_dadd(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_isub(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lsub(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_fsub(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_dsub(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_imul(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lmul(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_fmul(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_dmul(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_idiv(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_ldiv(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_fdiv(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_ddiv(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_irem(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lrem(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_frem(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_drem(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_ineg(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lneg(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_fneg(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_dneg(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_ishl(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lshl(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_ishr(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lshr(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_iand(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_land(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_ior(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lor(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_ixor(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lxor(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_iinc(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_lcmp(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_xcmpl(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_xcmpg(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_xcmpl(struct compilation_unit *, struct basic_block *, unsigned long);
extern int convert_xcmpg(struct compilation_unit *, struct basic_block *, unsigned long);

#endif
