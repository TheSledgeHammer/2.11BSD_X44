/*
 * voverlay.h
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_OVL_VOVERLAY_H_
#define SYS_DEVEL_OVL_VOVERLAY_H_

#include <sys/tree.h>

struct voverlay_entry {
	RB_ENTRY(voverlay_entry)					vovl_entry;
    vm_offset_t		                    		vovle_start;		    /* start address */
	vm_offset_t		                    		vovle_end;		        /* end address */

};

struct voverlay {
	RB_HEAD(vovl_rbtree, voverlay_entry)		vovl_tree;
	vm_size_t		                    		vovl_size;		        /* virtual size */

#define	min_offset								vovl_tree.vovle_start
#define max_offset								vovl_tree.vovle_end
};


struct ovlspace {
	struct voverlay		*ovls_vovl;
	struct koverlay		*ovls_kovl;
};

/* number of overlay maps and entries to statically allocate */
#define MAX_VOVL 	NOVLSR
#define MAX_VOVLENT	NOVLPSR				/* not known */
#endif /* SYS_DEVEL_OVL_VOVERLAY_H_ */
