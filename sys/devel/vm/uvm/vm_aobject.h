/*	$NetBSD: uvm_aobj.h,v 1.8 1999/03/26 17:34:15 chs Exp $	*/

/*
 * Copyright (c) 1998 Chuck Silvers, Charles D. Cranor and
 *                    Washington University.
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
 * from: Id: uvm_aobj.h,v 1.1.2.4 1998/02/06 05:19:28 chs Exp
 */
/*
 * uvm_aobj.h: anonymous memory uvm_object pager
 *
 * author: Chuck Silvers <chuq@chuq.com>
 * started: Jan-1998
 *
 * - design mostly from Chuck Cranor
 */

#ifndef _VM_AOBJECT_H_
#define _VM_AOBJECT_H_

struct vm_aobject {
	struct vm_object 		u_obj; 			/* has: lock, pgops, memq, #pages, #refs */
	int 					u_pages;		/* number of pages in entire object */
	int 					u_flags;		/* the flags (see uvm_aobj.h) */
	int 					*u_swslots;		/* array of offset->swapslot mappings */
					 	 	 	 	 		/*
					 	 	 	 	 		 * hashtable of offset->swapslot mappings
					 	 	 	 	 		 * (u_swhash is an array of bucket heads)
					 	 	 	 	 		 */
	struct uao_swhash 		*u_swhash;
	u_long 					u_swhashmask;	/* mask for hashtable */
	LIST_ENTRY(vm_aobject) 	u_list;			/* global list of aobjs */
};

/*
 * flags
 */

/* flags for uao_create: can only be used one time (at bootup) */
#define UAO_FLAG_KERNOBJ	0x1	/* create kernel object */
#define UAO_FLAG_KERNSWAP	0x2	/* enable kernel swap */

/* internal flags */
#define UAO_FLAG_KILLME		0x4	/* aobj should die when last released page is no longer PG_BUSY ... */
#define UAO_FLAG_NOSWAP		0x8	/* aobj can't swap (kernel obj only!) */

/*
 * prototypes
 */

int uao_set_swslot (struct vm_object *, int, int);
void uao_dropswap (struct vm_object *, int);

/*
 * globals
 */

extern struct pagerops aobject_pager;

/*
 * local functions
 */

static void			 			uao_init (void);
static struct uao_swhash_elt	*uao_find_swhash_elt (struct vm_aobject *, int, boolean_t);
static int			 			uao_find_swslot (struct vm_aobject *, int);
static boolean_t		 		uao_flush (struct vm_object *, vaddr_t, vaddr_t, int);
static void			 			uao_free (struct vm_aobject *);
static int			 			uao_get (struct vm_object *, vaddr_t, vm_page_t *, int *, int, vm_prot_t, int, int);
static boolean_t		 		uao_releasepg (struct vm_page *, struct vm_page **);

#endif /* _VM_AOBJECT_H_ */
