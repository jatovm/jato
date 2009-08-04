/* src/vm/jit/i386/disass.c - wrapper functions for GNU binutils disassembler

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

   Authors: Andreas  Krall
            Reinhard Grafl

   Changes: Christian Thalinger

*/

#include "jit/bc-offset-mapping.h"
#include "jit/disassemble.h"
#include "jit/compiler.h"
#include "lib/string.h"

#include "vm/trace.h"

#include <assert.h>
#include <dis-asm.h>
#include <stdarg.h>

char *regs[] = {
	"eax",
	"ecx",
	"edx",
	"ebx",
	"esp",
	"ebp",
	"esi",
	"edi"
};


/* disassinstr *****************************************************************

   Outputs a disassembler listing of one machine code instruction on
   'stdout'.

   code: instructions machine code

*******************************************************************************/

unsigned char *disassinstr(struct compilation_unit *cu, unsigned char *code)
{
	unsigned long seqlen;
	unsigned long i;

	if (!disass_initialized) {
		INIT_DISASSEMBLE_INFO(info, NULL, disass_printf);

		/* setting the struct members must be done after
		   INIT_DISASSEMBLE_INFO */

		info.mach             = bfd_mach_i386_i386;
		info.read_memory_func = &disass_buffer_read_memory;

		disass_initialized = 1;
	}

	if (opt_trace_bytecode_offset) {
		struct string *str = alloc_str();
		unsigned long bc_offset;

		bc_offset = jit_lookup_bc_offset(cu, code);
		print_bytecode_offset(bc_offset, str);
		trace_printf("[ %5s ]", str->value);
		free_str(str);
	}

	trace_printf("  0x%08lx:   ", (unsigned long) code);

	disass_len = 0;

	seqlen = print_insn_i386((bfd_vma) (unsigned long) code, &info);

	for (i = 0; i < seqlen; i++, code++) {
		trace_printf("%02x ", *code);
	}

	for (; i < 8; i++) {
		trace_printf("   ");
	}

	trace_printf("   %s\n", disass_buf);

	return code;
}
