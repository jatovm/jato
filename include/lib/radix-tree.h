#ifndef _VM_RADIX_TREE
#define _VM_RADIX_TREE

struct radix_tree {
	struct radix_tree_node *	root;
	int				bits_per_level;
	int				level_count;
};

struct radix_tree_node {
	struct radix_tree_node *	parent;
	int				count;	/* number of nonempty slots */
	void *				slots[0];
};

struct radix_tree *alloc_radix_tree(unsigned int bits_per_level,
				    unsigned int key_bits);
void free_radix_tree(struct radix_tree *tree);

int radix_tree_insert(struct radix_tree *tree, unsigned long key, void *value);
void radix_tree_remove(struct radix_tree *tree, unsigned long key);
void *radix_tree_lookup(struct radix_tree *tree, unsigned long key);
void *radix_tree_lookup_prev(struct radix_tree *tree, unsigned long key);

#endif /* _VM_RADIX_TREE */
