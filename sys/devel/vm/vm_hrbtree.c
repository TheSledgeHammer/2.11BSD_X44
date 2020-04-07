/*
 * vm_hrbtree.c
 *
 *  Created on: 28 Mar 2020
 *      Author: marti
 */

#include <devel/vm/vm_hrbtree.h>
#include <stdlib.h>

HRB_PROTOTYPE(vm_hrbtree, vm_hashed_rbtree, hrbt_entry, vm_hrbtree_cmp);
HRB_GENERATE(vm_hrbtree, vm_hashed_rbtree, hrbt_entry, vm_hrbtree_cmp);

int
vm_hrbtree_cmp(map1, map2)
	struct vm_hashed_rbtree *map1, *map2;
{
	if(map1 > map2) {
		return (1);
	} else if(map1 < map2) {
		return (-1);
	}
	return (0);
}

HRB_PROTOTYPE(vm_map_hrb, vm_map_entry, hrb_node, vm_map_entry_cmp);
HRB_GENERATE(vm_map_hrb, vm_map_entry, hrb_node, vm_map_entry_cmp);
int
vm_map_entry_cmp(vm_map1, vm_map2)
	struct vm_map_entry *vm_map1, *vm_map2;
{
	if(vm_map1->start > vm_map2->start) {
		return (1);
	} else if(vm_map1->start < vm_map2->start) {
		return (-1);
	}
	return (0);
}

HRB_PROTOTYPE(vm_amap_hrb, vm_amap_entry, hrb_node, vm_amap_entry_cmp);
HRB_GENERATE(vm_amap_hrb, vm_amap_entry, hrb_node, vm_amap_entry_cmp);
int
vm_amap_entry_cmp(vm_amap1, vm_amap2)
	struct vm_amap_entry *vm_amap1, *vm_amap2;
{
	if(vm_amap1->vm_amap > vm_amap2->vm_amap) {
		return (1);
	} else if(vm_amap1->vm_amap < vm_amap2->vm_amap) {
		return (-1);
	}
	return (0);
}

hrbtree_init(hrb)
	struct vm_hashed_rbtree *hrb;
{
	HRB_INIT(&hrb->hrbt_root);
}

hrbtree_insert_vm_map(hrb, map)
	struct vm_hashed_rbtree *hrb;
	struct vm_map_entry *map;
{
	hrb->hrbt_vm_map = map;
	HRB_INSERT(vm_hrbtree, hrb, map);
}

hrbtree_insert_vm_amap(hrb, amap)
	struct vm_hashed_rbtree *hrb;
	struct vm_amap_entry *amap;
{
	hrb->hrbt_vm_amap = amap;
	HRB_INSERT(vm_hrbtree, hrb, amap);
}
