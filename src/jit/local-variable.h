#ifndef __LOCAL_VARIABLE_H
#define __LOCAL_VARIABLE_H

#include <jvm_types.h>

struct local_variable {
	enum jvm_type type;
	unsigned long index;
};

#endif
