/*
 * Copyright (C) 2009  Vegard Nossum
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/options.h"

#include <string.h>

const struct option *get_option(const struct option *options, unsigned long len, const char *name)
{
	for (unsigned int i = 0; i < len; ++i) {
		const struct option *opt = &options[i];

		if (opt->arg && opt->arg_is_adjacent &&
		    !strncmp(name, opt->name, strlen(opt->name)))
			return opt;

		if (!strcmp(name, opt->name))
			return opt;
	}

	return NULL;
}
