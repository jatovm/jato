#ifndef JIT_GDB_H
#define JIT_GDB_H

#ifdef CONFIG_GDB

extern void gdb_init(void);
extern void gdb_register_method(struct vm_method *method);
extern void gdb_register_trampoline(struct vm_method *method);

#else /* CONFIG_GDB */

static inline void gdb_init(void)
{
}

static inline void gdb_register_method(struct vm_method *method)
{
}

static inline void gdb_register_trampoline(struct vm_method *method)
{
}

#endif /* CONFIG_GDB */

#endif

