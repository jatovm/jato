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

#include <limits.h>

/* _GNU_SOURCE doesn't bring in C99 long long constants,
   but we do get the GNU constants */
#ifndef LLONG_MAX
#define LLONG_MAX LONG_LONG_MAX
#endif

#ifndef LLONG_MIN
#define LLONG_MIN LONG_LONG_MIN
#endif

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#elif HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define FLOAT_CONST(val) (uintptr_t)val << (sizeof(uintptr_t) * 8 - 32)
#else
#define FLOAT_CONST(val) val
#endif

#define FLOAT_1_BITS FLOAT_CONST(0x3f800000)
#define FLOAT_2_BITS FLOAT_CONST(0x40000000)

#define READ_U1_OP(p)    ((p)[1])
#define READ_U2_OP(p)    (((p)[1]<<8)|(p)[2])
#define READ_U4_OP(p)    (((p)[1]<<24)|((p)[2]<<16)|((p)[3]<<8)|(p)[4])
#define READ_S1_OP(p)    (signed char)READ_U1_OP(p)
#define READ_S2_OP(p)    (signed short)READ_U2_OP(p)
#define READ_S4_OP(p)    (signed int)READ_U4_OP(p)
