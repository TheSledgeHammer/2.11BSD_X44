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

#ifndef SYS_MALLOC2_H_
#define SYS_MALLOC2_H_
#include <sys/malloc.h>
#include <sys/queue.h>

/**********************************************/
/* Planned (New): Yet to be Implemented: */
//#define M_DIRHASH	62	/* UFS dirhash */								/* UN-USED */
//#define M_WAPBL		65	/* UFS & VFS WAPBL */							/* UN-USED */
//#define M_TGRP		66	/* thread group header */						/* UN-USED */
//#define M_THREAD	67	/* thread structures */							/* UN-USED */

//#define M_KOVL		70	/* Kernel Overlay */
//#define M_VOVL		71	/* Virtual Overlay */
//#define M_VMSEG			72	/* VM Segmentation */

#define M_OVLMAP		/* OVL Map */
#define M_OVLOBJ		/* OVL Object */
#define M_AVMMAP		/* AVM Map (anonymous map: uvm_amap) */
#define M_AVMOBJ		/* AVM Object (anonymous object: uvm_aobj) */
#define M_CANFAIL
/**********************************************/

/* Two-bit Type field to distinguish between different splits of sized blocks */
#define TYPE_11     11      /* split from 2k sized block */
#define TYPE_01     01      /* left split from a 3 2k-3 block */
#define TYPE_10     10      /* right split from a 3 2k-3 block  */

struct kmembucket_entry {
	CIRCLEQ_ENTRY(kmembucket_entry) kbe_entry;			/* bucket entries */
	struct kmemtree          		*kbe_ztree;			/* Pointer kmemtree */
	unsigned long            		kbe_bsize;			/* bucket size */
	long                     		kbe_bindx;			/* bucket indx */
	boolean_t                		kbe_bspace;			/* bucket contains a tree: Default = False or 0 */

	/* Original kmembuckets: */
	caddr_t 						kbe_next;			/* list of free blocks */
	caddr_t 						kbe_last;			/* last free block */

	long							kbe_calls;			/* total calls to allocate this size */
	long							kbe_total;			/* total number of blocks allocated */
	long							kbe_totalfree;		/* # of free elements in this bucket */
	long							kbe_elmpercl;		/* # of elements in this sized allocation */
	long							kbe_highwat;		/* high water mark */
	long							kbe_couldfree;		/* over high water mark and could free */
};

CIRCLEQ_HEAD(kmembucket_clist, kmembucket_entry);
struct kmembuckets {
	struct kmembucket_clist 		kb_header;			/* Bucket List */
};

/* Tertiary Tree within each bucket, for each size of memory block that is retained */
struct kmemtree {
	struct kmembucket_entry     	*kt_kmemtable;
    struct kmemtree 				*kt_left;		    /* free blocks on left child */
    struct kmemtree 				*kt_middle;		    /* free blocks on middle child */
    struct kmemtree 				*kt_right;		    /* free blocks on right child */

    struct asl      				*kt_freelist1;      /* ptr to asl for 2k blocks */
    struct asl      				*kt_freelist2;      /* ptr to asl for 3(2k-3) blocks */

    int 							kt_type;			/* Two-bit Type field for different block sizes */
    boolean_t						kt_space;			/* Determines if tertiary tree needs to be created/used */

    long 							kt_size;			/* size of memory */
    int 							kt_entries;			/* # of child nodes in tree */

    caddr_t 						kt_va;				/* virtual address */

   unsigned long 					kt_bsize;			/* bucket size this tree belongs too */
   long 							kt_bindx;			/* bucket indx this tree belongs too */
};

/* Tertiary Tree: Available Space List */
struct asl {
    struct asl 						*asl_next;
    struct asl 						*asl_prev;
    unsigned long 					asl_size;
    //Total space allocated & free: Can provide a secondary validation to kmemstats
};


#define BUCKETSIZE(indx)	(isPowerOfTwo(indx))

#define SplitLeft(n)    	(n / 2)
#define SplitMiddle(n)  	(((SplitLeft(n)) * 2) / 3)
#define SplitRight(n)   	(((SplitMiddle(n) / 2)) + (((SplitLeft(n) + SplitMiddle(n) + n) % 2) + 1))
#define SplitTotal(n)   	(SplitLeft(n) + SplitMiddle(n) + SplitRight(n))

#define LOG2(n)         	(n >> 2)

/* All methods below are for internal use only for kern_malloc */
extern void 					kmembucket_setup(long indx);
extern struct kmembucket_entry	*kmembucket_allocate_head(struct kmembuckets *kbp, u_long size);
extern struct kmembucket_entry	*kmembucket_allocate_tail(struct kmembuckets *kbp, u_long size);
extern struct kmemtree 			*kmemtree_allocate(struct kmembuckets *kbp);
extern struct kmembucket_entry 	*kmembucket_search_next(struct kmembuckets *kbp, struct kmembucket_entry *kbe, caddr_t next);
extern struct kmembucket_entry 	*kmembucket_search_last(struct kmembuckets *kbp, struct kmembucket_entry *kbe, caddr_t last);

extern void 					kmemtree_trealloc(struct kmemtree *ktp, u_long size, int flags);
extern void 					kmemtree_trealloc_free(struct kmemtree *ktp, u_long size);

/* Tertiary Tree: Available Space List */
extern struct asl 				*asl_list(struct asl *free, u_long size);
extern struct asl 				*asl_insert(struct asl *free, u_long size);
extern struct asl 				*asl_remove(struct asl *free, u_long size);
extern struct asl 				*asl_search(struct asl *free, u_long size);

#endif /* SYS_MALLOC2_H_ */
