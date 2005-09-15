#ifndef __CONSTANT_H
#define __CONSTANT_H

enum constant_type {
	CONST_NULL,
	CONST_INT,
	CONST_LONG,
	CONST_FLOAT,
	CONST_DOUBLE,
};

struct constant {
	enum constant_type type;
	union {
		unsigned long long value;
		double fvalue;
	};
};

#endif
