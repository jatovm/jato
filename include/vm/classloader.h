#ifndef __VM_CLASSLOADER_H
#define __VM_CLASSLOADER_H

extern bool opt_trace_classloader;

struct vm_class;

int classloader_add_to_classpath(const char *classpath);

struct vm_class *classloader_load(const char *class_name);
struct vm_class *classloader_load_primitive(const char *class_name);

#endif
