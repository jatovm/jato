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
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define OS_ARCH "arm"

/* Override default min and max heap sizes.  ARM machines are
   usually embedded, and the standard defaults are too large. */
#define DEFAULT_MAX_HEAP 16*MB
#define DEFAULT_MIN_HEAP 1*MB

#ifdef DIRECT
#define HANDLER_TABLE_T static const void
#else
#define HANDLER_TABLE_T void
#endif

#if defined(__VFP_FP__) || defined(__ARMEB__)
#define DOUBLE_1_BITS 0x3ff0000000000000LL
#else
#define DOUBLE_1_BITS 0x000000003ff00000LL
#endif

#if defined(__VFP_FP__) || defined(__ARMEB__)
#define READ_DBL(v,p,l)	v = ((u8)p[0]<<56)|((u8)p[1]<<48)|((u8)p[2]<<40) \
                            |((u8)p[3]<<32)|((u8)p[4]<<24)|((u8)p[5]<<16) \
                            |((u8)p[6]<<8)|(u8)p[7]; p+=8
#else
#define READ_DBL(v,p,l)	v = ((u8)p[4]<<56)|((u8)p[5]<<48)|((u8)p[6]<<40) \
                            |((u8)p[7]<<32)|((u8)p[0]<<24)|((u8)p[1]<<16) \
                            |((u8)p[2]<<8)|(u8)p[3]; p+=8
#endif

/* Needed for i386 -- empty here */
#define FPU_HACK

#define COMPARE_AND_SWAP(addr, old_val, new_val) \
({                                               \
    int result, read_val;                        \
    __asm__ __volatile__ ("                      \
                mvn %1, #1;                      \
        1:      swp %1, %1, [%2];                \
                cmn %1, #2;                      \
                beq 1b;                          \
                cmp %3, %1;                      \
                strne %1, [%2];                  \
                movne %0, #0;                    \
                streq %4, [%2];                  \
                moveq %0, #1;"                   \
    : "=&r" (result), "=&r" (read_val)           \
    : "r" (addr), "r" (old_val), "r" (new_val)   \
    : "cc", "memory");                           \
    result;                                      \
})

#define ATOMIC_READ(addr)                        \
({                                               \
    int read_val;                                \
    __asm__ __volatile__ ("                      \
        1:      ldr %0, [%1];                    \
                cmn %0, #2;                      \
                beq 1b;"                         \
    : "=&r" (read_val) : "r" (addr) : "cc");     \
    read_val;                                    \
})

#define ATOMIC_WRITE(addr, new_val)              \
do {                                             \
    int read_val;                                \
    __asm__ __volatile__ ("                      \
                mvn %0, #1;                      \
        1:      swp %0, %0, [%1];                \
                cmn %0, #2;                      \
                beq 1b;                          \
                str %2, [%1];"                   \
    : "=&r" (read_val)                           \
    : "r" (addr), "r" (new_val)                  \
    : "cc", "memory");                           \
} while(0)

#define MBARRIER() __asm__ __volatile__ ("" ::: "memory")
#define UNLOCK_MBARRIER() __asm__ __volatile__ ("" ::: "memory")
#define JMM_LOCK_MBARRIER() __asm__ __volatile__ ("" ::: "memory")
#define JMM_UNLOCK_MBARRIER() __asm__ __volatile__ ("" ::: "memory")

