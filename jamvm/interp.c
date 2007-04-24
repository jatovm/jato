/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007
 * Robert Lougher <rob@lougher.org.uk>.
 *
 * This file is part of JamVM.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "jam.h"
#include "thread.h"
#include "lock.h"
#include "interp.h"

#ifdef DIRECT
#include "interp-direct.h"
#else
#include "interp-indirect.h"
#endif

/* Macros for checking for common exceptions */

#define THROW_EXCEPTION(excep_name, message)                               \
{                                                                          \
    frame->last_pc = pc;                                                   \
    signalException(excep_name, message);                                  \
    goto throwException;                                                   \
}

#define NULL_POINTER_CHECK(ref)                                            \
    if(!ref) THROW_EXCEPTION("java/lang/NullPointerException", NULL);

#define MAX_INT_DIGITS 11

#define ARRAY_BOUNDS_CHECK(array, idx)                                     \
{                                                                          \
    if(idx >= ARRAY_LEN(array)) {                                          \
        char buff[MAX_INT_DIGITS];                                         \
        snprintf(buff, MAX_INT_DIGITS, "%d", idx);                         \
        THROW_EXCEPTION("java/lang/ArrayIndexOutOfBoundsException", buff); \
    }                                                                      \
}

uintptr_t *executeJava() {
    CodePntr pc;
    ExecEnv *ee = getExecEnv();
    Frame *frame = ee->last_frame;
    MethodBlock *mb = frame->mb;
    uintptr_t *lvars = frame->lvars;
    uintptr_t *ostack = frame->ostack;
    ConstantPool *cp = &(CLASS_CB(mb->class)->constant_pool);

    Object *this = (Object*)lvars[0];
    Class *new_class;
    MethodBlock *new_mb;
    uintptr_t *arg1;
#ifdef PREFETCH
    const void *next_handler;
#endif
#ifdef USE_CACHE
    union {
        struct {
            uintptr_t v1;
            uintptr_t v2;
        } i;
        long long l;
    } cache;
#endif

#ifdef THREADED
#define L(opcode, level) &&opc##opcode##_##level

#ifdef DIRECT
#define I(opcode, level) &&unused
#else
#define I(opcode, level) L(opcode, level)
#endif

#define DEF_HANDLER_TABLE(level)                                              \
    HANDLER_TABLE_T *handlers_##level[] = {                                   \
        L(0,level), L(1,level), L(2,level), L(3,level), L(4,level),           \
        L(5,level), L(6,level), L(7,level), L(8,level), L(9,level),           \
        L(10,level), L(11,level), L(12,level), L(13,level), L(14,level),      \
        L(15,level), L(16,level), L(17,level), L(18,level), I(19,level),      \
        L(20,level), L(21,level), L(22,level), L(23,level), L(24,level),      \
        L(25,level), L(26,level), L(27,level), L(28,level), L(29,level),      \
        L(30,level), L(31,level), L(32,level), L(33,level), L(34,level),      \
        L(35,level), L(36,level), L(37,level), L(38,level), L(39,level),      \
        L(40,level), L(41,level), I(42,level), L(43,level), L(44,level),      \
        L(45,level), L(46,level), L(47,level), L(48,level), L(49,level),      \
        L(50,level), L(51,level), L(52,level), L(53,level), L(54,level),      \
        L(55,level), L(56,level), L(57,level), L(58,level), L(59,level),      \
        L(60,level), L(61,level), L(62,level), L(63,level), L(64,level),      \
        L(65,level), L(66,level), L(67,level), L(68,level), L(69,level),      \
        L(70,level), L(71,level), L(72,level), L(73,level), L(74,level),      \
        L(75,level), L(76,level), L(77,level), L(78,level), L(79,level),      \
        L(80,level), L(81,level), L(82,level), L(83,level), L(84,level),      \
        L(85,level), L(86,level), L(87,level), L(88,level), L(89,level),      \
        L(90,level), L(91,level), L(92,level), L(93,level), L(94,level),      \
        L(95,level), L(96,level), L(97,level), L(98,level), L(99,level),      \
        L(100,level), L(101,level), L(102,level), L(103,level), L(104,level), \
        L(105,level), L(106,level), L(107,level), L(108,level), L(109,level), \
        L(110,level), L(111,level), L(112,level), L(113,level), L(114,level), \
        L(115,level), L(116,level), L(117,level), L(118,level), L(119,level), \
        L(120,level), L(121,level), L(122,level), L(123,level), L(124,level), \
        L(125,level), L(126,level), L(127,level), L(128,level), L(129,level), \
        L(130,level), L(131,level), L(132,level), L(133,level), L(134,level), \
        L(135,level), L(136,level), L(137,level), L(138,level), L(139,level), \
        L(140,level), L(141,level), L(142,level), L(143,level), L(144,level), \
        L(145,level), L(146,level), L(147,level), L(148,level), L(149,level), \
        L(150,level), L(151,level), L(152,level), L(153,level), L(154,level), \
        L(155,level), L(156,level), L(157,level), L(158,level), L(159,level), \
        L(160,level), L(161,level), L(162,level), L(163,level), L(164,level), \
        L(165,level), L(166,level), L(167,level), L(168,level), L(169,level), \
        L(170,level), L(171,level), L(172,level), L(173,level), L(174,level), \
        L(175,level), L(176,level), L(177,level), L(178,level), L(179,level), \
        L(180,level), L(181,level), L(182,level), L(183,level), L(184,level), \
        L(185,level), &&unused, L(187,level), L(188,level), L(189,level),     \
        L(190,level), L(191,level), L(192,level), L(193,level), L(194,level), \
        L(195,level), I(196,level), L(197,level), L(198,level), L(199,level), \
        L(200,level), L(201,level), &&unused, L(203,level), L(204,level),     \
        &&unused, L(206,level), L(207,level), L(208,level), L(209,level),     \
        L(210,level), L(211,level), L(212,level), L(213,level), L(214,level), \
        L(215,level), L(216,level), &&unused, &&unused, &&unused, &&unused,   \
        &&unused, &&unused, &&unused, &&unused, &&unused, I(226,level),       \
        I(227,level), I(228,level), L(229,level), I(230,level), L(231,level), \
        L(232,level), L(233,level), &&unused, L(235,level), &&unused,         \
        &&unused, L(238,level), L(239,level), &&unused, &&unused, &&unused,   \
        L(243,level), L(244,level), L(245, level), &&unused, &&unused,        \
        &&unused, &&unused, &&unused, &&unused, &&unused, &&unused, &&unused, \
        &&unused}

    DEF_HANDLER_TABLE(0);

#ifdef USE_CACHE
    DEF_HANDLER_TABLE(1);
    DEF_HANDLER_TABLE(2);
    const void **handlers[] = {handlers_0, handlers_1, handlers_2};
#else
    const void **handlers[] = {handlers_0};
#endif

    PREPARE_MB(mb);
    pc = (CodePntr)mb->code;

    DISPATCH_FIRST

