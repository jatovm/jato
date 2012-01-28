#include "vm/method.h"
#include "vm/system.h"
#include "vm/vm.h"
#include "vm/trace.h"

#include "jit/compiler.h"
#include "lib/bitset.h"
#include <libharness.h>
#include <basic-block-assert.h>

/* A complex test:
	public void dominance()
	{
		int a, b, c;

		a = 1;
		b = 4;
		c = 5;

		if (a > 2)
		{
			a = 3;
		}else
		{
			while(b > 3)
			{
				if (a == 1)
				{
					c = 5;
				}else{
					c = 4;
				}

				b--;
			}
		}
	}
*/
static unsigned char dominance[40] = {
	/* 0 */ OPC_ICONST_1,
	/* 1 */ OPC_ISTORE_1,
	/* 2 */ OPC_ICONST_4,
	/* 3 */ OPC_ISTORE_2,
	/* 4 */ OPC_ICONST_5,
	/* 5 */ OPC_ISTORE_3,
	/* 6 */ OPC_ILOAD_1,
	/* 7 */ OPC_ICONST_2,
	/* 8 */ OPC_IF_ICMPLE, 0x00, 0x08,
	/* 11 */ OPC_ICONST_3,
	/* 12 */ OPC_ISTORE_1,
	/* 13 */ OPC_GOTO, 0x00, 0x1a,
	/* 16 */ OPC_ILOAD_2,
	/* 17 */ OPC_ICONST_3,
	/* 18 */ OPC_IF_ICMPLE, 0x00, 0x15,
	/* 21 */ OPC_ILOAD_1,
	/* 22 */ OPC_ICONST_1,
	/* 23 */ OPC_IF_ICMPNE, 0x00, 0x08,
	/* 26 */ OPC_ICONST_5,
	/* 27 */ OPC_ISTORE_3,
	/* 28 */ OPC_GOTO, 0x00, 0x05,
	/* 31 */ OPC_ICONST_4,
	/* 32 */ OPC_ISTORE_3,
	/* 33 */ OPC_IINC, 0x02, 0xff,
	/* 36 */ OPC_GOTO, 0xff, 0xec,
	/* 39 */ OPC_RETURN,
};

void test_dominance(void)
{
	struct basic_block *bb0, *bb1, *bb2, *bb3, *bb4, *bb5, *bb6, *bb7;
	struct compilation_unit *cu;

	struct cafebabe_method_info method_info;
	struct vm_method method = {
			.code_attribute.code = dominance,
			.code_attribute.code_length = ARRAY_SIZE(dominance),
			.method = &method_info,
	};

	memset(&method_info, 0, sizeof(method_info));

	cu = compilation_unit_alloc(&method);

	analyze_control_flow(cu);
	assert_int_equals(8, nr_bblocks(cu));

	bb0 = bb_entry(cu->bb_list.next);
	bb1 = bb_entry(bb0->bb_list_node.next);
	bb2 = bb_entry(bb1->bb_list_node.next);
	bb3 = bb_entry(bb2->bb_list_node.next);
	bb4 = bb_entry(bb3->bb_list_node.next);
	bb5 = bb_entry(bb4->bb_list_node.next);
	bb6 = bb_entry(bb5->bb_list_node.next);
	bb7 = bb_entry(bb6->bb_list_node.next);

	compute_dfns(cu);
	compute_dom(cu);
	compute_dom_frontier(cu);

	struct bitset *expected = alloc_bitset(cu->nr_vregs);
	bitset_clear_all(expected);

	/* There is no block in the dominance frontier set
	 * of block 0 */
	assert_bitset_equals(bb0->dom_frontier, expected, bb0->dfn);

	/* Block 2 is in the dominance frontier set
	 * of block 1 */
	set_bit(expected->bits, 2);
	assert_bitset_equals(bb1->dom_frontier, expected, bb1->dfn);

	/* Block 2, 3 are in the dominance frontier set
	 * of block 2 */
	set_bit(expected->bits, 3);
	assert_bitset_equals(bb2->dom_frontier, expected, bb2->dfn);

	/* Block 3 is in the dominance frontier set
	 * of block 3 */
	clear_bit(expected->bits, 2);
	assert_bitset_equals(bb3->dom_frontier, expected, bb3->dfn);

	/* Block 6 is in the dominance frontier set
	 * of block 4 */
	clear_bit(expected->bits, 3);
	set_bit(expected->bits, 6);
	assert_bitset_equals(bb4->dom_frontier, expected, bb4->dfn);

	/* Block 6 is in the dominance frontier set
	 * of block 5 */
	assert_bitset_equals(bb5->dom_frontier, expected, bb5->dfn);

	/* Block 3 is in the dominance frontier set
	 * of block 6 */
	clear_bit(expected->bits, 6);
	set_bit(expected->bits, 3);
	assert_bitset_equals(bb6->dom_frontier, expected, bb6->dfn);

	/* There is no block in the dominance frontier set
	 * of block 7 */
	clear_bit(expected->bits, 3);
	assert_bitset_equals(bb7->dom_frontier, expected, bb7->dfn);
}
