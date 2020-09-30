/*
 * vm_object.h
 *
 *  Created on: 29 Sep 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_VM_SEGMENT_VM_OBJECT_H_
#define SYS_DEVEL_VM_SEGMENT_VM_OBJECT_H_

#include <sys/tree.h>

struct vm_object {
	struct seglist					segt;				/* resident memory segments */

	RB_ENTRY(vm_object)				object_tree;

	vm_pager_t						pager;				/* Where to get data */
	vm_offset_t						segment_offset;		/* Offset into segment */
	struct vm_object				*shadow;			/* My shadow */
	vm_offset_t						shadow_offset;		/* Offset in shadow */

	RB_ENTRY(vm_object)				cached_list;		/* for persistence */
};

RB_HEAD(vm_object_hash_head, vm_object_hash_entry);
struct vm_object_hash_entry {
	RB_ENTRY(vm_object_hash_entry)  hash_links;	/* hash chain links */
	vm_object_t			   			object;		/* object represented */
};

struct objecttree;
RB_HEAD(objecttree, vm_object);

struct objecttree	vm_object_cached_tree;	/* list of objects persisting */
int					vm_object_cached;		/* size of cached list */
simple_lock_data_t	vm_cache_lock;			/* lock for object cache */

struct objecttree	vm_object_tree;			/* list of allocated objects */
long				vm_object_count;		/* count of all objects */
simple_lock_data_t	vm_object_list_lock;	/* lock for object list and count */

#endif /* SYS_DEVEL_VM_SEGMENT_VM_OBJECT_H_ */
