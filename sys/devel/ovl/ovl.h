/*
 * ovl.h
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 */
/* overlay address space (overlay memory) */

/* TODO:
 * Size of Overlay Space is a smaller portion of VM Space.
 * Taken from VM_MAX_ADDRESS and VM_MIN_ADDRESS.
 *
 * OVA_MAX_ADDRESS = VM_MAX_ADDRESS - x amount
 * OVA_MIN_ADDRESS = VM_MIN_ADDRESS - x amount
 */
/*	koverlay:
 * 	-
 * 	- Should be changed ovlspace allocation
 * 		- as to access and allocate a portion of physical memory
 * 		- ovlspace can be split
 */
#ifndef _OVL_H_
#define _OVL_H_

#define OVL_MIN_KERNEL_ADDRESS
#define OVL_MAX_KERNEL_ADDRESS
#define OVL_MIN_VIRTUAL_ADDRESS
#define OVL_MAX_VIRTUAL_ADDRESS

struct vovl_map;
typedef struct vovl_map 		*vovl_map_t;

struct vovl_map_entry;
typedef struct vovl_map_entry	*vovl_map_entry_t;

#include "koverlay.h"
#include <sys/queue.h>

#include <vm/include/vm.h>
#include <ovl_map.h>

/*
 * virtual overlay address space.
 */
struct vovlspace {
	struct ovl_map 	    vovl_map;	    	/* Overlay address */
	struct pmap    		vovl_pmap;	    	/* private physical map */

	int		        	vovl_refcnt;	   	/* number of references */
	segsz_t 			vovl_tsize;			/* text size */
	segsz_t 			vovl_dsize;			/* data size */
	segsz_t 			vovl_ssize;			/* stack size */
	caddr_t	        	vovl_taddr;			/* user overlay address of text */
	caddr_t	        	vovl_daddr;			/* user overlay address of data */
	caddr_t         	vovl_minsaddr;		/* user OVA at min stack growth */
	caddr_t         	vovl_maxsaddr;		/* user OVA at max stack growth */
};


#endif /* SYS_DEVEL_OVL_OVL_H_ */
