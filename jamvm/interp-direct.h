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

#ifndef THREADED
#error Direct interpreter cannot be built non-threaded
#endif

/* Macros for handler/bytecode rewriting */

#ifdef USE_CACHE
#define WITH_OPCODE_CHANGE_CP_DINDEX(opcode, index, cache) \
{                                                          \
    index = pc->operand.uui.u1;                            \
    cache = pc->operand.uui.i;                             \
    if(pc->handler != L(opcode, 0) &&                      \
       pc->handler != L(opcode, 1) &&                      \
       pc->handler != L(opcode, 2))                        \
        REDISPATCH                                         \
}
#else /* USE_CACHE */
#define WITH_OPCODE_CHANGE_CP_DINDEX(opcode, index, cache) \
{                                                          \
    index = pc->operand.uui.u1;                            \
    cache = pc->operand.uui.i;                             \
    if(pc->handler != L(opcode, 0))                        \
        REDISPATCH                                         \
}
#endif

#define OPCODE_REWRITE(opcode, cache, new_operand)         \
{                                                          \
    pc->handler = &&rewrite_lock;                          \
    MBARRIER();                                            \
    pc->operand = new_operand;                             \
    MBARRIER();                                            \
    pc->handler = handlers[cache][opcode];                 \
}

/* Two levels of macros are needed to correctly produce the label
 * from the OPC_xxx macro passed into DEF_OPC as cpp doesn't 
 * prescan when concatenating with ##...
 *
 * On gcc <= 2.95, we also get a space inserted before the :
 * e.g DEF_OPC(OPC_NULL) -> opc0 : - the ##: is a hack to fix
 * this, but this generates warnings on >= 2.96...
 */
#if (__GNUC__ == 2) && (__GNUC_MINOR__ <= 95)
#define label(x, y)                     \
opc##x##_##y##:
#else
#define label(x, y)                     \
opc##x##_##y:
#endif

#define DEF_OPC(opcode, level)          \
label(opcode, level)

#ifdef PREFETCH
#define DISPATCH_FIRST                  \
{                                       \
    next_handler = pc[1].handler;       \
    REDISPATCH                          \
}

#define DISPATCH(level, ins_len)        \
{                                       \
    const void *dispatch = next_handler;\
    next_handler = (++pc)[1].handler;   \
    goto *dispatch;                     \
}
#else
#define DISPATCH_FIRST                  \
    REDISPATCH

#define DISPATCH(level, ins_len)        \
    goto *(++pc)->handler;

#endif /* PREFETCH */

#define BRANCH(TEST)                    \
    if(TEST) {                          \
        pc = (Instruction*)             \
             pc->operand.pntr;          \
        DISPATCH_FIRST                  \
    } else                              \
        DISPATCH(0,0)

#define REDISPATCH                      \
    goto *pc->handler;

#define DISPATCH_RET(ins_len)           \
    pc++;                               \
    DISPATCH_FIRST

#ifdef USE_CACHE
#define DEF_OPC_RW(op)                  \
    DEF_OPC(op, 0)                      \
    DEF_OPC(op, 1)                      \
    DEF_OPC(op, 2)
#else
#define DEF_OPC_RW(op)                  \
    DEF_OPC(op, 0)
#endif

#define PREPARE_MB(mb)                  \
    if((uintptr_t)mb->code & 0x3)       \
        prepare(mb, handlers)

#define ARRAY_TYPE(pc)        pc->operand.i
#define SINGLE_INDEX(pc)      pc->operand.i
#define DOUBLE_INDEX(pc)      pc->operand.i
#define SINGLE_SIGNED(pc)     pc->operand.i
#define DOUBLE_SIGNED(pc)     pc->operand.i
#define IINC_LVAR_IDX(pc)     pc->operand.ii.i1
#define IINC_DELTA(pc)        pc->operand.ii.i2
#define INV_QUICK_ARGS(pc)    pc->operand.uu.u1
#define INV_QUICK_IDX(pc)     pc->operand.uu.u2
#define INV_INTF_IDX(pc)      pc->operand.uu.u1
#define INV_INTF_CACHE(pc)    pc->operand.uu.u2
#define MULTI_ARRAY_DIM(pc)   pc->operand.uui.u2
#define RESOLVED_CONSTANT(pc) pc->operand.u
#define RESOLVED_FIELD(pc)    ((FieldBlock*)pc->operand.pntr)
#define RESOLVED_METHOD(pc)   ((MethodBlock*)pc->operand.pntr)
#define RESOLVED_CLASS(pc)    (Class *)CP_INFO(cp, pc->operand.uui.u1)

extern void initialiseDirect();
extern void prepare(MethodBlock *mb, const void ***handlers);
