/*
 * Copyright (C) 2003, 2004, 2005, 2006 Robert Lougher <rob@lougher.org.uk>.
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

/* Macros for handler/bytecode rewriting */

#define WITH_OPCODE_CHANGE_CP_DINDEX(opcode, index)        \
    index = DOUBLE_INDEX(pc);                              \
    if(pc[0] != opcode)                                    \
        DISPATCH(0, 0);

#define OPCODE_REWRITE(opcode)                             \
    pc[0] = opcode 

#define OPCODE_REWRITE_OPERAND1(opcode, operand1)          \
{                                                          \
    pc[0] = OPC_LOCK;                                      \
    MBARRIER();                                            \
    pc[1] = operand1;                                      \
    MBARRIER();                                            \
    pc[0] = opcode;                                        \
}

#define OPCODE_REWRITE_OPERAND2(opcode, operand1, operand2)\
{                                                          \
    pc[0] = OPC_LOCK;                                      \
    MBARRIER();                                            \
    pc[1] = operand1;                                      \
    pc[2] = operand2;                                      \
    MBARRIER();                                            \
    pc[0] = opcode;                                        \
}

#ifdef THREADED
/* Two levels of macros are needed to correctly produce the label
 * from the OPC_xxx macro passed into DEF_OPC as cpp doesn't 
 * prescan when concatenating with ##...
 *
 * On gcc <= 2.95, we also get a space inserted before the :
 * e.g DEF_OPC(OPC_NULL) -> opc0 : - the ##: is a hack to fix
 * this, but this generates warnings on >= 2.96...
 */
#if (__GNUC__ == 2) && (__GNUC_MINOR__ <= 95)
#define label(x, y)              \
opc##x##_##y##:
#else
#define label(x, y)              \
opc##x##_##y:
#endif

#define DEF_OPC(opcode, level)   \
label(opcode, level)

#define DISPATCH(level, ins_len) \
{                                \
    pc += ins_len;               \
    goto *handlers_##level[*pc]; \
}
#else /* THREADED */

#define DEF_OPC(opcode, level)   \
    case opcode:

#define DISPATCH(level, ins_len) \
{                                \
    pc += ins_len;               \
    break;                       \
}
#endif

#define DISPATCH_FIRST           \
    DISPATCH(0, 0)

#define DISPATCH_RET(ins_len)    \
    DISPATCH(0, ins_len)

#define BRANCH(TEST)                 \
    DISPATCH(0, (TEST) ? READ_S2_OP(pc) : 3)

/* No method preparation is needed on the
   indirect interpreter */
#define PREPARE_MB(mb)

#define ARRAY_TYPE(pc)        READ_U1_OP(pc)
#define SINGLE_INDEX(pc)      READ_U1_OP(pc)
#define DOUBLE_INDEX(pc)      READ_U2_OP(pc)
#define SINGLE_SIGNED(pc)     READ_S1_OP(pc)
#define DOUBLE_SIGNED(pc)     READ_S2_OP(pc)
#define IINC_LVAR_IDX(pc)     SINGLE_INDEX(pc)
#define IINC_DELTA(pc)        SINGLE_SIGNED(pc + 1)
#define INV_QUICK_ARGS(pc)    READ_U1_OP(pc + 1)
#define INV_QUICK_IDX(pc)     READ_U1_OP(pc)
#define INV_INTF_IDX(pc)      DOUBLE_INDEX(pc)
#define INV_INTF_CACHE(pc)    READ_U1_OP(pc + 3)
#define MULTI_ARRAY_DIM(pc)   READ_U1_OP(pc + 2)
#define RESOLVED_CONSTANT(pc) CP_INFO(cp, SINGLE_INDEX(pc))
#define RESOLVED_FIELD(pc)    ((FieldBlock*)CP_INFO(cp, DOUBLE_INDEX(pc)))
#define RESOLVED_METHOD(pc)   ((MethodBlock*)CP_INFO(cp, DOUBLE_INDEX(pc)))
#define RESOLVED_CLASS(pc)    ((Class *)CP_INFO(cp, DOUBLE_INDEX(pc)))

