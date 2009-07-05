/* src/vm/jit/disass.h - disassembler header

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Christian Thalinger

   Changes:

*/


#ifndef _DISASS_H
#define _DISASS_H

#include "jit/compilation-unit.h"
#include <dis-asm.h>
#include <stdbool.h>

/* some macros ****************************************************************/

#define DISASSINSTR(code) \
    (code) = disassinstr((code))

#define DISASSEMBLE(start,end) \
    disassemble((start), (end))


/* export global variables ****************************************************/

extern disassemble_info info;
extern bool disass_initialized;

extern char *regs[];

extern char disass_buf[512];
extern unsigned long disass_len;


/* function prototypes *******************************************************/

void disassemble(struct compilation_unit *cu, void *start, void *end);

void disass_printf(PTR p, const char *fmt, ...);

int disass_buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length, struct disassemble_info *info);

/* machine dependent functions */

unsigned char *disassinstr(struct compilation_unit *cu, unsigned char *code);

#endif /* _DISASS_H */
