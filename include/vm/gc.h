#ifndef VM_GC_H
#define VM_GC_H

#include <stdbool.h>

struct register_state;

extern void *gc_safepoint_page;
extern bool verbose_gc;
extern bool gc_enabled;

void gc_init(void);

void *gc_alloc(size_t size);

void gc_safepoint(struct register_state *);
void suspend_handler(int, siginfo_t *, void *);

#endif
