#ifndef JATO_LIB_OPTIONS_H
#define JATO_LIB_OPTIONS_H

#include <stdbool.h>

struct option {
	const char *name;

	bool arg;
	bool arg_is_adjacent;

	union {
		void (*func)(void);
		void (*func_arg)(const char *arg);
	} handler;
};

#define DEFINE_OPTION(_name, _handler) \
	{ .name = _name, .arg = false, .handler.func = _handler }

#define DEFINE_OPTION_ARG(_name, _handler) \
	{ .name = _name, .arg = true, .arg_is_adjacent = false, .handler.func_arg = _handler }

#define DEFINE_OPTION_ADJACENT_ARG(_name, _handler) \
	{ .name = _name, .arg = true, .arg_is_adjacent = true, .handler.func_arg = _handler }

const struct option *get_option(const struct option *options, unsigned long len, const char *name);

#endif
