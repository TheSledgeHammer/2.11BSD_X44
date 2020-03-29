/*
 * vm_hrbtree.c
 *
 *  Created on: 28 Mar 2020
 *      Author: marti
 */

#include <devel/vm/vm_hrbtree.h>
#include <stdlib.h>

unsigned int
hash(entry)
	uint32_t entry;
{
	return (prospector32(entry));
}

int
vm_hrbtree_cmp(hrbtree1, hrbtree2)
	struct vm_hashed_rbtree *hrbtree1, *hrbtree2;
{
    if(hrbtree1->hrbt_hindex > hrbtree2->hrbt_hindex) {
        return (1);
    } else if(hrbtree1->hrbt_hindex < hrbtree2->hrbt_hindex) {
        return (-1);
    }
    return (0);
}

void
vm_hrbtree_init(hrbtree)
	struct vm_hashed_rbtree *hrbtree;
{
	RB_INIT(hrbtree->hrbt_root);
}

void
vm_hrbtree_insert(hrbtree, entry)
	struct vm_hashed_rbtree *hrbtree;
	uint32_t entry;
{
	hrbtree->hrbt_hindex = hash(entry);
	RB_INSERT(vm_hashed_rbtree, hrbtree->hrbt_root, hrbtree);
}

int
vm_map_entry_cmp(vm_map1, vm_map2)
	struct vmmap_entry *vm_map1, *vm_map2;
{
	if(vm_map1->start > vm_map2->start) {
		return (1);
	} else if(vm_map1->start < vm_map2->start) {
		return (-1);
	}
	return (0);
}

int
vm_amap_entry_cmp(vm_amap1, vm_amap2)
	struct vmamap_entry *vm_amap1, *vm_amap2;
{
	if(vm_amap1->vm_anon > vm_amap2->vm_anon) {
		return (1);
	} else if(vm_amap1->vm_anon < vm_amap2->vm_anon) {
		return (-1);
	}
	return (0);
}
