#ifndef __ARGS_TEST_UTILS_H
#define __ARGS_TEST_UTILS_H

struct basic_block;
struct expression;

void create_args(struct expression **args, int nr_args);
void push_args(struct basic_block *bb, struct expression **args, int nr_args);
void assert_args(struct expression **expected_args, int nr_args, struct expression *args_list);

#endif