#else /* THREADED */

    pc = (CodePntr)mb->code;

    while(TRUE) {
        switch(*pc) {
            default:
#endif

unused:
#ifndef DIRECT
    jam_printf("Unrecognised opcode %d in: %s.%s\n", *pc, CLASS_CB(mb->class)->name, mb->name);
    exitVM(1);
#else
rewrite_lock:
    REDISPATCH
#endif

#define MULTI_LEVEL_OPCODES(level)                         \
    DEF_OPC(OPC_ICONST_M1, level)                          \
        PUSH_##level(-1, 1);                               \
                                                           \
    DEF_OPC(OPC_ACONST_NULL, level)                        \
    DEF_OPC(OPC_ICONST_0, level)                           \
    DEF_OPC(OPC_FCONST_0, level)                           \
        PUSH_##level(0, 1);                                \
                                                           \
    DEF_OPC(OPC_ICONST_1, level)                           \
        PUSH_##level(1, 1);                                \
                                                           \
    DEF_OPC(OPC_ICONST_2, level)                           \
        PUSH_##level(2, 1);                                \
                                                           \
    DEF_OPC(OPC_ICONST_3, level)                           \
        PUSH_##level(3, 1);                                \
                                                           \
    DEF_OPC(OPC_ICONST_4, level)                           \
        PUSH_##level(4, 1);                                \
                                                           \
    DEF_OPC(OPC_ICONST_5, level)                           \
        PUSH_##level(5, 1);                                \
                                                           \
    DEF_OPC(OPC_FCONST_1, level)                           \
        PUSH_##level(FLOAT_1_BITS, 1);                     \
                                                           \
    DEF_OPC(OPC_FCONST_2, level)                           \
        PUSH_##level(FLOAT_2_BITS, 1);                     \
                                                           \
    DEF_OPC(OPC_SIPUSH, level)                             \
        PUSH_##level(DOUBLE_SIGNED(pc), 3);                \
                                                           \
    DEF_OPC(OPC_BIPUSH, level)                             \
        PUSH_##level(SINGLE_SIGNED(pc), 2);                \
                                                           \
    DEF_OPC(OPC_LDC_QUICK, level)                          \
        PUSH_##level(RESOLVED_CONSTANT(pc), 2);            \
                                                           \
    DEF_OPC(OPC_LDC_W_QUICK, level)                        \
        PUSH_##level(CP_INFO(cp, DOUBLE_INDEX(pc)), 3);    \
                                                           \
    DEF_OPC(OPC_ILOAD, level)                              \
    DEF_OPC(OPC_FLOAD, level)                              \
    DEF_OPC(OPC_ALOAD, level)                              \
        PUSH_##level(lvars[SINGLE_INDEX(pc)], 2);          \
                                                           \
    DEF_OPC(OPC_ALOAD_THIS, level)                         \
        ALOAD_THIS(level);                                 \
                                                           \
    DEF_OPC(OPC_ILOAD_0, level)                            \
    DEF_OPC(OPC_FLOAD_0, level)                            \
        PUSH_##level(lvars[0], 1);                         \
                                                           \
    DEF_OPC(OPC_ILOAD_1, level)                            \
    DEF_OPC(OPC_FLOAD_1, level)                            \
    DEF_OPC(OPC_ALOAD_1, level)                            \
        PUSH_##level(lvars[1], 1);                         \
                                                           \
    DEF_OPC(OPC_ILOAD_2, level)                            \
    DEF_OPC(OPC_FLOAD_2, level)                            \
    DEF_OPC(OPC_ALOAD_2, level)                            \
        PUSH_##level(lvars[2], 1);                         \
                                                           \
    DEF_OPC(OPC_ILOAD_3, level)                            \
    DEF_OPC(OPC_FLOAD_3, level)                            \
    DEF_OPC(OPC_ALOAD_3, level)                            \
        PUSH_##level(lvars[3], 1);                         \
                                                           \
    DEF_OPC(OPC_ISTORE, level)                             \
    DEF_OPC(OPC_FSTORE, level)                             \
    DEF_OPC(OPC_ASTORE, level)                             \
        POP_##level(lvars[SINGLE_INDEX(pc)], 2);           \
                                                           \
    DEF_OPC(OPC_ISTORE_0, level)                           \
    DEF_OPC(OPC_ASTORE_0, level)                           \
    DEF_OPC(OPC_FSTORE_0, level)                           \
        POP_##level(lvars[0], 1);                          \
                                                           \
    DEF_OPC(OPC_ISTORE_1, level)                           \
    DEF_OPC(OPC_ASTORE_1, level)                           \
    DEF_OPC(OPC_FSTORE_1, level)                           \
        POP_##level(lvars[1], 1);                          \
                                                           \
    DEF_OPC(OPC_ISTORE_2, level)                           \
    DEF_OPC(OPC_ASTORE_2, level)                           \
    DEF_OPC(OPC_FSTORE_2, level)                           \
        POP_##level(lvars[2], 1);                          \
                                                           \
    DEF_OPC(OPC_ISTORE_3, level)                           \
    DEF_OPC(OPC_ASTORE_3, level)                           \
    DEF_OPC(OPC_FSTORE_3, level)                           \
        POP_##level(lvars[3], 1);                          \
                                                           \
    DEF_OPC(OPC_IADD, level)                               \
        BINARY_OP_##level(+);                              \
                                                           \
    DEF_OPC(OPC_ISUB, level)                               \
        BINARY_OP_##level(-);                              \
                                                           \
    DEF_OPC(OPC_IMUL, level)                               \
        BINARY_OP_##level(*);                              \
                                                           \
    DEF_OPC(OPC_IDIV, level)                               \
        ZERO_DIVISOR_CHECK_##level;                        \
        BINARY_OP_##level(/);                              \
                                                           \
    DEF_OPC(OPC_IREM, level)                               \
        ZERO_DIVISOR_CHECK_##level;                        \
        BINARY_OP_##level(%);                              \
                                                           \
    DEF_OPC(OPC_IAND, level)                               \
        BINARY_OP_##level(&);                              \
                                                           \
    DEF_OPC(OPC_IOR, level)                                \
        BINARY_OP_##level(|);                              \
                                                           \
    DEF_OPC(OPC_IXOR, level)                               \
        BINARY_OP_##level(^);                              \
                                                           \
    DEF_OPC(OPC_INEG, level)                               \
        UNARY_MINUS_##level;                               \
                                                           \
    DEF_OPC(OPC_ISHL, level)                               \
        SHIFT_OP_##level(int, <<);                         \
                                                           \
    DEF_OPC(OPC_ISHR, level)                               \
        SHIFT_OP_##level(int, >>);                         \
                                                           \
    DEF_OPC(OPC_IUSHR, level)                              \
        SHIFT_OP_##level(unsigned int, >>);                \
                                                           \
    DEF_OPC(OPC_IF_ACMPEQ, level)                          \
    DEF_OPC(OPC_IF_ICMPEQ, level)                          \
        IF_ICMP_##level(==);                               \
                                                           \
    DEF_OPC(OPC_IF_ACMPNE, level)                          \
    DEF_OPC(OPC_IF_ICMPNE, level)                          \
        IF_ICMP_##level(!=);                               \
                                                           \
    DEF_OPC(OPC_IF_ICMPLT, level)                          \
        IF_ICMP_##level(<);                                \
                                                           \
    DEF_OPC(OPC_IF_ICMPGE, level)                          \
        IF_ICMP_##level(>=);                               \
                                                           \
    DEF_OPC(OPC_IF_ICMPGT, level)                          \
        IF_ICMP_##level(>);                                \
                                                           \
    DEF_OPC(OPC_IF_ICMPLE, level)                          \
        IF_ICMP_##level(<=);                               \
                                                           \
    DEF_OPC(OPC_IFNE, level)                               \
    DEF_OPC(OPC_IFNONNULL, level)                          \
        IF_##level(!=);                                    \
                                                           \
    DEF_OPC(OPC_IFEQ, level)                               \
    DEF_OPC(OPC_IFNULL, level)                             \
        IF_##level(==);                                    \
                                                           \
    DEF_OPC(OPC_IFLT, level)                               \
        IF_##level(<);                                     \
                                                           \
    DEF_OPC(OPC_IFGE, level)                               \
        IF_##level(>=);                                    \
                                                           \
    DEF_OPC(OPC_IFGT, level)                               \
        IF_##level(>);                                     \
                                                           \
    DEF_OPC(OPC_IFLE, level)                               \
        IF_##level(<=);                                    \
                                                           \
    DEF_OPC(OPC_IINC, level)                               \
        lvars[IINC_LVAR_IDX(pc)] += IINC_DELTA(pc);        \
        DISPATCH(level, 3);                                \
                                                           \
    DEF_OPC(OPC_POP, level)                                \
        POP1_##level;                                      \
                                                           \
    DEF_OPC(OPC_POP2, level)                               \
        ostack -= 2 - level;                               \
        DISPATCH(0, 1);                                    \
                                                           \
    DEF_OPC(OPC_DUP, level)                                \
        DUP_##level;                                       \
                                                           \
    DEF_OPC(OPC_IRETURN, level)                            \
    DEF_OPC(OPC_ARETURN, level)                            \
    DEF_OPC(OPC_FRETURN, level)                            \
        RETURN_##level;                                    \
                                                           \
    DEF_OPC(OPC_RETURN, level)                             \
        goto methodReturn;                                 \
                                                           \
    DEF_OPC(OPC_GETSTATIC_QUICK, level)                    \
        PUSH_##level(RESOLVED_FIELD(pc)->static_value, 3); \
                                                           \
    DEF_OPC(OPC_PUTSTATIC_QUICK, level)                    \
        POP_##level(RESOLVED_FIELD(pc)->static_value, 3);  \
                                                           \
    DEF_OPC(OPC_GETFIELD_THIS, level)                      \
        GETFIELD_THIS(level);                              \
                                                           \
    DEF_OPC(OPC_GETFIELD_QUICK, level)                     \
        GETFIELD_QUICK_##level;

#define ZERO_DIVISOR_CHECK(value)                          \
    if(value == 0)                                         \
        THROW_EXCEPTION("java/lang/ArithmeticException",   \
                        "division by zero");

#define ZERO_DIVISOR_CHECK_0                               \
    ZERO_DIVISOR_CHECK((int)ostack[-1]);

#define ZERO_DIVISOR_CHECK_1                               \
    ZERO_DIVISOR_CHECK((int)cache.i.v1);

#define ZERO_DIVISOR_CHECK_2                               \
    ZERO_DIVISOR_CHECK((int)cache.i.v2);

#ifdef USE_CACHE
#define PUSH_0(value, ins_len)                             \
    cache.i.v1 = value;                                    \
    DISPATCH(1, ins_len);
#else
#define PUSH_0(value, ins_len)                             \
    *ostack++ = value;                                     \
    DISPATCH(0, ins_len);
#endif

#define PUSH_1(value, ins_len)                             \
    cache.i.v2 = value;                                    \
    DISPATCH(2, ins_len);

#define PUSH_2(value, ins_len)                             \
    *ostack++ = cache.i.v1;                                \
    cache.i.v1 = cache.i.v2;                               \
    cache.i.v2 = value;                                    \
    DISPATCH(2, ins_len);

#define POP_0(dest, ins_len)                               \
    dest = *--ostack;                                      \
    DISPATCH(0, ins_len);

#define POP_1(dest, ins_len)                               \
    dest = cache.i.v1;                                     \
    DISPATCH(0, ins_len);

#define POP_2(dest, ins_len)                               \
    dest = cache.i.v2;                                     \
    DISPATCH(1, ins_len);

#define POP1_0                                             \
    ostack--;                                              \
    DISPATCH(0, 1);

#define POP1_1                                             \
    DISPATCH(0, 1);

#define POP1_2                                             \
    DISPATCH(1, 1);

#define DUP_0                                              \
    PUSH_0(ostack[-1], 1);

#define DUP_1                                              \
    PUSH_1(cache.i.v1, 1);

#define DUP_2                                              \
    PUSH_2(cache.i.v2, 1);

#define RETURN_0                                           \
    *lvars++ = *--ostack;                                  \
    goto methodReturn;

#define RETURN_1                                           \
    *lvars++ = cache.i.v1;                                 \
    goto methodReturn;

#define RETURN_2                                           \
    *lvars++ = cache.i.v2;                                 \
    goto methodReturn;

#define GETFIELD_QUICK_0                                   \
{                                                          \
    Object *obj = (Object *)*--ostack;                     \
    NULL_POINTER_CHECK(obj);                               \
    PUSH_0(INST_DATA(obj)[SINGLE_INDEX(pc)], 3);           \
}

#define GETFIELD_QUICK_1                                   \
{                                                          \
    Object *obj = (Object *)cache.i.v1;                    \
    NULL_POINTER_CHECK(obj);                               \
    PUSH_0(INST_DATA(obj)[SINGLE_INDEX(pc)], 3);           \
}

#define GETFIELD_QUICK_2                                   \
{                                                          \
    Object *obj = (Object *)cache.i.v2;                    \
    NULL_POINTER_CHECK(obj);                               \
    PUSH_1(INST_DATA(obj)[SINGLE_INDEX(pc)], 3);           \
}

#define UNARY_MINUS_0                                      \
{                                                          \
    int v = (int)*--ostack;                                \
    PUSH_0(-v, 1);                                         \
}

#define UNARY_MINUS_1                                      \
    PUSH_0(-(int)cache.i.v1, 1);

#define UNARY_MINUS_2                                      \
    PUSH_1(-(int)cache.i.v2, 1);

#define BINARY_OP_0(OP)                                    \
    ostack -= 2;                                           \
    PUSH_0((int)ostack[0] OP (int)ostack[1], 1);

#define BINARY_OP_1(OP)                                    \
    PUSH_0((int)*--ostack OP (int)cache.i.v1, 1);

#define BINARY_OP_2(OP)                                    \
    PUSH_0((int)cache.i.v1 OP (int)cache.i.v2, 1);

#define SHIFT_OP_0(TYPE, OP)                               \
    ostack -= 2;                                           \
    PUSH_0((TYPE)ostack[0] OP (ostack[1] & 0x1f), 1);

#define SHIFT_OP_1(TYPE, OP)                               \
    PUSH_0((TYPE)*--ostack OP (cache.i.v1 & 0x1f), 1);

#define SHIFT_OP_2(TYPE, OP)                               \
    PUSH_0((TYPE)cache.i.v1 OP (cache.i.v2 & 0x1f), 1);

#define IF_ICMP_0(COND)                                    \
    ostack -= 2;                                           \
    BRANCH((int)ostack[0] COND (int)ostack[1]);

#define IF_ICMP_1(COND)                                    \
    BRANCH((int)*--ostack COND (int)cache.i.v1);

#define IF_ICMP_2(COND)                                    \
    BRANCH((int)cache.i.v1 COND (int)cache.i.v2);

#define IF_0(COND)                                         \
    BRANCH((int)*--ostack COND 0);

#define IF_1(COND)                                         \
    BRANCH((int)cache.i.v1 COND 0);

#ifdef USE_CACHE
#define IF_2(COND)                                         \
    *ostack++ = cache.i.v1;                                \
    BRANCH((int)cache.i.v2 COND 0);
#else
#define IF_2(COND)                                         \
    BRANCH((int)cache.i.v2 COND 0);
#endif

#ifdef DIRECT
#define ALOAD_THIS(level)

#define GETFIELD_THIS(level)                               \
    PUSH_##level(INST_DATA(this)[pc->operand.i], 4);
#else /* DIRECT */
#define ALOAD_THIS(level)                                  \
    if(pc[1] == OPC_GETFIELD_QUICK) {                      \
        OPCODE_REWRITE(OPC_GETFIELD_THIS);                 \
        DISPATCH(level, 0);                                \
    }

#define GETFIELD_THIS(level)                               \
    PUSH_##level(INST_DATA(this)[pc[2]], 4);
#endif /* DIRECT */

    MULTI_LEVEL_OPCODES(0);

#ifdef USE_CACHE
    MULTI_LEVEL_OPCODES(1);
    MULTI_LEVEL_OPCODES(2);

#define DEF_OPC_012(opcode)           \
    DEF_OPC(opcode, 0)                \
        cache.i.v1 = *--ostack;       \
                                      \
    DEF_OPC(opcode, 1)                \
        cache.i.v2 = cache.i.v1;      \
        cache.i.v1 = *--ostack;       \
                                      \
    DEF_OPC(opcode, 2)

#define DEF_OPC_012_2(op1, op2)       \
    DEF_OPC(op1, 0)                   \
    DEF_OPC(op2, 0)                   \
        cache.i.v1 = *--ostack;       \
                                      \
    DEF_OPC(op1, 1)                   \
    DEF_OPC(op2, 1)                   \
        cache.i.v2 = cache.i.v1;      \
        cache.i.v1 = *--ostack;       \
                                      \
    DEF_OPC(op1, 2)                   \
    DEF_OPC(op2, 2)

#define DEF_OPC_012_3(op1, op2, op3)  \
    DEF_OPC(op1, 0)                   \
    DEF_OPC(op2, 0)                   \
    DEF_OPC(op3, 0)                   \
        cache.i.v1 = *--ostack;       \
                                      \
    DEF_OPC(op1, 1)                   \
    DEF_OPC(op2, 1)                   \
    DEF_OPC(op3, 1)                   \
        cache.i.v2 = cache.i.v1;      \
        cache.i.v1 = *--ostack;       \
                                      \
    DEF_OPC(op1, 2)                   \
    DEF_OPC(op2, 2)                   \
    DEF_OPC(op3, 2)

#define DEF_OPC_210(opcode)           \
    DEF_OPC(opcode, 2)                \
        *ostack++ = cache.i.v1;       \
        cache.i.v1 = cache.i.v2;      \
                                      \
    DEF_OPC(opcode, 1)                \
        *ostack++ = cache.i.v1;       \
                                      \
    DEF_OPC(opcode, 0)

#define DEF_OPC_210_2(op1, op2)       \
    DEF_OPC(op1, 2)                   \
    DEF_OPC(op2, 2)                   \
        *ostack++ = cache.i.v1;       \
        cache.i.v1 = cache.i.v2;      \
                                      \
    DEF_OPC(op1, 1)                   \
    DEF_OPC(op2, 1)                   \
        *ostack++ = cache.i.v1;       \
                                      \
    DEF_OPC(op1, 0)                   \
    DEF_OPC(op2, 0)

#else /* USE_CACHE */

#define DEF_OPC_012(opcode)           \
    DEF_OPC(opcode, 0)

#define DEF_OPC_012_2(op1, op2)       \
    DEF_OPC(op1, 0)                   \
    DEF_OPC(op2, 0)

#define DEF_OPC_012_3(op1, op2, op3)  \
    DEF_OPC(op1, 0)                   \
    DEF_OPC(op2, 0)                   \
    DEF_OPC(op3, 0)

#define DEF_OPC_210(opcode)           \
    DEF_OPC(opcode, 0)

#define DEF_OPC_210_2(op1, op2)       \
    DEF_OPC(op1, 0)                   \
    DEF_OPC(op2, 0)

#endif /* USE_CACHE */

    DEF_OPC_210(OPC_NOP)
        DISPATCH(0, 1);

#ifdef USE_CACHE
#define PUSH_LONG(value, ins_len) \
    cache.l = value;              \
    DISPATCH(2, ins_len);
#else
#define PUSH_LONG(value, ins_len) \
    *(u8*)ostack = value;         \
    ostack += 2;                  \
    DISPATCH(0, ins_len);
#endif
    
    DEF_OPC_210_2(
            OPC_LCONST_0,
            OPC_DCONST_0)
        PUSH_LONG(0, 1);

    DEF_OPC_210(OPC_DCONST_1)
        PUSH_LONG(DOUBLE_1_BITS, 1);

    DEF_OPC_210(OPC_LCONST_1)
        PUSH_LONG(1, 1);

    DEF_OPC_210(OPC_LDC2_W)
        PUSH_LONG(CP_LONG(cp, DOUBLE_INDEX(pc)), 3);

    DEF_OPC_210_2(
            OPC_LLOAD,
            OPC_DLOAD)
        PUSH_LONG(*(u8*)(&lvars[SINGLE_INDEX(pc)]), 2);

    DEF_OPC_210_2(
            OPC_LLOAD_0,
            OPC_DLOAD_0)
        PUSH_LONG(*(u8*)(&lvars[0]), 1);

    DEF_OPC_210_2(
            OPC_LLOAD_1,
            OPC_DLOAD_1)
        PUSH_LONG(*(u8*)(&lvars[1]), 1);

    DEF_OPC_210_2(
            OPC_LLOAD_2,
            OPC_DLOAD_2)
        PUSH_LONG(*(u8*)(&lvars[2]), 1);

    DEF_OPC_210_2(
            OPC_LLOAD_3,
            OPC_DLOAD_3)
        PUSH_LONG(*(u8*)(&lvars[3]), 1);

#ifdef USE_CACHE
#define POP_LONG(dest, ins_len) \
    dest = cache.l;             \
    DISPATCH(0, ins_len);
#else
#define POP_LONG(dest, ins_len) \
    ostack -= 2;                \
    dest = *(u8*)ostack;        \
    DISPATCH(0, ins_len);
#endif

    DEF_OPC_012_2(
            OPC_LSTORE,
            OPC_DSTORE)
        POP_LONG(*(u8*)(&lvars[SINGLE_INDEX(pc)]), 2);

    DEF_OPC_012_2(
            OPC_LSTORE_0,
            OPC_DSTORE_0)
        POP_LONG(*(u8*)(&lvars[0]), 1);

    DEF_OPC_012_2(
            OPC_LSTORE_1,
            OPC_DSTORE_1)
        POP_LONG(*(u8*)(&lvars[1]), 1);

    DEF_OPC_012_2(
            OPC_LSTORE_2,
            OPC_DSTORE_2)
        POP_LONG(*(u8*)(&lvars[2]), 1);

    DEF_OPC_012_2(
            OPC_LSTORE_3,
            OPC_DSTORE_3)
        POP_LONG(*(u8*)(&lvars[3]), 1);

#ifdef USE_CACHE
#define ARRAY_LOAD_IDX cache.i.v2
#define ARRAY_LOAD_ARY cache.i.v1
#else
#define ARRAY_LOAD_IDX *--ostack
#define ARRAY_LOAD_ARY *--ostack
#endif

#define ARRAY_LOAD(TYPE)                              \
{                                                     \
    int idx = ARRAY_LOAD_IDX;                         \
    Object *array = (Object *)ARRAY_LOAD_ARY;         \
                                                      \
    NULL_POINTER_CHECK(array);                        \
    ARRAY_BOUNDS_CHECK(array, idx);                   \
    PUSH_0(((TYPE *)ARRAY_DATA(array))[idx], 1);      \
}

    DEF_OPC_012_2(
            OPC_IALOAD,
            OPC_FALOAD)
        ARRAY_LOAD(int);

    DEF_OPC_012(OPC_AALOAD)
        ARRAY_LOAD(uintptr_t);

    DEF_OPC_012(OPC_BALOAD)
        ARRAY_LOAD(signed char);

    DEF_OPC_012(OPC_CALOAD)
        ARRAY_LOAD(unsigned short);

    DEF_OPC_012(OPC_SALOAD)
        ARRAY_LOAD(short);

    DEF_OPC_012_2(
            OPC_LALOAD,
            OPC_DALOAD)
    {
        int idx = ARRAY_LOAD_IDX;
        Object *array = (Object *)ARRAY_LOAD_ARY;

        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);
        PUSH_LONG(((u8 *)ARRAY_DATA(array))[idx], 1);
    }

#ifdef USE_CACHE
#define ARRAY_STORE_VAL cache.i.v2
#define ARRAY_STORE_IDX cache.i.v1
#else
#define ARRAY_STORE_VAL *--ostack
#define ARRAY_STORE_IDX *--ostack
#endif

#define ARRAY_STORE(TYPE)                         \
{                                                 \
    int val = ARRAY_STORE_VAL;                    \
    int idx = ARRAY_STORE_IDX;                    \
    Object *array = (Object *)*--ostack;          \
                                                  \
    NULL_POINTER_CHECK(array);                    \
    ARRAY_BOUNDS_CHECK(array, idx);               \
    ((TYPE *)ARRAY_DATA(array))[idx] = val;       \
    DISPATCH(0, 1);                               \
}

    DEF_OPC_012_2(
            OPC_IASTORE,
            OPC_FASTORE)
        ARRAY_STORE(int);

    DEF_OPC_012(OPC_BASTORE)
        ARRAY_STORE(char);

    DEF_OPC_012_2(
            OPC_CASTORE,
            OPC_SASTORE)
        ARRAY_STORE(short);

    DEF_OPC_012(OPC_AASTORE)
    { 
        Object *obj = (Object*)ARRAY_STORE_VAL;
        int idx = ARRAY_STORE_IDX;
        Object *array = (Object *)*--ostack;

        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);

        if((obj != NULL) && !arrayStoreCheck(array->class, obj->class))
            THROW_EXCEPTION("java/lang/ArrayStoreException", NULL);

        ((Object**)ARRAY_DATA(array))[idx] = obj;
        DISPATCH(0, 1);
    }

