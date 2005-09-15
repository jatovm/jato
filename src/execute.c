/*
 * Copyright (C) 2003, 2004 Robert Lougher <rob@lougher.demon.co.uk>.
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
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include "jam.h"
#include "sig.h"
#include "frame.h"
#include "lock.h"

#define VA_DOUBLE(args, sp)  *(u8*)sp = va_arg(args, u8); sp+=2
#define VA_SINGLE(args, sp)  *sp++ = va_arg(args, u4)

#define JA_DOUBLE(args, sp)  *(u8*)sp = *args++; sp+=2
#define JA_SINGLE(args, sp)  *sp++ = *(u4*)args; args++

void *executeMethodArgs(Object *ob, Class *class, MethodBlock *mb, ...) {
    va_list jargs;
    void *ret;

    va_start(jargs, mb);
    ret = executeMethodVaList(ob, class, mb, jargs);
    va_end(jargs);

    return ret;
}

void *executeMethodVaList(Object *ob, Class *class, MethodBlock *mb, va_list jargs) {
    char *sig = mb->type;

    ExecEnv *ee = getExecEnv();
    void *ret;
    u4 *sp;

    CREATE_TOP_FRAME(ee, class, mb, sp, ret);

    /* copy args onto stack */

    if(ob)
        *sp++ = (u4) ob; /* push receiver first */

    SCAN_SIG(sig, VA_DOUBLE(jargs, sp), VA_SINGLE(jargs, sp))

    if(mb->access_flags & ACC_SYNCHRONIZED)
        objectLock(ob ? ob : (Object*)mb->class);

    if(mb->access_flags & ACC_NATIVE)
        (*(u4 *(*)(Class*, MethodBlock*, u4*))mb->native_invoker)(class, mb, ret);
    else
        executeJava();

    if(mb->access_flags & ACC_SYNCHRONIZED)
        objectUnlock(ob ? ob : (Object*)mb->class);

    POP_TOP_FRAME(ee);
    return ret;
}

void *executeMethodList(Object *ob, Class *class, MethodBlock *mb, u8 *jargs) {
    char *sig = mb->type;

    ExecEnv *ee = getExecEnv();
    void *ret;
    u4 *sp;

    CREATE_TOP_FRAME(ee, class, mb, sp, ret);

    /* copy args onto stack */

    if(ob)
        *sp++ = (u4) ob; /* push receiver first */

    SCAN_SIG(sig, JA_DOUBLE(jargs, sp), JA_SINGLE(jargs, sp))

    if(mb->access_flags & ACC_SYNCHRONIZED)
        objectLock(ob ? ob : (Object*)mb->class);

    if(mb->access_flags & ACC_NATIVE)
        (*(u4 *(*)(Class*, MethodBlock*, u4*))mb->native_invoker)(class, mb, ret);
    else
        executeJava();

    if(mb->access_flags & ACC_SYNCHRONIZED)
        objectUnlock(ob ? ob : (Object*)mb->class);

    POP_TOP_FRAME(ee);
    return ret;
}
