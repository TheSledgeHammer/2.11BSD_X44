/*
 * vm_object.h
 *
 *  Created on: 29 Sep 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_VM_SEGMENT_VM_OBJECT_H_
#define SYS_DEVEL_VM_SEGMENT_VM_OBJECT_H_

struct vm_object {
	struct seg_tree			segt;				/* resident memory segments */

	vm_pager_t				pager;				/* Where to get data */
	vm_offset_t				segment_offset;		/* Offset into segment */
	struct vm_object		*shadow;			/* My shadow */
	vm_offset_t				shadow_offset;		/* Offset in shadow */
	//RB_ENTRY(vm_object)	cached_list;		/* for persistence */
};


struct vm_object_hash_entry {

};

#endif /* SYS_DEVEL_VM_SEGMENT_VM_OBJECT_H_ */