#ifdef USE_CACHE
    DEF_OPC_012_2(
            OPC_LASTORE,
            OPC_DASTORE)
    {
        int idx = ostack[-1];
        Object *array = (Object *)ostack[-2];

        ostack -= 2;
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);

        ((u8 *)ARRAY_DATA(array))[idx] = cache.l;
        DISPATCH(0, 1);
    }
#else
    DEF_OPC_012_2(
            OPC_LASTORE,
            OPC_DASTORE)
    {
        int idx = ostack[-3];
        Object *array = (Object *)ostack[-4];

        ostack -= 4;
        NULL_POINTER_CHECK(array);
        ARRAY_BOUNDS_CHECK(array, idx);

        ((u8 *)ARRAY_DATA(array))[idx] = *(u8*)&ostack[2];
        DISPATCH(0, 1);
    }
#endif

#ifdef USE_CACHE
    DEF_OPC_012(OPC_DUP_X1)
        *ostack++ = cache.i.v2;
        DISPATCH(2, 1);

    DEF_OPC_012(OPC_DUP_X2)
        *ostack++ = ostack[-1];
        ostack[-2] = cache.i.v2;
        DISPATCH(2, 1);

    DEF_OPC_012(OPC_DUP2)
        *ostack++ = cache.i.v1;
        *ostack++ = cache.i.v2;
        DISPATCH(2, 1);

    DEF_OPC_012(OPC_DUP2_X1)
        ostack[0]  = cache.i.v2;
        ostack[1]  = ostack[-1];
        ostack[-1] = cache.i.v1;
        ostack += 2;
        DISPATCH(2, 1);

    DEF_OPC_012(OPC_DUP2_X2)
        ostack[0] = ostack[-2];
        ostack[1] = ostack[-1];
        ostack[-2] = cache.i.v1;
        ostack[-1] = cache.i.v2;
        ostack += 2;
        DISPATCH(2, 1);

    DEF_OPC_012(OPC_SWAP)
    {
        uintptr_t word1 = cache.i.v1;
        cache.i.v1 = cache.i.v2;
        cache.i.v2 = word1;
        DISPATCH(2, 1);
    }
