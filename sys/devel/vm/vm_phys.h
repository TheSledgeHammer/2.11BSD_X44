/*
 * vm_phys.h
 *
 *  Created on: 21 Oct 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_VM_VM_PHYS_H_
#define SYS_DEVEL_VM_VM_PHYS_H_

struct vm_phys_freelist {
	struct pglist 	vpf_pg;
	int 			vpf_cnt;
};

struct vm_phys {
	caddr_t 		vp_start;
	caddr_t			vp_end;
	vm_page_t 		vp_first_page;
	vm_page_t 		vp_last_page;

	struct vm_phys_freelist (*freelist);
};

#endif /* SYS_DEVEL_VM_VM_PHYS_H_ */
