#ifndef _VM_RADIX_TREE
#define _VM_RADIX_TREE

struct radix_tree;

struct radix_tree *alloc_radix_tree(unsigned int bits_per_level,
				    unsigned int key_bits);
void free_radix_tree(struct radix_tree *tree);

int tree_put(struct radix_tree *tree, unsigned long key, void *value);
void tree_remove(struct radix_tree *tree, unsigned long key);
void *tree_lookup(struct radix_tree *tree, unsigned long key);
void *tree_lookup_preceeding(struct radix_tree *tree, unsigned long key);

#endif /* _VM_RADIX_TREE */