#else /* USE_CACHE */
    DEF_OPC_012(OPC_DUP_X1)
    {
        uintptr_t word1 = ostack[-1];
        uintptr_t word2 = ostack[-2];
        ostack[-2] = word1;
        ostack[-1] = word2;
        *ostack++ = word1;
        DISPATCH(0, 1);
    }

    DEF_OPC_012(OPC_DUP_X2)
    {
        uintptr_t word1 = ostack[-1];
        uintptr_t word2 = ostack[-2];
        uintptr_t word3 = ostack[-3];
        ostack[-3] = word1;
        ostack[-2] = word3;
        ostack[-1] = word2;
        *ostack++ = word1;
        DISPATCH(0, 1);
    }

    DEF_OPC_012(OPC_DUP2)
        ostack[0] = ostack[-2];
        ostack[1] = ostack[-1];
        ostack += 2;
        DISPATCH(0, 1);

    DEF_OPC_012(OPC_DUP2_X1)
    {
        uintptr_t word1 = ostack[-1];
        uintptr_t word2 = ostack[-2];
        uintptr_t word3 = ostack[-3];
        ostack[-3] = word2;
        ostack[-2] = word1;
        ostack[-1] = word3;
        ostack[0]  = word2;
        ostack[1]  = word1;
        ostack += 2;
        DISPATCH(0, 1);
    }

    DEF_OPC_012(OPC_DUP2_X2)
    {
        uintptr_t word1 = ostack[-1];
        uintptr_t word2 = ostack[-2];
        uintptr_t word3 = ostack[-3];
        uintptr_t word4 = ostack[-4];
        ostack[-4] = word2;
        ostack[-3] = word1;
        ostack[-2] = word4;
        ostack[-1] = word3;
        ostack[0]  = word2;
        ostack[1]  = word1;
        ostack += 2;
        DISPATCH(0, 1);
    }

    DEF_OPC_012(OPC_SWAP)
    {
        uintptr_t word1 = ostack[-1];
        ostack[-1] = ostack[-2];
        ostack[-2] = word1;
        DISPATCH(0, 1)
    }
#endif /* USE_CACHE */

#define BINARY_OP_fp(TYPE, OP)                  \
    *(TYPE *)&ostack[-sizeof(TYPE)/4 * 2] =     \
        *(TYPE *)&ostack[-sizeof(TYPE)/4 * 2] OP\
        *(TYPE *)&ostack[-sizeof(TYPE)/4];      \
    ostack -= sizeof(TYPE)/4;                   \
    DISPATCH(0, 1);

    DEF_OPC_210(OPC_FADD)
        BINARY_OP_fp(float, +);

    DEF_OPC_210(OPC_DADD)
        BINARY_OP_fp(double, +);

    DEF_OPC_210(OPC_FSUB)
        BINARY_OP_fp(float, -);

    DEF_OPC_210(OPC_DSUB)
        BINARY_OP_fp(double, -);

    DEF_OPC_210(OPC_FMUL)
        BINARY_OP_fp(float, *);

    DEF_OPC_210(OPC_DMUL)
        BINARY_OP_fp(double, *);

    DEF_OPC_210(OPC_FDIV)
        BINARY_OP_fp(float, /);

    DEF_OPC_210(OPC_DDIV)
        BINARY_OP_fp(double, /);

#ifdef USE_CACHE
#define BINARY_OP_long(OP)                               \
    cache.l = *(long long*)&ostack[-2] OP cache.l;       \
    ostack -= 2;                                         \
    DISPATCH(2, 1);

#define ZERO_DIVISOR_CHECK_long                          \
    ZERO_DIVISOR_CHECK(cache.l);
#else
#define BINARY_OP_long(OP)                               \
    BINARY_OP_fp(long long, OP)

#define ZERO_DIVISOR_CHECK_long                          \
    ZERO_DIVISOR_CHECK(*(u8*)&ostack[-2]);
#endif

    DEF_OPC_012(OPC_LADD)
        BINARY_OP_long(+);

    DEF_OPC_012(OPC_LSUB)
        BINARY_OP_long(-);

    DEF_OPC_012(OPC_LMUL)
        BINARY_OP_long(*);

    DEF_OPC_012(OPC_LDIV)
        ZERO_DIVISOR_CHECK_long;
        BINARY_OP_long(/);

    DEF_OPC_012(OPC_LREM)
        ZERO_DIVISOR_CHECK_long;
        BINARY_OP_long(%);

    DEF_OPC_012(OPC_LAND)
        BINARY_OP_long(&);

    DEF_OPC_012(OPC_LOR)
        BINARY_OP_long(|);

    DEF_OPC_012(OPC_LXOR)
        BINARY_OP_long(^);

#ifdef USE_CACHE
#define SHIFT_OP_long(TYPE, OP)       \
{                                     \
    int shift = cache.i.v2 & 0x3f;    \
    cache.i.v2 = cache.i.v1;          \
    cache.i.v1 = *--ostack;           \
    cache.l = (TYPE)cache.l OP shift; \
    DISPATCH(2, 1);                   \
}
#else
#define SHIFT_OP_long(TYPE, OP)       \
{                                     \
    int shift = *--ostack & 0x3f;     \
    *(TYPE*)&ostack[-2] =             \
        *(TYPE*)&ostack[-2] OP shift; \
    DISPATCH(0, 1);                   \
}
#endif

    DEF_OPC_012(OPC_LSHL)
        SHIFT_OP_long(long long, <<);

    DEF_OPC_012(OPC_LSHR)
        SHIFT_OP_long(long long, >>);

    DEF_OPC_012(OPC_LUSHR)
        SHIFT_OP_long(unsigned long long, >>);

    DEF_OPC_210(OPC_FREM)
    {
        float v2 = *(float *)&ostack[-1];
        float v1 = *(float *)&ostack[-2];

        *(float *)&ostack[-2] = fmod(v1, v2);
        ostack -= 1;
        DISPATCH(0, 1);
    }

    DEF_OPC_210(OPC_DREM)
    {
        double v2 = *(double *)&ostack[-2];
        double v1 = *(double *)&ostack[-4];

        *(double *)&ostack[-4] = fmod(v1, v2);
        ostack -= 2;
        DISPATCH(0, 1);
    }

