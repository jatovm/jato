#ifndef __VM_CLASSLOADER_H
#define __VM_CLASSLOADER_H

struct vm_class;

struct vm_class *classloader_load(const char *class_name);

#endif
