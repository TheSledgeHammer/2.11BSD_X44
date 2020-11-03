/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYS_DEVEL_VM_LVM_VM_LVM_H_
#define SYS_DEVEL_VM_LVM_VM_LVM_H_

#include <sys/queue.h>

struct vm_lv;
struct vm_logical_volume {
	/* logical volume */
	TAILQ_ENTRY(vm_logical_volume) 	lv_entries;		   	/* next logical volume */
	struct vm_volume_group          *lv_vg;             /* volume group */

    vm_size_t		    			lv_size;            /* logical volume size */
	caddr_t 						lv_start;			/* start offset */
	caddr_t							lv_end;				/* end offset */
	int 							lv_refcnt;			/* reference counter */

	caddr_t 						lv_avail_start;
	caddr_t							lv_avail_end;

	/* vm_segments */
	vm_segment_t 					lv_first_segment;	/* first segment */
	vm_segment_t 					lv_last_segment;	/* last segment */
	vm_offset_t						lv_first_laddr;		/* first logical segment address */
	vm_offset_t						lv_last_laddr;   	/* last logical segment address */
	struct seglist 					*lv_seglist;		/* segment list in vm_segment */
	vm_segment_t        			lv_segment_array;   /* segment array */
	vm_size_t           			lv_nsegments;       /* segment array size / number of segments */
};

struct vm_pv;
struct vm_physical_volume {
    /* physical volume */
    TAILQ_ENTRY(vm_physical_volume) pv_entries;         /* next physical volume */
    struct vm_volume_group          *pv_vg;             /* volume group */

    vm_size_t		    			pv_size;            /* physical volume size */
    caddr_t		    				pv_start;           /* start offset */
    caddr_t         				pv_end;             /* end offset */
    int			        			pv_refcnt;          /* reference counter */

	caddr_t 						pv_avail_start;
	caddr_t 						pv_avail_end;

    /* vm_pages */
    long                			pv_first_page;      /* first page */
    long                			pv_last_page;       /* last page */
    vm_offset_t         			pv_first_paddr;     /* first physical page address */
    vm_offset_t         			pv_last_paddr;      /* last physical page address */
    struct pglist       			*pv_pglist;         /* page list in vm_page */
    vm_page_t           			pv_page_array;      /* page array */
    vm_size_t           			pv_npages;          /* page array size / number of pages */
};

struct vm_vg;
TAILQ_HEAD(vm_vg, vm_volume_group);
struct vm_volume_group {
	TAILQ_ENTRY(vm_volume_group) 			vg_entries;
	TAILQ_HEAD(vm_pv, vm_physical_volume) 	vg_pvs;
	TAILQ_HEAD(vm_lv, vm_logical_volume) 	vg_lvs;
};

#endif /* SYS_DEVEL_VM_LVM_VM_LVM_H_ */
