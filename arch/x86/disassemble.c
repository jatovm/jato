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
#include <dis-asm.h>

/* global variables ***********************************************************/

disassemble_info info;
bool disass_initialized = false;


/* We need this on i386 and x86_64 since we don't know the byte length
   of currently printed instructions.  512 bytes should be enough. */

char disass_buf[512];
unsigned long disass_len;

/* machine dependent functions */

#ifdef CONFIG_X86_32

# define JIT_BFD_MACH	bfd_mach_i386_i386

#else /* CONFIG_X86_32 */

# define JIT_BFD_MACH	bfd_mach_x86_64

#endif /* CONFIG_X86_32 */

static void disass_printf(PTR p, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	disass_len += vsprintf(disass_buf + disass_len, fmt, ap);

	va_end(ap);
}

static int disass_buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length, struct disassemble_info *info)
{
	memcpy(myaddr, (void *) (unsigned long) memaddr, length);

	return 0;
}

static unsigned char *disassemble_insn(struct compilation_unit *cu, unsigned char *code)
{
	unsigned long seqlen;
	unsigned long i;

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

void disassemble(struct vm_method *vmm)
{
	struct compilation_unit *cu = vmm->compilation_unit;
	struct jit_trampoline *trampoline = vmm->trampoline;
	void *start, *end;

	if (!disass_initialized) {
		INIT_DISASSEMBLE_INFO(info, NULL, disass_printf);

		/* setting the struct members must be done after
		   INIT_DISASSEMBLE_INFO */

		info.mach             = JIT_BFD_MACH;
		info.read_memory_func = &disass_buffer_read_memory;

		disass_initialized = 1;
	}

	trace_printf("# Trampoline: \n");

	start = buffer_ptr(trampoline->objcode);
	end = buffer_current(trampoline->objcode);

	for (; start < end; )
		start = disassemble_insn(cu, start);

	trace_printf("\n# Method: \n");

	start = buffer_ptr(cu->objcode);
	end = buffer_current(cu->objcode);

	for (; start < end; )
		start = disassemble_insn(cu, start);
}
