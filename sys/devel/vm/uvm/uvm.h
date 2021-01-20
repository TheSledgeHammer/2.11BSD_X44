/*	$NetBSD: uvm.h,v 1.15.2.1 2000/04/26 22:11:19 he Exp $	*/

/*
 *
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _VM_UVM_H_
#define _VM_UVM_H_

#include <devel/vm/include/vm.h>

/* XXX: Temporary
 * uvm structure (vm global state: collected in one structure for ease
 * of reference...)
 * TODO: place missing in relevant structures
 */
struct uvm {
	/* swap-related items */
	simple_lock_data_t 			swap_data_lock;
};
extern struct uvm uvm;

/* XXX: Temporary
 * uvmexp: global data structures that are exported to parts of the kernel
 * other than the vm system.
 * TODO: place missing in vmmeter.h
 */
struct uvmexp {
	/* swap */
	int 						swpgonly;	/* number of swap pages in use, not also in RAM */
};
extern struct uvmexp uvmexp;

/* malloc.h */
#define M_VMAMAP 				81
#define M_VMAOBJ 				82

/* vm_page.h */
#define PQ_FREE					0x0001		/* page is on free list */
#define PQ_INACTIVE				0x0002		/* page is in inactive list */
#define PQ_ACTIVE				0x0004		/* page is in active list */
#define PQ_LAUNDRY				0x0008		/* page is being cleaned now */
#define PQ_ANON					0x0010		/* page is part of an anon, rather than an vm_object */
#define PQ_AOBJ					0x0020		/* page is part of an anonymous vm_object */
#define PQ_SWAPBACKED			(PQ_ANON|PQ_AOBJ)

/* vm_object.h */
#define VM_OBJ_KERN				(-2)

/* vm_pager.h */
#define PGO_ALLPAGES			0x010	/* flush whole object/get all pages */
#define PGO_CLEANIT				0x001	/* write dirty pages to backing store */
#define PGO_SYNCIO				0x002	/* if PGO_CLEAN: use sync I/O? */
/*
 * obviously if neither PGO_INVALIDATE or PGO_FREE are set then the pages
 * stay where they are.
 */
#define PGO_DEACTIVATE			0x004	/* deactivate flushed pages */
#define PGO_FREE				0x008	/* free flushed pages */

#define PGO_ALLPAGES			0x010	/* flush whole object/get all pages */
#define PGO_DOACTCLUST			0x020	/* flag to mk_pcluster to include active */
#define PGO_LOCKED				0x040	/* fault data structures are locked [get] */
#define PGO_PDFREECLUST			0x080	/* daemon's free cluster flag [vm_pager_put] */
#define PGO_REALLOCSWAP			0x100	/* reallocate swap area [pager_dropcluster] */

/* page we are not interested in getting */
#define PGO_DONTCARE 			((struct vm_page *) -1)	/* [get only] */

/* vm_map.h */
/*
 * vm_map_entry etype bits:
 */
#define VM_ET_OBJ				0x01	/* it is a vm_object */
#define VM_ET_SUBMAP			0x02	/* it is a vm_map submap */
#define VM_ET_COPYONWRITE 		0x04	/* copy_on_write */
#define VM_ET_NEEDSCOPY			0x08	/* needs_copy */

#define VM_ET_ISOBJ(E)			(((E)->etype & VM_ET_OBJ) != 0)
#define VM_ET_ISSUBMAP(E)		(((E)->etype & VM_ET_SUBMAP) != 0)
#define VM_ET_ISCOPYONWRITE(E)	(((E)->etype & VM_ET_COPYONWRITE) != 0)
#define VM_ET_ISNEEDSCOPY(E)	(((E)->etype & VM_ET_NEEDSCOPY) != 0)

/* vm_extern.h */
/* vm_aobject.c */
struct vm_object				*uao_create (vm_size_t, int);
void							uao_detach (struct vm_object *);
void							uao_reference (struct vm_object *);

#endif /* SYS_DEVEL_VM_UVM_UVM_H_ */
