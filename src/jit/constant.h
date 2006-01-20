#ifndef __CONSTANT_H
#define __CONSTANT_H

struct constant {
	union {
		unsigned long long value;
		double fvalue;
	};
};

#endif