#define UNARY_MINUS(TYPE)                    \
    *(TYPE*)&ostack[-sizeof(TYPE)/4] =       \
          -*(TYPE*)&ostack[-sizeof(TYPE)/4]; \
    DISPATCH(0, 1);

    DEF_OPC_210(OPC_LNEG)
        UNARY_MINUS(long long);

    DEF_OPC_210(OPC_FNEG)
        UNARY_MINUS(float);

    DEF_OPC_210(OPC_DNEG)
        UNARY_MINUS(double);

    DEF_OPC_210(OPC_I2L)
        ostack -= 1;
        PUSH_LONG((int)*ostack, 1);

    DEF_OPC_012(OPC_L2I)
    {
       long long l;
#ifdef USE_CACHE
        l = cache.l;
#else
        ostack -= 2;
        l = *(long long*)ostack;
#endif
        PUSH_0((int)l, 1);
    }

#define OPC_int2fp(DEST_TYPE)            \
    ostack -= 1;                         \
    *(DEST_TYPE *)ostack =               \
              (DEST_TYPE)(int)*ostack;   \
    ostack += sizeof(DEST_TYPE)/4;       \
    DISPATCH(0, 1);

    DEF_OPC_210(OPC_I2F)
        OPC_int2fp(float);

    DEF_OPC_210(OPC_I2D)
        OPC_int2fp(double);

#define OPC_X2Y(SRC_TYPE, DEST_TYPE)     \
{                                        \
    SRC_TYPE v;                          \
    ostack -= sizeof(SRC_TYPE)/4;        \
    v = *(SRC_TYPE *)ostack;             \
    *(DEST_TYPE *)ostack = (DEST_TYPE)v; \
    ostack += sizeof(DEST_TYPE)/4;       \
    DISPATCH(0, 1);                      \
}

    DEF_OPC_210(OPC_L2F)
        OPC_X2Y(long long, float);

    DEF_OPC_210(OPC_L2D)
        OPC_X2Y(long long, double);

    DEF_OPC_210(OPC_F2D)
        OPC_X2Y(float, double);

    DEF_OPC_210(OPC_D2F)
        OPC_X2Y(double, float);

#define OPC_fp2int(SRC_TYPE)            \
{                                       \
    int res;                            \
    SRC_TYPE value;                     \
    ostack -= sizeof(SRC_TYPE)/4;       \
    value = *(SRC_TYPE *)ostack;        \
                                        \
    if(value >= (SRC_TYPE)INT_MAX)      \
        res = INT_MAX;                  \
    else if(value <= (SRC_TYPE)INT_MIN) \
        res = INT_MIN;                  \
    else if(value != value)             \
        res = 0;                        \
    else                                \
        res = (int) value;              \
                                        \
    PUSH_0(res, 1);                     \
}

    DEF_OPC_210(OPC_F2I)
        OPC_fp2int(float);

    DEF_OPC_210(OPC_D2I)
        OPC_fp2int(double);

#define OPC_fp2long(SRC_TYPE)              \
{                                          \
    long long res;                         \
    SRC_TYPE value;                        \
    ostack -= sizeof(SRC_TYPE)/4;          \
    value = *(SRC_TYPE *)ostack;           \
                                           \
    if(value >= (SRC_TYPE)LLONG_MAX)       \
        res = LLONG_MAX;                   \
    else if(value <= (SRC_TYPE)LLONG_MIN)  \
        res = LLONG_MIN;                   \
    else if(value != value)                \
        res = 0;                           \
    else                                   \
        res = (long long) value;           \
                                           \
    PUSH_LONG(res, 1);                     \
}

    DEF_OPC_210(OPC_F2L)
        OPC_fp2long(float);

    DEF_OPC_210(OPC_D2L)
        OPC_fp2long(double);

    DEF_OPC_210(OPC_I2B)
    {
        signed char v = *--ostack & 0xff;
        PUSH_0(v, 1);
    }

    DEF_OPC_210(OPC_I2C)
    {
        int v = *--ostack & 0xffff;
        PUSH_0(v, 1);
    }

    DEF_OPC_210(OPC_I2S)
    {
        signed short v = *--ostack & 0xffff;
        PUSH_0((int) v, 1);
    }

#ifdef USE_CACHE
    DEF_OPC_012(OPC_LCMP)
    {
        long long v1 = *(long long*)&ostack[-2];
        int r = (v1 == cache.l) ? 0 : ((v1 < cache.l) ? -1 : 1);
        cache.i.v1 = r;
        ostack -= 2;
        DISPATCH(1, 1);
    }
#else
    DEF_OPC_012(OPC_LCMP)
    {
        long long v2 = *(long long*)&ostack[-2];
        long long v1 = *(long long*)&ostack[-4];
        ostack[-4] = (v1 == v2) ? 0 : ((v1 < v2) ? -1 : 1);
        ostack -= 3;
        DISPATCH(0, 1);
    }
#endif

#define FCMP(TYPE, isNan)                                 \
{                                                         \
    int res;                                              \
    TYPE v1, v2;                                          \
    ostack -= sizeof(TYPE)/4; v2 = *(TYPE *)ostack;       \
    ostack -= sizeof(TYPE)/4; v1 = *(TYPE *)ostack;       \
    if(v1 == v2)                                          \
        res = 0;                                          \
    else if(v1 < v2)                                      \
        res = -1;                                         \
    else if(v1 > v2)                                      \
         res = 1;                                         \
    else                                                  \
         res = isNan;                                     \
    PUSH_0(res, 1);                                       \
}

    DEF_OPC_210(OPC_DCMPG)
        FCMP(double, 1);

    DEF_OPC_210(OPC_DCMPL)
        FCMP(double, -1);

    DEF_OPC_210(OPC_FCMPG)
        FCMP(float, 1);

    DEF_OPC_210(OPC_FCMPL)
        FCMP(float, -1);

#ifdef DIRECT
    DEF_OPC_210_2(
            OPC_GOTO,
            OPC_GOTO_W)
#else
    DEF_OPC_210(OPC_GOTO)
#endif
        BRANCH(TRUE);

#ifdef DIRECT
    DEF_OPC_210_2(
            OPC_JSR,
            OPC_JSR_W)
#else
    DEF_OPC_210(OPC_JSR)
#endif
        *ostack++ = (uintptr_t)pc;
        BRANCH(TRUE);

    DEF_OPC_210(OPC_RET)
        pc = (CodePntr)lvars[SINGLE_INDEX(pc)];
        DISPATCH_RET(3);

    DEF_OPC_012_2(
            OPC_LRETURN,
            OPC_DRETURN)
#ifdef USE_CACHE
        *(u8*)lvars = cache.l;
#else
        ostack -= 2;
        *(u8*)lvars = *(u8*)ostack;
#endif
        lvars += 2;
        goto methodReturn;

    DEF_OPC_210(OPC_ARRAYLENGTH)
    {
        Object *array = (Object *)*--ostack;

        NULL_POINTER_CHECK(array);
        PUSH_0(ARRAY_LEN(array), 1);
    }

    DEF_OPC_210(OPC_ATHROW)
    {
        Object *obj = (Object *)ostack[-1];
        frame->last_pc = pc;
        NULL_POINTER_CHECK(obj);
                
        ee->exception = obj;
        goto throwException;
    }

    DEF_OPC_210(OPC_NEWARRAY)
    {
        int type = ARRAY_TYPE(pc);
        int count = *--ostack;
        Object *obj;

        frame->last_pc = pc;
        if((obj = allocTypeArray(type, count)) == NULL)
            goto throwException;

        PUSH_0((uintptr_t)obj, 2);
    }

    DEF_OPC_210(OPC_MONITORENTER)
    {
        Object *obj = (Object *)*--ostack;
        NULL_POINTER_CHECK(obj);
        objectLock(obj);
        DISPATCH(0, 1);
    }

    DEF_OPC_210(OPC_MONITOREXIT)
    {
        Object *obj = (Object *)*--ostack;
        NULL_POINTER_CHECK(obj);
        objectUnlock(obj);
        DISPATCH(0, 1);
    }

