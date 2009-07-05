/* src/vm/jit/disass-common.c - common functions for GNU binutils disassembler

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

#include "jit/disassemble.h"

#include <dis-asm.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* global variables ***********************************************************/

disassemble_info info;
bool disass_initialized = false;


/* We need this on i386 and x86_64 since we don't know the byte length
   of currently printed instructions.  512 bytes should be enough. */

char disass_buf[512];
unsigned long disass_len;


/* disassemble *****************************************************************

   Outputs a disassembler listing of some machine code on `stdout'.

   start: pointer to first machine instruction
   end:   pointer after last machine instruction

*******************************************************************************/

void disassemble(struct compilation_unit *cu, void *start, void *end)
{
	for (; start < end; )
		start = disassinstr(cu, start);
}


/* disass_printf ***************************************************************

   Required by binutils disassembler.  This just prints the
   disassembled instructions to stdout.

*******************************************************************************/

void disass_printf(PTR p, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	disass_len += vsprintf(disass_buf + disass_len, fmt, ap);

	va_end(ap);
}


/* buffer_read_memory **********************************************************

   We need to replace the buffer_read_memory from binutils.

*******************************************************************************/

int disass_buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length, struct disassemble_info *info)
{
	memcpy(myaddr, (void *) (unsigned long) memaddr, length);

	return 0;
}
