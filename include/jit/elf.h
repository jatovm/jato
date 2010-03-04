#ifndef JIT_ELF_H
#define JIT_ELF_H

extern void *elf_init(void);
extern void elf_add_symbol(char *name, void *addr, size_t size);
extern size_t elf_get_size(void);

#endif