#ifdef DIRECT
    DEF_OPC_RW(OPC_LDC)
    {
        int idx, cache;
        Operand operand;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_LDC, idx, cache);

        frame->last_pc = pc;

        operand.u = resolveSingleConstant(mb->class, idx);

        if(exceptionOccured0(ee))
            goto throwException;

        if(CP_TYPE(cp, idx) == CONSTANT_ResolvedClass ||
           CP_TYPE(cp, idx) == CONSTANT_ResolvedString) {
            operand.i = idx;
            OPCODE_REWRITE(OPC_LDC_W_QUICK, cache, operand);
        } else
            OPCODE_REWRITE(OPC_LDC_QUICK, cache, operand);

        REDISPATCH
    }

    DEF_OPC_210(OPC_TABLESWITCH)
    {
        SwitchTable *table = (SwitchTable*)pc->operand.pntr;
        int index = *--ostack;

        if(index < table->low || index > table->high)
            pc = table->deflt;
        else
            pc = table->entries[index - table->low];

        DISPATCH_FIRST
    }

    DEF_OPC_210(OPC_LOOKUPSWITCH)
    {
        LookupTable *table = (LookupTable*)pc->operand.pntr;
        int key = *--ostack;
        int i;

        for(i = 0; (i < table->num_entries) && (key != table->entries[i].key); i++);

        pc = (i == table->num_entries ? table->deflt
                                      : table->entries[i].handler);
        DISPATCH_FIRST
    }

    DEF_OPC_RW(OPC_GETSTATIC) 
    {
        int idx, cache;
        FieldBlock *fb;
        Operand operand;
               
        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_GETSTATIC, idx, cache);

        frame->last_pc = pc;
        fb = resolveField(mb->class, idx);

        if(exceptionOccured0(ee))
            goto throwException;

        operand.pntr = fb;
        OPCODE_REWRITE(((*fb->type == 'J') || (*fb->type == 'D') ?
                 OPC_GETSTATIC2_QUICK : OPC_GETSTATIC_QUICK), cache, operand);

        REDISPATCH
    }

    DEF_OPC_RW(OPC_PUTSTATIC) 
    {
        int idx, cache;
        FieldBlock *fb;
        Operand operand;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_PUTSTATIC, idx, cache);

        frame->last_pc = pc;
        fb = resolveField(mb->class, idx);

        if(exceptionOccured0(ee))
            goto throwException;

        operand.pntr = fb;
        OPCODE_REWRITE(((*fb->type == 'J') || (*fb->type == 'D') ?
                 OPC_PUTSTATIC2_QUICK : OPC_PUTSTATIC_QUICK), cache, operand);

        REDISPATCH
    }

    DEF_OPC_RW(OPC_GETFIELD)
    {
        int idx, cache;
        FieldBlock *fb;
        Operand operand;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_GETFIELD, idx, cache);

        frame->last_pc = pc;
        fb = resolveField(mb->class, idx);

        if(exceptionOccured0(ee))
            goto throwException;

        operand.i = fb->offset;
        OPCODE_REWRITE(((*fb->type == 'J') || (*fb->type == 'D') ? 
                 OPC_GETFIELD2_QUICK : OPC_GETFIELD_QUICK), cache, operand);

        REDISPATCH
    }

    DEF_OPC_RW(OPC_PUTFIELD)
    {
        int idx, cache;
        FieldBlock *fb;
        Operand operand;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_PUTFIELD, idx, cache);

        frame->last_pc = pc;
        fb = resolveField(mb->class, idx);

        if(exceptionOccured0(ee))
            goto throwException;

        operand.i = fb->offset;
        OPCODE_REWRITE(((*fb->type == 'J') || (*fb->type == 'D') ? 
                 OPC_PUTFIELD2_QUICK : OPC_PUTFIELD_QUICK), cache, operand);

        REDISPATCH
    }

    DEF_OPC_RW(OPC_INVOKEVIRTUAL)
    {
        int idx, cache;
        Operand operand;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_INVOKEVIRTUAL, idx, cache);

        frame->last_pc = pc;
        new_mb = resolveMethod(mb->class, idx);
 
        if(exceptionOccured0(ee))
            goto throwException;

        operand.uu.u1 = new_mb->args_count;
        operand.uu.u2 = new_mb->method_table_index;
        OPCODE_REWRITE(OPC_INVOKEVIRTUAL_QUICK, cache, operand);
        REDISPATCH
    }

    DEF_OPC_RW(OPC_INVOKESPECIAL)
    {
        int idx, cache;
        Operand operand;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_INVOKESPECIAL, idx, cache);

        frame->last_pc = pc;
        new_mb = resolveMethod(mb->class, idx);
 
        if(exceptionOccured0(ee))
            goto throwException;

        /* Check if invoking a super method... */
        if((CLASS_CB(mb->class)->access_flags & ACC_SUPER) &&
              ((new_mb->access_flags & ACC_PRIVATE) == 0) && (new_mb->name[0] != '<')) {

            operand.i = new_mb->method_table_index;
            OPCODE_REWRITE(OPC_INVOKESUPER_QUICK, cache, operand);
        } else {
            operand.pntr = new_mb;
            OPCODE_REWRITE(OPC_INVOKENONVIRTUAL_QUICK, cache, operand);
        }

        REDISPATCH
    }

    DEF_OPC_RW(OPC_INVOKESTATIC)
    {
        int idx, cache;
        Operand operand;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_INVOKESTATIC, idx, cache);

        frame->last_pc = pc;
        new_mb = resolveMethod(mb->class, idx);
 
        if(exceptionOccured0(ee))
            goto throwException;

        operand.pntr = new_mb;
        OPCODE_REWRITE(OPC_INVOKESTATIC_QUICK, cache, operand);
        REDISPATCH
    }

    DEF_OPC_RW(OPC_INVOKEINTERFACE)
    {
        int idx, cache;
        Operand operand;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_INVOKEINTERFACE, idx, cache);

        frame->last_pc = pc;
        new_mb = resolveInterfaceMethod(mb->class, idx);
 
        if(exceptionOccured0(ee))
            goto throwException;

        if(CLASS_CB(new_mb->class)->access_flags & ACC_INTERFACE) {
            operand.uu.u1 = idx;
            operand.uu.u2 = 0;
            OPCODE_REWRITE(OPC_INVOKEINTERFACE_QUICK, cache, operand);
        } else {
            operand.uu.u1 = new_mb->args_count;
            operand.uu.u2 = new_mb->method_table_index;
            OPCODE_REWRITE(OPC_INVOKEVIRTUAL_QUICK, cache, operand);
        }

        REDISPATCH
    }

    DEF_OPC_RW(OPC_MULTIANEWARRAY)
    {
        int idx = pc->operand.uui.u1;
        int cache = pc->operand.uui.i;

        frame->last_pc = pc;
        resolveClass(mb->class, idx, FALSE);

        if(exceptionOccured0(ee))
            goto throwException;
        
        OPCODE_REWRITE(OPC_MULTIANEWARRAY_QUICK, cache, pc->operand);
        REDISPATCH
    }

    DEF_OPC_RW(OPC_NEW)
    DEF_OPC_RW(OPC_ANEWARRAY)
    DEF_OPC_RW(OPC_CHECKCAST)
    DEF_OPC_RW(OPC_INSTANCEOF)
    {
        int idx = pc->operand.uui.u1;
        int opcode = pc->operand.uui.u2;
        int cache = pc->operand.uui.i;
        Class *class;

        frame->last_pc = pc;
        class = resolveClass(mb->class, idx, opcode == OPC_NEW);

        if(exceptionOccured0(ee))
            goto throwException;
        
        if(opcode == OPC_NEW) {
            ClassBlock *cb = CLASS_CB(class);
            if(cb->access_flags & (ACC_INTERFACE | ACC_ABSTRACT)) {
                signalException("java/lang/InstantiationError", cb->name);
                goto throwException;
            }
        }

        OPCODE_REWRITE((opcode + OPC_NEW_QUICK-OPC_NEW), cache, pc->operand);
        REDISPATCH
    }
#else /* DIRECT */
    DEF_OPC_210(OPC_LDC)
        frame->last_pc = pc;

        resolveSingleConstant(mb->class, SINGLE_INDEX(pc));

        if(exceptionOccured0(ee))
            goto throwException;

        OPCODE_REWRITE(OPC_LDC_QUICK);
        DISPATCH(0, 0);

    DEF_OPC_210(OPC_LDC_W)
        frame->last_pc = pc;

        resolveSingleConstant(mb->class, DOUBLE_INDEX(pc));

        if(exceptionOccured0(ee))
            goto throwException;

        OPCODE_REWRITE(OPC_LDC_W_QUICK);
        DISPATCH(0, 0);

    DEF_OPC_210(OPC_ALOAD_0)
        if(mb->access_flags & ACC_STATIC)
            OPCODE_REWRITE(OPC_ILOAD_0);
        else
            OPCODE_REWRITE(OPC_ALOAD_THIS);
        DISPATCH(0, 0);

    DEF_OPC_210(OPC_TABLESWITCH)
    {
        int *aligned_pc = (int*)((uintptr_t)(pc + 4) & ~0x3);
        int deflt = ntohl(aligned_pc[0]);
        int low   = ntohl(aligned_pc[1]);
        int high  = ntohl(aligned_pc[2]);
        int index = *--ostack;

        DISPATCH(0, (index < low || index > high) ?
                        deflt : ntohl(aligned_pc[index - low + 3]));
    }

    DEF_OPC_210(OPC_LOOKUPSWITCH)
    {
        int *aligned_pc = (int*)((uintptr_t)(pc + 4) & ~0x3);
        int deflt  = ntohl(aligned_pc[0]);
        int npairs = ntohl(aligned_pc[1]);
        int key    = *--ostack;
        int i;

        for(i = 2; (i < npairs*2+2) && (key != ntohl(aligned_pc[i])); i += 2);

        DISPATCH(0, i == npairs*2+2 ? deflt : ntohl(aligned_pc[i+1]));
    }

    DEF_OPC_210(OPC_GETSTATIC) 
    {
        FieldBlock *fb;
               
        frame->last_pc = pc;
        fb = resolveField(mb->class, DOUBLE_INDEX(pc));

        if(exceptionOccured0(ee))
            goto throwException;

        if((*fb->type == 'J') || (*fb->type == 'D'))
            OPCODE_REWRITE(OPC_GETSTATIC2_QUICK);
        else
            OPCODE_REWRITE(OPC_GETSTATIC_QUICK);
        DISPATCH(0, 0);
    }

    DEF_OPC_210(OPC_PUTSTATIC) 
    {
        FieldBlock *fb;
               
        frame->last_pc = pc;
        fb = resolveField(mb->class, DOUBLE_INDEX(pc));

        if(exceptionOccured0(ee))
            goto throwException;

        if((*fb->type == 'J') || (*fb->type == 'D'))
            OPCODE_REWRITE(OPC_PUTSTATIC2_QUICK);
        else
            OPCODE_REWRITE(OPC_PUTSTATIC_QUICK);
        DISPATCH(0, 0);
    }

    DEF_OPC_210(OPC_GETFIELD)
    {
        int idx;
        FieldBlock *fb;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_GETFIELD, idx);

        frame->last_pc = pc;
        fb = resolveField(mb->class, idx);

        if(exceptionOccured0(ee))
            goto throwException;

        if(fb->offset > 255)
            OPCODE_REWRITE(OPC_GETFIELD_QUICK_W);
        else
            OPCODE_REWRITE_OPERAND1(((*fb->type == 'J') || (*fb->type == 'D') ? 
                 OPC_GETFIELD2_QUICK : OPC_GETFIELD_QUICK), fb->offset);

        DISPATCH(0, 0);
    }

    DEF_OPC_210(OPC_PUTFIELD)
    {
        int idx;
        FieldBlock *fb;

        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_PUTFIELD, idx);

        frame->last_pc = pc;
        fb = resolveField(mb->class, idx);

        if(exceptionOccured0(ee))
            goto throwException;

        if(fb->offset > 255)
            OPCODE_REWRITE(OPC_PUTFIELD_QUICK_W);
        else
            OPCODE_REWRITE_OPERAND1(((*fb->type == 'J') || (*fb->type == 'D') ? 
                 OPC_PUTFIELD2_QUICK : OPC_PUTFIELD_QUICK), fb->offset);

        DISPATCH(0, 0);
    }

    DEF_OPC_210(OPC_GETFIELD_QUICK_W)
    {
        FieldBlock *fb = RESOLVED_FIELD(pc);
        Object *obj = (Object *)*--ostack;
        uintptr_t *addr;

        NULL_POINTER_CHECK(obj);
        addr = &(INST_DATA(obj)[fb->offset]);

        if((*fb->type == 'J') || (*fb->type == 'D')) {
            PUSH_LONG(*(u8*)addr, 3);
        } else {
            PUSH_0(*addr, 3);
        }
    }

