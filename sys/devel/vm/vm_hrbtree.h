
/* Hashed Red-Black Tree */

#include <sys/queue.h>
#include <sys/tree.h>
#include <devel/hashedrbtree.h>

struct vm_hrbtree;
HRB_HEAD(vm_hrbtree, vm_hashed_rbtree);
struct vm_hashed_rbtree
{
	struct vm_hrbtree				hrbt_root;
	HRB_ENTRY(vm_hashed_rbtree)		hrbt_entry;
	struct vm_map_hrb				hrbt_vm_map;
	struct vm_amap_hrb				hrbt_vm_amap;
};

struct vm_map_hrb;
HRB_HEAD(vm_map_hrb, vm_map_entry);
struct vm_map_entry {
	struct vm_map_hrb				hrb_root;
	HRB_ENTRY(vm_map_entry)			hrb_node;
};

struct vm_map {

};

struct vm_amap_hrb;
HRB_HEAD(vm_amap_hrb, vm_amap_entry);
struct vm_amap_entry {
	struct vm_amap_hrb				hrb_root;
	HRB_ENTRY(vm_amap_entry)		hrb_node;
	struct vm_amap					vm_amap;
};

struct vm_amap {
	struct vm_anon 	*vm_anon;

};

void vm_hrbtree_init(struct vm_hashed_rbtree *hrbtree);
