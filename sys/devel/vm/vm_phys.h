/*
 * vm_phys.h
 *
 *  Created on: 21 Oct 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_VM_VM_PHYS_H_
#define SYS_DEVEL_VM_VM_PHYS_H_

struct vm_physseg {
	caddr_t 		start;
	caddr_t			end;
	caddr_t 		avail_start;
	caddr_t 		avail_end;
	vm_page_t		pgs;
	vm_page_t		lastpg;
};
typedef struct vm_physseg *vm_physseg_t;

caddr_t vm_physseg_get_start(vm_physseg_t);
caddr_t vm_physseg_get_end(vm_physseg_t);

caddr_t vm_physseg_get_avail_start(vm_physseg_t);
caddr_t vm_physseg_get_avail_end(vm_physseg_t);


#endif /* SYS_DEVEL_VM_VM_PHYS_H_ */
