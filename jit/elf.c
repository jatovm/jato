/*
 * ELF object file generation (currently used by GDB support).
 *
 * Copyright (C) 2010 Eduard - Gabriel Munteanu
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifdef CONFIG_GDB

#include <elf.h>
#include <string.h>

#include "jit/compilation-unit.h"
#include "jit/elf.h"
#include "jit/gdb.h"
#include "jit/text.h"

#ifdef CONFIG_32_BIT

typedef Elf32_Ehdr	elf_ehdr_t;
typedef Elf32_Shdr	elf_shdr_t;
typedef Elf32_Sym	elf_sym_t;

# define MACH_ELFCLASS	ELFCLASS32

#else /* CONFIG_32_BIT */

typedef Elf64_Ehdr	elf_ehdr_t;
typedef Elf64_Shdr	elf_shdr_t;
typedef Elf64_Sym	elf_sym_t;

# define MACH_ELFCLASS	ELFCLASS64

#endif /* CONFIG_32_BIT */

#ifdef CONFIG_X86_32
# define MACH_ARCH	EM_386
#elif CONFIG_X86_64
# define MACH_ARCH	EM_X86_64
#else
# error "GDB support is not available on this arch!"
#endif

/* ELF layout and data. */
#define OSABI		ELFOSABI_SYSV
#define SHSTRTAB	"\0.shstrtab\0.strtab\0.text\0.symtab"
#define SHSTRTAB_START	sizeof(Elf64_Ehdr)
#define SHSTRTAB_SIZE	sizeof(SHSTRTAB)
#define STRTAB_START	(SHSTRTAB_START + SHSTRTAB_SIZE)
#define STRTAB_SIZE	(256 * 1024)
#define SYMTAB_START	(STRTAB_START + STRTAB_SIZE)
#define SYMTAB_SIZE	STRTAB_SIZE
#define SHTABLE_START	(SYMTAB_START + SYMTAB_SIZE)
#define N_SECTIONS	5
#define GDB_BUF_SIZE	(SHTABLE_START + N_SECTIONS * sizeof(Elf64_Shdr))

static void *elf_buf;

static elf_sym_t *symtab;
static size_t sym_count;

static unsigned char *strtab;
static size_t strtab_offset;

static void elf_init_ehdr(void)
{
	elf_ehdr_t *ehdr = elf_buf;

	ehdr->e_ident[0]	= ELFMAG0;
	ehdr->e_ident[1]	= ELFMAG1;
	ehdr->e_ident[2]	= ELFMAG2;
	ehdr->e_ident[3]	= ELFMAG3;
	ehdr->e_ident[4]	= MACH_ELFCLASS;
	ehdr->e_ident[5]	= ELFDATA2LSB;
	ehdr->e_ident[6]	= EV_CURRENT;
	ehdr->e_ident[7]	= OSABI;
	ehdr->e_ident[8]	= 0;
	ehdr->e_ident[9]	= 0;
	ehdr->e_ident[10]	= 0;
	ehdr->e_ident[11]	= 0;
	ehdr->e_ident[12]	= 0;
	ehdr->e_ident[13]	= 0;
	ehdr->e_ident[14]	= 0;
	ehdr->e_ident[15]	= 0;
	ehdr->e_type		= ET_EXEC;
	ehdr->e_machine		= MACH_ARCH;
	ehdr->e_version		= EV_CURRENT;
	ehdr->e_entry		= 0;
	ehdr->e_phoff		= 0;
	ehdr->e_shoff		= SHTABLE_START;
	ehdr->e_flags		= 0;
	ehdr->e_ehsize		= sizeof(elf_ehdr_t);
	ehdr->e_phentsize	= 0;
	ehdr->e_phnum		= 0;
	ehdr->e_shentsize	= sizeof(elf_shdr_t);
	ehdr->e_shnum		= N_SECTIONS;
	ehdr->e_shstrndx	= 1;
}

static void elf_init_sections(void)
{
	elf_shdr_t *shdr = elf_buf + SHTABLE_START;
	char shstrtab[] = SHSTRTAB;

	/* The entry with index 0 must be all zeros. */
	memset(&shdr[0], 0, sizeof(elf_shdr_t));

	/* .shstrtab */
	shdr[1].sh_name		= 1;
	shdr[1].sh_type		= SHT_STRTAB;
	shdr[1].sh_flags	= 0;
	shdr[1].sh_offset	= SHSTRTAB_START;
	shdr[1].sh_size		= SHSTRTAB_SIZE;
	shdr[1].sh_link		= SHN_UNDEF;
	shdr[1].sh_info		= 0;
	shdr[1].sh_addralign	= 1;
	shdr[1].sh_entsize	= 0;
	memcpy(elf_buf + SHSTRTAB_START, shstrtab, SHSTRTAB_SIZE);

	/* .strtab */
	shdr[2].sh_name		= 11;
	shdr[2].sh_type		= SHT_STRTAB;
	shdr[2].sh_flags	= 0;
	shdr[2].sh_offset	= STRTAB_START;
	shdr[2].sh_size		= STRTAB_SIZE;
	shdr[2].sh_link		= SHN_UNDEF;
	shdr[2].sh_info		= 0;
	shdr[2].sh_addralign	= 1;
	shdr[2].sh_entsize	= 0;

	/* .text */
	shdr[3].sh_name		= 19;
	shdr[3].sh_type		= SHT_PROGBITS;
	shdr[3].sh_flags	= SHF_ALLOC | SHF_EXECINSTR;
	shdr[3].sh_offset	= (unsigned long) (jit_text_ptr() - elf_buf);
	shdr[3].sh_size		= -1;	/* Maybe this needs to be fixed. */
	shdr[3].sh_link		= SHN_UNDEF;
	shdr[3].sh_info		= 0;
	shdr[3].sh_addralign	= 1;
	shdr[3].sh_entsize	= 0;

	/* .symtab */
	shdr[4].sh_name		= 25;
	shdr[4].sh_type		= SHT_SYMTAB;
	shdr[4].sh_flags	= SHF_ALLOC;
	shdr[4].sh_offset	= SYMTAB_START;
	shdr[4].sh_size		= SYMTAB_SIZE;
	shdr[4].sh_link		= 2;	/* Linked to .strtab */
	shdr[4].sh_info		= 0;
	shdr[4].sh_addralign	= 1;
	shdr[4].sh_entsize	= sizeof(elf_sym_t);

	strtab = elf_buf + shdr[2].sh_offset;

	/* First string is the null string. */
	strtab[0] = 0;
	strtab_offset = 1;

	symtab = elf_buf + shdr[4].sh_offset;
}

void *elf_init(void)
{
	elf_buf = jit_text_ptr();
	jit_text_reserve(GDB_BUF_SIZE);

	elf_init_ehdr();
	elf_init_sections();

	return elf_buf;
}

void elf_add_symbol(char *name, void *addr, size_t size)
{
	elf_sym_t *sym = &symtab[sym_count++];
	size_t len;

	assert(symtab);
	assert(sym + 1 - symtab <= SYMTAB_SIZE);

	sym->st_name	= (unsigned long) strtab_offset;
	sym->st_info	= (STB_GLOBAL << 4) | STT_FUNC;
	sym->st_other	= 0;
	sym->st_shndx	= 3;
	sym->st_value	= (unsigned long) addr;
	sym->st_size	= size;

	len = strlen(name) + 1;
	assert(strtab_offset + len <= STRTAB_SIZE);
	memcpy(strtab + strtab_offset, name, len);
	strtab_offset += len;
}

size_t elf_get_size(void)
{
	return jit_text_ptr() - elf_buf;
}

#endif