#ifdef USE_CACHE
    DEF_OPC_012(OPC_PUTFIELD_QUICK_W)
    {
        FieldBlock *fb = RESOLVED_FIELD(pc);
 
        if((*fb->type == 'J') || (*fb->type == 'D')) {
            Object *obj = (Object *)*--ostack;

            NULL_POINTER_CHECK(obj);
            *(u8*)(&(INST_DATA(obj)[fb->offset])) = cache.l;
        } else {
            Object *obj = (Object *)cache.i.v1;

            NULL_POINTER_CHECK(obj);
            INST_DATA(obj)[fb->offset] = cache.i.v2;
        }
        DISPATCH(0, 3);
    }
#else
    DEF_OPC_012(OPC_PUTFIELD_QUICK_W)
    {
        FieldBlock *fb = RESOLVED_FIELD(pc);
 
        if((*fb->type == 'J') || (*fb->type == 'D')) {
            Object *obj = (Object *)ostack[-3];

            ostack -= 3;
            NULL_POINTER_CHECK(obj);
            *(u8*)(&(INST_DATA(obj)[fb->offset])) = *(u8*)&ostack[1];
        } else {
            Object *obj = (Object *)ostack[-2];

            ostack -= 2;
            NULL_POINTER_CHECK(obj);
            INST_DATA(obj)[fb->offset] = ostack[1];
        }
        DISPATCH(0, 3);
    }
#endif

    DEF_OPC_210(OPC_INVOKEVIRTUAL)
    {
        int idx;
        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_INVOKEVIRTUAL, idx);

        frame->last_pc = pc;
        new_mb = resolveMethod(mb->class, idx);
 
        if(exceptionOccured0(ee))
            goto throwException;

        if((new_mb->args_count < 256) && (new_mb->method_table_index < 256)) {
            OPCODE_REWRITE_OPERAND2(OPC_INVOKEVIRTUAL_QUICK,
                                    new_mb->method_table_index, new_mb->args_count);
        } else
            OPCODE_REWRITE(OPC_INVOKEVIRTUAL_QUICK_W);
        DISPATCH(0, 0);
    }

    DEF_OPC_210(OPC_INVOKEVIRTUAL_QUICK_W)
        new_mb = RESOLVED_METHOD(pc);
        arg1 = ostack - (new_mb->args_count);
        NULL_POINTER_CHECK(*arg1);

        new_class = (*(Object **)arg1)->class;
        new_mb = CLASS_CB(new_class)->method_table[new_mb->method_table_index];

        goto invokeMethod;

    DEF_OPC_210(OPC_INVOKESPECIAL)
    {
        int idx;
        WITH_OPCODE_CHANGE_CP_DINDEX(OPC_INVOKESPECIAL, idx);

        frame->last_pc = pc;
        new_mb = resolveMethod(mb->class, idx);
 
        if(exceptionOccured0(ee))
            goto throwException;

        /* Check if invoking a super method... */
        if((CLASS_CB(mb->class)->access_flags & ACC_SUPER) &&
              ((new_mb->access_flags & ACC_PRIVATE) == 0) && (new_mb->name[0] != '<')) {
            OPCODE_REWRITE_OPERAND2(OPC_INVOKESUPER_QUICK,
                    new_mb->method_table_index >> 8,
                    new_mb->method_table_index & 0xff);
        } else
            OPCODE_REWRITE(OPC_INVOKENONVIRTUAL_QUICK);
        DISPATCH(0, 0);
    }

    DEF_OPC_210(OPC_INVOKESTATIC)
        frame->last_pc = pc;
        new_mb = resolveMethod(mb->class, DOUBLE_INDEX(pc));
 
        if(exceptionOccured0(ee))
            goto throwException;

        OPCODE_REWRITE(OPC_INVOKESTATIC_QUICK);
        DISPATCH(0, 0);

    DEF_OPC_210(OPC_INVOKEINTERFACE)
        frame->last_pc = pc;
        new_mb = resolveInterfaceMethod(mb->class, DOUBLE_INDEX(pc));
 
        if(exceptionOccured0(ee))
            goto throwException;

        if(CLASS_CB(new_mb->class)->access_flags & ACC_INTERFACE)
            OPCODE_REWRITE(OPC_INVOKEINTERFACE_QUICK);
        else {
            pc[3] = pc[4] = OPC_NOP;
            OPCODE_REWRITE(OPC_INVOKEVIRTUAL);
        }

        DISPATCH(0, 0);

#define REWRITE_RESOLVE_CLASS(opcode)                                     \
    DEF_OPC_210(opcode)                                                   \
        frame->last_pc = pc;                                              \
        resolveClass(mb->class, DOUBLE_INDEX(pc), FALSE);                 \
                                                                          \
        if(exceptionOccured0(ee))                                         \
            goto throwException;                                          \
                                                                          \
        OPCODE_REWRITE((opcode + OPC_ANEWARRAY_QUICK-OPC_ANEWARRAY));     \
        DISPATCH(0, 0);                                                   \

   REWRITE_RESOLVE_CLASS(OPC_ANEWARRAY)
   REWRITE_RESOLVE_CLASS(OPC_CHECKCAST)
   REWRITE_RESOLVE_CLASS(OPC_INSTANCEOF)
   REWRITE_RESOLVE_CLASS(OPC_MULTIANEWARRAY)

    DEF_OPC_210(OPC_NEW)
    {
        Class *class;
        ClassBlock *cb;

        frame->last_pc = pc;
        class = resolveClass(mb->class, DOUBLE_INDEX(pc), TRUE);

        if(exceptionOccured0(ee))
            goto throwException;
        
        cb = CLASS_CB(class);
        if(cb->access_flags & (ACC_INTERFACE | ACC_ABSTRACT)) {
            signalException("java/lang/InstantiationError", cb->name);
            goto throwException;
        }

        OPCODE_REWRITE(OPC_NEW_QUICK);
        DISPATCH(0, 0);
    }

    DEF_OPC_210(OPC_WIDE)
    {
       int opcode = pc[1];
        switch(opcode) {
            case OPC_ILOAD:
            case OPC_FLOAD:
            case OPC_ALOAD:
                *ostack++ = lvars[DOUBLE_INDEX(pc+1)];
                pc += 4;
                break;

            case OPC_LLOAD:
            case OPC_DLOAD:
                *(u8*)ostack = *(u8*)(&lvars[DOUBLE_INDEX(pc+1)]);
                ostack += 2;
                pc += 4;
                break;

            case OPC_ISTORE:
            case OPC_FSTORE:
            case OPC_ASTORE:
                lvars[DOUBLE_INDEX(pc+1)] = *--ostack;
                pc += 4;
                break;

            case OPC_LSTORE:
            case OPC_DSTORE:
                ostack -= 2;
                *(u8*)(&lvars[DOUBLE_INDEX(pc+1)]) = *(u8*)ostack;
                pc += 4;
                break;

            case OPC_RET:
                pc = (unsigned char*)lvars[DOUBLE_INDEX((pc+1))];
                break;

            case OPC_IINC:
                lvars[DOUBLE_INDEX(pc+1)] += DOUBLE_SIGNED(pc+3);
                pc += 6;
                break;
        }
        DISPATCH(0, 0);
    }

    DEF_OPC_210(OPC_GOTO_W)
        DISPATCH(0, READ_S4_OP(pc));

    DEF_OPC_210(OPC_JSR_W)
        PUSH_0((uintptr_t)pc+2, READ_S4_OP(pc));

    DEF_OPC_210(OPC_LOCK)
        DISPATCH(0, 0);
#endif /* DIRECT */

    DEF_OPC_210(OPC_GETSTATIC2_QUICK) 
    {
        FieldBlock *fb = RESOLVED_FIELD(pc);
        PUSH_LONG(*(u8*)&fb->static_value, 3);
    }

    DEF_OPC_012(OPC_PUTSTATIC2_QUICK) 
    {
        FieldBlock *fb = RESOLVED_FIELD(pc);
        POP_LONG(*(u8*)&fb->static_value, 3);
    }

    DEF_OPC_210(OPC_GETFIELD2_QUICK)
    {
        Object *obj = (Object *)*--ostack;
        NULL_POINTER_CHECK(obj);
                
        PUSH_LONG(*(u8*)(&(INST_DATA(obj)[SINGLE_INDEX(pc)])), 3);
    }

#ifdef USE_CACHE
    DEF_OPC_012(OPC_PUTFIELD2_QUICK)
    {
        Object *obj = (Object *)*--ostack;
        NULL_POINTER_CHECK(obj);

        *(u8*)(&(INST_DATA(obj)[SINGLE_INDEX(pc)])) = cache.l;
        DISPATCH(0, 3);
    }

    DEF_OPC_012(OPC_PUTFIELD_QUICK)
    {
        Object *obj = (Object *)cache.i.v1;
        NULL_POINTER_CHECK(obj);
                
        INST_DATA(obj)[SINGLE_INDEX(pc)] = cache.i.v2;
        DISPATCH(0, 3);
    }
