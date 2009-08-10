#ifndef VM_GC_H
#define VM_GC_H

extern void *gc_safepoint_page;

void gc_init(void);

void gc_attach_thread(void);
void gc_detach_thread(void);

void gc_safepoint(void);

#endif
