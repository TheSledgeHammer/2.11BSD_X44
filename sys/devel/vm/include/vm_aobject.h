/*	$NetBSD: uvm_aobj.h,v 1.23 2013/10/18 17:48:44 christos Exp $	*/

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

#ifndef _UVM_UVM_AOBJ_H_
#define _UVM_UVM_AOBJ_H_


/*
 * An anonymous UVM object (aobj) manages anonymous-memory.  In addition to
 * keeping the list of resident pages, it may also keep a list of allocated
 * swap blocks.  Depending on the size of the object, this list is either
 * stored in an array (small objects) or in a hash table (large objects).
 *
 * Lock order
 *
 *	uao_list_lock ->
 *		uvm_object::vmobjlock
 */

/*
 * Note: for hash tables, we break the address space of the aobj into blocks
 * of UAO_SWHASH_CLUSTER_SIZE pages, which shall be a power of two.
 */

#define	UAO_SWHASH_CLUSTER_SHIFT	4
#define	UAO_SWHASH_CLUSTER_SIZE		(1 << UAO_SWHASH_CLUSTER_SHIFT)

/* Get the "tag" for this page index. */
#define	UAO_SWHASH_ELT_TAG(idx)		((idx) >> UAO_SWHASH_CLUSTER_SHIFT)
#define UAO_SWHASH_ELT_PAGESLOT_IDX(idx) \
    ((idx) & (UAO_SWHASH_CLUSTER_SIZE - 1))

/* Given an ELT and a page index, find the swap slot. */
#define	UAO_SWHASH_ELT_PAGESLOT(elt, idx) \
    ((elt)->slots[UAO_SWHASH_ELT_PAGESLOT_IDX(idx)])

/* Given an ELT, return its pageidx base. */
#define	UAO_SWHASH_ELT_PAGEIDX_BASE(ELT) \
    ((elt)->tag << UAO_SWHASH_CLUSTER_SHIFT)

/* The hash function. */
#define	UAO_SWHASH_HASH(aobj, idx) \
    (&(aobj)->u_swhash[(((idx) >> UAO_SWHASH_CLUSTER_SHIFT) \
    & (aobj)->u_swhashmask)])

/*
 * The threshold which determines whether we will use an array or a
 * hash table to store the list of allocated swap blocks.
 */
#define	UAO_SWHASH_THRESHOLD		(UAO_SWHASH_CLUSTER_SIZE * 4)
#define	UAO_USES_SWHASH(aobj) \
    ((aobj)->u_pages > UAO_SWHASH_THRESHOLD)

/* The number of buckets in a hash, with an upper bound. */
#define	UAO_SWHASH_MAXBUCKETS		256
#define	UAO_SWHASH_BUCKETS(aobj) \
    (MIN((aobj)->u_pages >> UAO_SWHASH_CLUSTER_SHIFT, UAO_SWHASH_MAXBUCKETS))

/*
 * uao_swhash_elt: when a hash table is being used, this structure defines
 * the format of an entry in the bucket list.
 */

struct uao_swhash_elt {
	LIST_ENTRY(uao_swhash_elt) 	list;							/* the hash list */
	vm_offset_t					tag;							/* our 'tag' */
	int 						count;							/* our number of active slots */
	int 						slots[UAO_SWHASH_CLUSTER_SIZE];	/* the slots */
};

/*
 * uvm_aobj: the actual anon-backed uvm_object
 *
 * => the uvm_object is at the top of the structure, this allows
 *   (struct uvm_aobj *) == (struct uvm_object *)
 * => only one of u_swslots and u_swhash is used in any given aobj
 */

struct uvm_aobject {
	struct vm_object 		u_obj; 			/* has: lock, pgops, #pages, #refs */
	//pgoff_t 				u_pages;	 	/* number of pages in entire object */
	int 					u_flags;		/* the flags (see uvm_aobj.h) */
	int 					*u_swslots;		/* array of offset->swapslot mappings */
				 	 	 	 	 	 		/*
				 	 	 	 	 	 	 	 * hashtable of offset->swapslot mappings
				 	 	 	 	 	 	 	 * (u_swhash is an array of bucket heads)
				 	 	 	 	 	 	 	 */
	struct uao_swhash 		*u_swhash;
	u_long 					u_swhashmask;	/* mask for hashtable */
	LIST_ENTRY(uvm_aobject) u_list;			/* global list of aobjs */
	int 					u_freelist;		/* freelist to allocate pages from */
};

static void	uao_free(struct uvm_aobject *);
static int	uao_get(struct vm_object *, voff_t, struct vm_page **, int *, int, vm_prot_t, int, int);
static int	uao_put(struct vm_object *, voff_t, voff_t, int);

#if defined(VMSWAP)
static struct uao_swhash_elt *uao_find_swhash_elt
    (struct uvm_aobj *, int, bool);

static boolean_t uao_pagein(struct uvm_aobject *, int, int);
static boolean_t uao_pagein_page(struct uvm_aobject *, int);
#endif /* defined(VMSWAP) */

static struct vm_page	*uao_pagealloc(struct vm_object *, voff_t, int);

/*
 * Flags for uao_create: UAO_FLAG_KERNOBJ and UAO_FLAG_KERNSWAP are
 * used only once, to initialise UVM.
 */
#define	UAO_FLAG_KERNOBJ	0x1	/* create kernel object */
#define	UAO_FLAG_KERNSWAP	0x2	/* enable kernel swap */
#define	UAO_FLAG_NOSWAP		0x8	/* aobj may not swap */

#ifdef _KERNEL
void		uao_init(void);
int			uao_set_swslot(struct vm_object *, int, int);

#if	defined(VMSWAP)
int			uao_find_swslot(struct vm_object *, int);
void		uao_dropswap(struct vm_object *, int);
boolean_t	uao_swap_off(int, int);
void		uao_dropswap_range(struct vm_object *, voff_t, voff_t);
#else
#define		uao_find_swslot(obj, off)	(__USE(off), 0)
#define		uao_dropswap(obj, off)			/* nothing */
#define		uao_dropswap_range(obj, lo, hi)	/* nothing */
#endif

#endif /* _KERNEL */

#endif /* _UVM_UVM_AOBJ_H_ */
