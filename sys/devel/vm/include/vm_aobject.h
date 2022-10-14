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

/*
 * for hash tables, we break the address space of the aobj into blocks
 * of VAO_SWHASH_CLUSTER_SIZE pages.   we require the cluster size to
 * be a power of two.
 */

#define VAO_SWHASH_CLUSTER_SHIFT 4
#define VAO_SWHASH_CLUSTER_SIZE (1 << VAO_SWHASH_CLUSTER_SHIFT)

/* get the "tag" for this page index */
#define VAO_SWHASH_ELT_TAG(PAGEIDX) 							\
	((PAGEIDX) >> VAO_SWHASH_CLUSTER_SHIFT)

/* given an ELT and a page index, find the swap slot */
#define VAO_SWHASH_ELT_PAGESLOT(ELT, PAGEIDX) 					\
	((ELT)->slots[(PAGEIDX) & (VAO_SWHASH_CLUSTER_SIZE - 1)])

/* given an ELT, return its pageidx base */
#define VAO_SWHASH_ELT_PAGEIDX_BASE(ELT) 						\
	((ELT)->tag << VAO_SWHASH_CLUSTER_SHIFT)

/*
 * the swhash hash function
 */
#define VAO_SWHASH_HASH(AOBJ, PAGEIDX) 							\
	(&(AOBJ)->u_swhash[(((PAGEIDX) >> VAO_SWHASH_CLUSTER_SHIFT) \
			& (AOBJ)->u_swhashmask)])

/*
 * the swhash threshhold determines if we will use an array or a
 * hash table to store the list of allocated swap blocks.
 */

#define VAO_SWHASH_THRESHOLD (VAO_SWHASH_CLUSTER_SIZE * 4)
#define VAO_USES_SWHASH(AOBJ) 									\
	((AOBJ)->u_pages > VAO_SWHASH_THRESHOLD)	/* use hash? */

/*
 * the number of buckets in a swhash, with an upper bound
 */
#define VAO_SWHASH_MAXBUCKETS 256
#define VAO_SWHASH_BUCKETS(AOBJ) 								\
	(min((AOBJ)->u_pages >> VAO_SWHASH_CLUSTER_SHIFT, 			\
			VAO_SWHASH_MAXBUCKETS))

/*
 * uao_list: global list of active aobjects, locked by uao_list_lock
 */
struct vm_aobject {
	struct vm_object 			u_obj; 			/* has: lock, pgops, memq, #pages, #refs */
	int 						u_pages;		/* number of pages in entire segment */
	int 						u_segments;		/* number of segments in entire object */
	int 						u_flags;		/* the flags (see uvm_aobj.h) */


	int							u_ref_count;
	int 						*u_swslots;		/* array of offset->swapslot mappings */
					 	 	 	 	 			/*
					 	 	 	 	 			 * hashtable of offset->swapslot mappings
					 	 	 	 	 			 * (u_swhash is an array of bucket heads)
					 	 	 	 	 			 */
	struct aobjectswhash		*u_swhash;
	u_long 						u_swhashmask;	/* mask for hashtable */
	LIST_ENTRY(vm_aobject) 		u_list;			/* global list of aobjs */
};

/*
 * uao_swhash_elt: when a hash table is being used, this structure defines
 * the format of an entry in the bucket list.
 */
/*
 * uao_swhash: the swap hash table structure
 */
struct vao_swhash_elt {
	LIST_ENTRY(vao_swhash_elt) 	list;							/* the hash list */
	vm_offset_t 				tag;							/* our 'tag' */
	int 						count;							/* our number of active slots */
	int 						slots[VAO_SWHASH_CLUSTER_SIZE];	/* the slots */
};

struct aobjectswhash;
LIST_HEAD(aobjectswhash, vao_swhash_elt);
struct aobjectlist;
LIST_HEAD(aobjectlist, vm_aobject);

struct aobjectswhash			*aobjectswhash;			/* aobject's swhash list */
struct aobjectlist 				aobject_list;			/* list of aobject's */
static simple_lock_data_t 		aobject_list_lock; 		/* lock for aobject lists */

/*
 * flags
 */
/* flags for uao_create: can only be used one time (at bootup) */
#define VAO_FLAG_KERNOBJ		0x1	/* create kernel object */
#define VAO_FLAG_KERNSWAP		0x2	/* enable kernel swap */

/* internal flags */
#define VAO_FLAG_KILLME			0x4	/* aobj should die when last released page is no longer PG_BUSY ... */
#define VAO_FLAG_NOSWAP			0x8	/* aobj can't swap (kernel obj only!) */

#ifdef _KERNEL
void 							vm_aobject_init(vm_size_t, vm_object_t, int);
void							vm_aobject_swhash_allocate(vm_aobject_t, int, int);
int 							vm_aobject_set_swslot(vm_object_t, int, int);
void							vm_aobject_dropswap(vm_object_t, int);
bool_t							vm_aobject_swap_off(int, int);
#endif /* _KERNEL */
#endif /* _VM_AOBJECT_H_ */
