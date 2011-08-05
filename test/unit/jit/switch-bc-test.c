#include "vm/method.h"
#include "vm/system.h"
#include "vm/vm.h"
#include "vm/trace.h"

#include "jit/compiler.h"
#include "lib/bitset.h"
#include <libharness.h>
#include <basic-block-assert.h>

/* Test1 for tableswitch:
	int tableswitch(int i) {
		switch (i) {
			case 0:  return 0;
			case 1:  return 1;
			case 2:  return 2;
			default: return -1;
		}
	}
 */
static unsigned char tableswitch1[36] = {
	0x1b, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x1d,
	0x00, 0x00, 0x00, 0x1f, 0x03, 0xac, 0x04, 0xac,
	0x05, 0xac, 0x02, 0xac,
};

void test_convert_tableswitch_bbs1(void)
{
	struct basic_block *bb0, *bb1, *bb2, *bb3, *bb4, *bb5, *bb6;
	struct compilation_unit *cu;

	struct cafebabe_method_info method_info;
	struct vm_method method = {
		.code_attribute.code = tableswitch1,
		.code_attribute.code_length = ARRAY_SIZE(tableswitch1),
		.method = &method_info,
	};

	cu = compilation_unit_alloc(&method);

	analyze_control_flow(cu);
	convert_to_ir(cu);
	assert_int_equals(7, nr_bblocks(cu));

	bb0 = bb_entry(cu->bb_list.next);
	bb1 = bb_entry(bb0->bb_list_node.next);
	bb2 = bb_entry(bb1->bb_list_node.next);
	bb3 = bb_entry(bb2->bb_list_node.next);
	bb4 = bb_entry(bb3->bb_list_node.next);
	bb5 = bb_entry(bb4->bb_list_node.next);
	bb6 = bb_entry(bb5->bb_list_node.next);

	assert_basic_block_successors((struct basic_block*[]){bb6, bb1}, 2, bb0);
	assert_basic_block_successors((struct basic_block*[]){bb6, bb2}, 2, bb1);
	assert_basic_block_successors((struct basic_block*[]){bb3, bb4, bb5}, 3, bb2);
	assert_basic_block_successors((struct basic_block*[]){ }, 0, bb3);
	assert_basic_block_successors((struct basic_block*[]){ }, 0, bb4);
	assert_basic_block_successors((struct basic_block*[]){ }, 0, bb5);
	assert_basic_block_successors((struct basic_block*[]){ }, 0, bb6);
}

/* Test2 for tableswitch:
	void tableswitch() {
		int a = 1;
		switch (a) {
			case 0:
				a = 10;
			case 1:
				a = 30;
			case 5:
				a = 50;
		}
	}
 */
static unsigned char tableswitch2[46] = {
	0x04, 0x3c, 0x1b, 0xab, 0x00, 0x00, 0x00, 0x2a,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x05,
	0x00, 0x00, 0x00, 0x27, 0x10, 0x0a, 0x3c, 0x10,
	0x1e, 0x3c, 0x10, 0x28, 0x3c, 0xb1,
};

void test_convert_tableswitch_bbs2(void)
{
	struct basic_block *bb0, *bb1, *bb2, *bb3, *bb4, *bb5;
	struct compilation_unit *cu;

	struct cafebabe_method_info method_info;
	struct vm_method method = {
		.code_attribute.code = tableswitch2,
		.code_attribute.code_length = ARRAY_SIZE(tableswitch2),
		.method = &method_info,
	};

	cu = compilation_unit_alloc(&method);

	analyze_control_flow(cu);
	convert_to_ir(cu);
	assert_int_equals(6, nr_bblocks(cu));

	bb0 = bb_entry(cu->bb_list.next);
	bb1 = bb_entry(bb0->bb_list_node.next);
	bb2 = bb_entry(bb1->bb_list_node.next);
	bb3 = bb_entry(bb2->bb_list_node.next);
	bb4 = bb_entry(bb3->bb_list_node.next);
	bb5 = bb_entry(bb4->bb_list_node.next);

	assert_basic_block_successors((struct basic_block*[]){bb5, bb1}, 2, bb0);
	assert_basic_block_successors((struct basic_block*[]){bb2, bb3, bb4}, 3, bb1);
	assert_basic_block_successors((struct basic_block*[]){bb3}, 1, bb2);
	assert_basic_block_successors((struct basic_block*[]){bb4}, 1, bb3);
	assert_basic_block_successors((struct basic_block*[]){bb5}, 1, bb4);
	assert_basic_block_successors((struct basic_block*[]){ }, 0, bb5);
}

/* Test1 for lookupswitch:
	public void lookupswitch()
	{
		int a = 1;

		switch(a)
		{
			case 0:
				a = 0;
			case 1:
				a = 1;
			default:
				a = 3;
		}

	}
 */
static unsigned char lookupswitch1[35] = {
	0x04, 0x3c, 0x1b, 0xab, 0x00, 0x00, 0x00, 0x1d,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x1b, 0x03, 0x3c, 0x04, 0x3c,
	0x06, 0x3c, 0xb1,
};

void test_convert_lookupswitch_bbs1(void)
{
	struct basic_block *bb0, *bb1, *bb2, *bb3, *bb4;
	struct compilation_unit *cu;

	struct cafebabe_method_info method_info;
	struct vm_method method = {
		.code_attribute.code = lookupswitch1,
		.code_attribute.code_length = ARRAY_SIZE(lookupswitch1),
		.method = &method_info,
	};

	cu = compilation_unit_alloc(&method);

	analyze_control_flow(cu);
	convert_to_ir(cu);
	assert_int_equals(5, nr_bblocks(cu));

	bb0 = bb_entry(cu->bb_list.next);
	bb1 = bb_entry(bb0->bb_list_node.next);
	bb2 = bb_entry(bb1->bb_list_node.next);
	bb3 = bb_entry(bb2->bb_list_node.next);
	bb4 = bb_entry(bb3->bb_list_node.next);

	assert_basic_block_successors((struct basic_block*[]){bb4, bb1}, 2, bb0);
	assert_basic_block_successors((struct basic_block*[]){bb2, bb3}, 2, bb1);
	assert_basic_block_successors((struct basic_block*[]){bb3}, 1, bb2);
	assert_basic_block_successors((struct basic_block*[]){bb4}, 1, bb3);
	assert_basic_block_successors((struct basic_block*[]){ }, 0, bb4);
}
