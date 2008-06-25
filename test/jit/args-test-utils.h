#ifndef __ARGS_TEST_UTILS_H
#define __ARGS_TEST_UTILS_H

void create_args(struct expression **args, int nr_args);
void push_args(struct compilation_unit *cu,struct expression **args, int nr_args);
void assert_args(struct expression **expected_args,int nr_args,
		 struct expression *args_list);

#endif
