#ifndef __VM_CLASSLOADER_H
#define __VM_CLASSLOADER_H

#include <stdbool.h>

extern bool opt_trace_classloader;

struct vm_class;
struct vm_object;

int classloader_add_to_classpath(const char *classpath);
int try_to_add_zip_to_classpath(const char *zip);
char *get_classpath(void);

void classloader_init(void);
struct vm_class *classloader_load(struct vm_object *loader,
				  const char *class_name);
struct vm_class *classloader_load_primitive(const char *class_name);
struct vm_class *classloader_find_class(const char *name);

#endif
