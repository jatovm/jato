#ifndef _VM_JAR_H
#define _VM_JAR_H

#include "lib/list.h"

struct jar_section {
	struct list_head headers;
};

struct jar_header {
	char *name;
	char *value;

	struct list_head node;
};

struct jar_main_section {
	unsigned int major_version;
	unsigned int minor_version;

	/* List of 'struct jar_header' */
	struct list_head main_attributes;
};

struct jar_individual_section {
	struct jar_header *name_header;

	/* List of 'struct jar_header' */
	struct list_head perentry_attributes;

	struct list_head node;
};

struct jar_manifest {
	struct jar_main_section *main_section;
	struct list_head individual_sections;
};

struct vm_jar {
	struct jar_manifest *manifest;
};

struct vm_jar *vm_jar_open(const char *filename);

const char *vm_jar_get_main_class(const struct vm_jar *jar);

#endif
