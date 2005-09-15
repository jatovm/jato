/*
 * Copyright (C) 2003, 2004, 2005 Robert Lougher <rob@lougher.demon.co.uk>.
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

#define OS_ARCH "ppc"

#define HANDLER_TABLE_T static const void
#define DOUBLE_1_BITS 0x3ff0000000000000LL

#define READ_DBL(v,p,l)	v = ((u8)p[0]<<56)|((u8)p[1]<<48)|((u8)p[2]<<40) \
                            |((u8)p[3]<<32)|((u8)p[4]<<24)|((u8)p[5]<<16) \
                            |((u8)p[6]<<8)|(u8)p[7]; p+=8

/* Needed for i386 -- empty here */
#define FPU_HACK

#define COMPARE_AND_SWAP(addr, old_val, new_val) \
({                                               \
    int result, read_val;                        \
    __asm__ __volatile__ ("                      \
                li %0,0\n                        \
        1:      lwarx %1,0,%2\n                  \
                cmpw %3,%1\n                     \
                bne- 2f\n                        \
                stwcx. %4,0,%2\n                 \
                bne- 1b\n                        \
                li %0,1\n                        \
        2:"                                      \
    : "=&r" (result), "=&r" (read_val)           \
    : "r" (addr), "r" (old_val), "r" (new_val)   \
    : "cc", "memory");                           \
    result;                                      \
})

#define ATOMIC_READ(addr) *addr
#define ATOMIC_WRITE(addr, value) *addr = value

#define MBARRIER() __asm__ __volatile__ ("sync" ::: "memory")
#define UNLOCK_MBARRIER() __asm__ __volatile__ ("sync" ::: "memory")
#define JMM_LOCK_MBARRIER() __asm__ __volatile__ ("isync" ::: "memory")
#define JMM_UNLOCK_MBARRIER() __asm__ __volatile__ ("lwsync" ::: "memory")
