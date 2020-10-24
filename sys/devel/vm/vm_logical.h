/*
 * vm_logical.h
 *
 *  Created on: 21 Oct 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_VM_VM_LOGICAL_H_
#define SYS_DEVEL_VM_VM_LOGICAL_H_

struct vm_logical_freelist {
	struct seglist 	vlf_seg;
	int 			vlf_cnt;
};

struct vm_logical {
	caddr_t 		vl_start;
	caddr_t			vl_end;
	vm_segment_t 	vl_first_segment;
	vm_segment_t 	vl_last_segment;

	struct vm_logical_freelist (*vl_freelist);
};


#endif /* SYS_DEVEL_VM_VM_LOGICAL_H_ */
