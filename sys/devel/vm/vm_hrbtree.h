
/* Hashed Red-Black Tree */

#include <sys/queue.h>
#include <sys/tree.h>

struct vm_hrbtree;
RB_HEAD(vm_hrbtree, vm_hashed_rbtree);
RB_PROTOTYPE(vm_hrbtree, vm_hashed_rbtree, hrbt_entry, vm_hrbtree_cmp);

struct vm_hashed_rbtree
{
	struct vm_hrbtree				hrbt_root;
	RB_ENTRY(vm_hashed_rbtree) 		hrbt_entry;			/* tree node */
	unsigned int        			hrbt_hindex;		/* Hash Index */

	vm_offset_t						hrbt_start;			/* start address */
};

struct vm_map_rbtree;
RB_HEAD(vm_map_rbtree, vmmap_entry);
RB_PROTOTYPE(vm_map_rbtree, vmmap_entry, vmrb_entry, vm_map_entry_cmp);

struct vmmap_entry {
	RB_ENTRY(vm_hashed_rbtree) 	vmrb_entry;
	vm_offset_t					start;						/* start address */
};

void vm_hrbtree_init(struct vm_hashed_rbtree *hrbtree);
