/*	$NetBSD: uvm.h,v 1.75 2020/02/23 15:46:43 ad Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 *
 * from: Id: uvm.h,v 1.1.2.14 1998/02/02 20:07:19 chuck Exp
 */


#ifndef _UVM_UVM_H_
#define _UVM_UVM_H_

#include <vm/include/vm.h>
#include <devel/vm/vm_amap.h>
#include <devel/vm/vm_aobject.h>

/*
 * TODO:
 * krwlock_t?
 * uvm_amap: vsize_t
 * uvm_aobj: voff_t
 */

struct vm_amap;
typedef struct vm_amap *vm_amap_t;

struct uvm_aobj;
typedef struct uvm_aobj *vm_aobj_t;

#ifdef _KERNEL
/*
 * pull in VM_NFREELIST
 */
#include <machine/vmparam.h>
/*
 * uvm structure (vm global state: collected in one structure for ease
 * of reference...)
 */

struct uvm {
	/* vm_page related parameters */
	/* vm_page queues */
	struct pgfreelist 	page_free[VM_NFREELIST]; /* unallocated pages */
	u_int				bucketcount;
	boolean_t			page_init_done;			/* true if uvm_page_init() finished */
	boolean_t			numa_alloc;				/* use NUMA page allocation strategy */

	/* page daemon trigger */
	int 				pagedaemon;				/* daemon sleeps on this */
	struct proc 		*pagedaemon_proc;		/* daemon's lid */
};

#endif /* _KERNEL */

#endif /* _UVM_UVM_H_ */
