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

#define OS_ARCH "mips"

/* Override default min and max heap sizes.  MIPS machines are
   usually embedded, and the standard defaults are too large. */
#define DEFAULT_MAX_HEAP 8*MB
#define DEFAULT_MIN_HEAP 1*MB

#define HANDLER_TABLE_T static const void
#define DOUBLE_1_BITS 0x3ff0000000000000LL

#define READ_DBL(v,p,l)	v = ((u8)p[0]<<56)|((u8)p[1]<<48)|((u8)p[2]<<40) \
                            |((u8)p[3]<<32)|((u8)p[4]<<24)|((u8)p[5]<<16) \
                            |((u8)p[6]<<8)|(u8)p[7]; p+=8

#define FPU_HACK 

#define COMPARE_AND_SWAP(addr, old_val, new_val)  \
({                                                \
    int result, read_val;                         \
    __asm__ __volatile__ ("                       \
              .set      push\n                    \
              .set      mips2\n                   \
      1:      ll        %1,%2\n                   \
              move      %0,$0\n                   \
              bne       %1,%3,2f\n                \
              move      %0,%4\n                   \
              sc        %0,%2\n                   \
              .set      pop\n                     \
              beqz      %0,1b\n                   \
      2:"                                         \
    : "=&r" (result), "=&r" (read_val)            \
    : "m" (*addr), "r" (old_val), "r" (new_val)   \
    : "memory");                                  \
    result;                                       \
})

#define LOCKWORD_READ(addr) *addr
#define LOCKWORD_WRITE(addr, value) *addr = value
#define LOCKWORD_COMPARE_AND_SWAP(addr, old_val, new_val) \
        COMPARE_AND_SWAP(addr, old_val, new_val)

#define MBARRIER()              \
    __asm__ __volatile__ ("     \
        .set push\n             \
        .set mips2\n            \
        sync\n                  \
        .set  pop\n"            \
    ::: "memory")

#define UNLOCK_MBARRIER()     MBARRIER()
#define JMM_LOCK_MBARRIER()   MBARRIER()
#define JMM_UNLOCK_MBARRIER() MBARRIER()