#else
    DEF_OPC_012(OPC_PUTFIELD2_QUICK)
    {
        Object *obj = (Object *)ostack[-3];

        ostack -= 3;
        NULL_POINTER_CHECK(obj);
        *(u8*)(&(INST_DATA(obj)[SINGLE_INDEX(pc)])) = *(u8*)&ostack[1];
        DISPATCH(0, 3);
    }

    DEF_OPC_012(OPC_PUTFIELD_QUICK)
    {
        Object *obj = (Object *)ostack[-2];

        ostack -= 2;
        NULL_POINTER_CHECK(obj);
        INST_DATA(obj)[SINGLE_INDEX(pc)] = ostack[1];
        DISPATCH(0, 3);
    }
#endif

    DEF_OPC_210(OPC_INVOKESUPER_QUICK)
        new_mb = CLASS_CB(CLASS_CB(mb->class)->super)->method_table[DOUBLE_INDEX(pc)];
        arg1 = ostack - (new_mb->args_count);
        NULL_POINTER_CHECK(*arg1);
        goto invokeMethod;

    DEF_OPC_210(OPC_INVOKENONVIRTUAL_QUICK)
        new_mb = RESOLVED_METHOD(pc);
        arg1 = ostack - (new_mb->args_count);
        NULL_POINTER_CHECK(*arg1);
        goto invokeMethod;

    DEF_OPC_210(OPC_INVOKESTATIC_QUICK)
        new_mb = RESOLVED_METHOD(pc);
        arg1 = ostack - new_mb->args_count;
        goto invokeMethod;

    DEF_OPC_210(OPC_INVOKEINTERFACE_QUICK)
    {
        int mtbl_idx;
        ClassBlock *cb;
        int cache = INV_INTF_CACHE(pc);

        new_mb = (MethodBlock *)CP_INFO(cp, INV_INTF_IDX(pc));
        arg1 = ostack - new_mb->args_count;

        NULL_POINTER_CHECK(*arg1);

        cb = CLASS_CB(new_class = (*(Object **)arg1)->class);

        if((cache >= cb->imethod_table_size) ||
                  (new_mb->class != cb->imethod_table[cache].interface)) {
            for(cache = 0; (cache < cb->imethod_table_size) &&
                           (new_mb->class != cb->imethod_table[cache].interface); cache++);

            if(cache == cb->imethod_table_size)
                THROW_EXCEPTION("java/lang/IncompatibleClassChangeError",
                                 "unimplemented interface");

            INV_INTF_CACHE(pc) = cache;
        }

        mtbl_idx = cb->imethod_table[cache].offsets[new_mb->method_table_index];
        new_mb = cb->method_table[mtbl_idx];

        goto invokeMethod;
    }

    DEF_OPC_210(OPC_NEW_QUICK)
    {
        Class *class = RESOLVED_CLASS(pc);
        Object *obj;

        frame->last_pc = pc;
        if((obj = allocObject(class)) == NULL)
            goto throwException;

        PUSH_0((uintptr_t)obj, 3);
    }
 
    DEF_OPC_210(OPC_ANEWARRAY_QUICK)
    {
        Class *class = RESOLVED_CLASS(pc);
        char *name = CLASS_CB(class)->name;
        int count = *--ostack;
        Class *array_class;
        char *ac_name;
        Object *obj;

        frame->last_pc = pc;

        if(count < 0) {
            signalException("java/lang/NegativeArraySizeException", NULL);
            goto throwException;
        }

        ac_name = sysMalloc(strlen(name) + 4);

        if(name[0] == '[')
            strcat(strcpy(ac_name, "["), name);
        else
            strcat(strcat(strcpy(ac_name, "[L"), name), ";");

        array_class = findArrayClassFromClass(ac_name, mb->class);
        free(ac_name);

        if(exceptionOccured0(ee))
            goto throwException;

        if((obj = allocArray(array_class, count, sizeof(Object*))) == NULL)
            goto throwException;

        PUSH_0((uintptr_t)obj, 3);
    }

    DEF_OPC_210(OPC_CHECKCAST_QUICK)
    {
        Class *class = RESOLVED_CLASS(pc);
        Object *obj = (Object*)ostack[-1]; 
               
        if((obj != NULL) && !isInstanceOf(class, obj->class))
            THROW_EXCEPTION("java/lang/ClassCastException", CLASS_CB(obj->class)->name);
    
        DISPATCH(0, 3);
    }

    DEF_OPC_210(OPC_INSTANCEOF_QUICK)
    {
        Class *class = RESOLVED_CLASS(pc);
        Object *obj = (Object*)ostack[-1]; 
               
        if(obj != NULL)
            ostack[-1] = isInstanceOf(class, obj->class); 

        DISPATCH(0, 3);
    }

    DEF_OPC_210(OPC_MULTIANEWARRAY_QUICK)
    {
        Class *class = RESOLVED_CLASS(pc);
        int i, dim = MULTI_ARRAY_DIM(pc);
        Object *obj;

        ostack -= dim;
        frame->last_pc = pc;

        for(i = 0; i < dim; i++)
            if((intptr_t)ostack[i] < 0) {
                signalException("java/lang/NegativeArraySizeException", NULL);
                goto throwException;
            }

        if((obj = allocMultiArray(class, dim, (intptr_t *)ostack)) == NULL)
            goto throwException;

        PUSH_0((uintptr_t)obj, 4);
    }

    /* Special bytecode which forms the body of an abstract method.
       If it is invoked it'll throw an abstract method exception. */

    DEF_OPC_210(OPC_ABSTRACT_METHOD_ERROR)
        /* As the method has been invoked, a frame will exist for
           the abstract method itself.  Pop this to get the correct
           exception stack trace. */
        ee->last_frame = frame->prev;

        /* Throw the exception */
        signalException("java/lang/AbstractMethodError", mb->name);
        goto throwException;

    DEF_OPC_210(OPC_INVOKEVIRTUAL_QUICK)
        arg1 = ostack - INV_QUICK_ARGS(pc);
        NULL_POINTER_CHECK(*arg1);

        new_class = (*(Object **)arg1)->class;
        new_mb = CLASS_CB(new_class)->method_table[INV_QUICK_IDX(pc)];

invokeMethod:
{
    /* Create new frame first.  This is also created for natives
       so that they appear correctly in the stack trace */

    Frame *new_frame = (Frame *)(arg1 + new_mb->max_locals);
    Object *sync_ob = NULL;

    frame->last_pc = pc;
    ostack = (uintptr_t *)(new_frame+1);

    if((char*)(ostack + new_mb->max_stack) > ee->stack_end) {
        if(ee->overflow++) {
            /* Overflow when we're already throwing stack overflow.
               Stack extension should be enough to throw exception,
               so something's seriously gone wrong - abort the VM! */
            jam_printf("Fatal stack overflow!  Aborting VM.\n");
            exitVM(1);
        }
        ee->stack_end += STACK_RED_ZONE_SIZE;
        THROW_EXCEPTION("java/lang/StackOverflowError", NULL);
    }

    new_frame->mb = new_mb;
    new_frame->lvars = arg1;
    new_frame->ostack = ostack;
    new_frame->prev = frame;

    ee->last_frame = new_frame;

    if(new_mb->access_flags & ACC_SYNCHRONIZED) {
        sync_ob = new_mb->access_flags & ACC_STATIC ? (Object*)new_mb->class : (Object*)*arg1;
        objectLock(sync_ob);
    }

    if(new_mb->access_flags & ACC_NATIVE) {
        ostack = (*(uintptr_t *(*)(Class*, MethodBlock*, uintptr_t*))
                     new_mb->native_invoker)(new_mb->class, new_mb, arg1);

        if(sync_ob)
            objectUnlock(sync_ob);

        ee->last_frame = frame;

        if(exceptionOccured0(ee))
            goto throwException;
        DISPATCH(0, *pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);
    } else {
        PREPARE_MB(new_mb);

        frame = new_frame;
        mb = new_mb;
        lvars = new_frame->lvars;
        this = (Object*)lvars[0];
        pc = (CodePntr)mb->code;
        cp = &(CLASS_CB(mb->class)->constant_pool);

        DISPATCH_FIRST
    }
}

methodReturn:
    /* Set interpreter state to previous frame */

    frame = frame->prev;

    if(frame->mb == NULL) {
        /* The previous frame is a dummy frame - this indicates
           top of this Java invocation. */
        return ostack;
    }

    if(mb->access_flags & ACC_SYNCHRONIZED) {
        Object *sync_ob = mb->access_flags & ACC_STATIC ? (Object*)mb->class : this;
        objectUnlock(sync_ob);
    }

    mb = frame->mb;
    ostack = lvars;
    lvars = frame->lvars;
    this = (Object*)lvars[0];
    pc = frame->last_pc;
    cp = &(CLASS_CB(mb->class)->constant_pool);

    /* Pop frame */ 
    ee->last_frame = frame;

    DISPATCH_RET(*pc == OPC_INVOKEINTERFACE_QUICK ? 5 : 3);

throwException:
    {
        Object *excep = ee->exception;
        ee->exception = NULL;

        pc = findCatchBlock(excep->class);

        /* If we didn't find a handler, restore exception and
           return to previous invocation */

        if(pc == NULL) {
            ee->exception = excep;
            return NULL;
        }

        /* If we're handling a stack overflow, reduce the stack
           back past the red zone to enable handling of further
           overflows */

        if(ee->overflow) {
            ee->overflow = FALSE;
            ee->stack_end -= STACK_RED_ZONE_SIZE;
        }

        /* Setup intepreter to run the found catch block */

        frame = ee->last_frame;
        mb = frame->mb;
        ostack = frame->ostack;
        lvars = frame->lvars;
        this = (Object*)lvars[0];
        cp = &(CLASS_CB(mb->class)->constant_pool);

        *ostack++ = (uintptr_t)excep;

        /* Dispatch to the first bytecode */

        DISPATCH_FIRST
    }
#ifndef THREADED
  }}
#endif
}

void initialiseInterpreter() {
#ifdef DIRECT
    initialiseDirect();
#endif
}

