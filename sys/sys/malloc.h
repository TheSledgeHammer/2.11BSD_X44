/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)malloc.h	8.5 (Berkeley) 5/3/95
 */

#ifndef _SYS_MALLOC_H_
#define	_SYS_MALLOC_H_

#include <sys/queue.h>

#ifndef KMEMSTATS
#define KMEMSTATS
#endif

#include <sys/malloctypes.h>

struct kmemstats {
	long				ks_inuse;	/* # of packets of this type currently in use */
	long				ks_calls;	/* total packets of this type ever allocated */
	long 				ks_memuse;	/* total memory held in bytes */
	u_short				ks_limblocks;	/* number of times blocked for hitting limit */
	u_short				ks_mapblocks;	/* number of times blocked for kernel map */
	long				ks_maxused;	/* maximum number ever used */
	long				ks_limit;	/* most that are allowed to exist */
	long				ks_size;	/* sizes of this thing that are allocated */
	long				ks_spare;
};

/* Array of descriptors that describe the contents of each page */
struct kmemusage {
	short 				ku_indx;	/* bucket index */
	union {
		u_short 		freecnt;	/* for small allocations, free pieces in page */
		u_short 		pagecnt;	/* for large allocations, pages alloced */
	} ku_un;
};
#define ku_freecnt 			ku_un.freecnt
#define ku_pagecnt 			ku_un.pagecnt

/* Set of buckets for each size of memory block that is retained */
struct kmembuckets {
	caddr_t 			kb_next;	/* list of free blocks */
	caddr_t 			kb_last;	/* last free block */

	long				kb_calls;	/* total calls to allocate this size */
	long				kb_total;	/* total number of blocks allocated */
	long				kb_totalfree;	/* # of free elements in this bucket */
	long				kb_elmpercl;	/* # of elements in this sized allocation */
	long				kb_highwat;	/* high water mark */
	long				kb_couldfree;	/* over high water mark and could free */
};

/* Per-Slab Metadata */
struct kmemmeta {
	LIST_ENTRY(kmemmeta) 		ksm_list;	/* list of slab metadata */
	struct kmemslabs 		*ksm_slab; 	/* what slab i am in */

	int 				ksm_bslots;  	/* bucket slots (per bucket) */
	int 				ksm_aslots;	/* allocated slots (per bucket) */
	int 				ksm_fslots;	/* slots free (per bucket) */
	u_long 				ksm_bsize;	/* bucket size  */
	u_long 				ksm_bindx;	/* bucket index */
	u_long 				ksm_min;	/* bucket minimum boundary */
	u_long 				ksm_max;	/* bucket maximum boundary */

	int 				ksm_tbslots;	/* total bucket slots */
	int 				ksm_taslots;	/* total allocated slots  */
	int 				ksm_tfslots;	/* total free slots  */
	int 				ksm_refcnt; 	/* meta refcnt */
};

/* A Slab for each set of buckets */
struct kmemslabs {
	CIRCLEQ_ENTRY(kmemslabs) 	ksl_empty;	/* slab empty list */
	CIRCLEQ_ENTRY(kmemslabs) 	ksl_partial;  	/* slab partial list */
	CIRCLEQ_ENTRY(kmemslabs) 	ksl_full;  	/* slab full list */
    	struct kmembuckets		*ksl_bucket;	/* slab kmembucket */
    	struct kmemmeta			*ksl_meta;	/* slab metadata */
    	u_long				ksl_size;	/* slab size */
    	int				ksl_mtype;	/* malloc type */
    	int				ksl_stype;	/* slab type: see below */
    	int				ksl_flags;	/* slab flags */
};

/* WIP */
/* Slab Magazine's */
struct kmemmagazine {
	LIST_ENTRY(kmemmagazine) 	ksm_list;	/* slab magazine list */
    	void 				*ksm_object;	/* magazine object */
	u_long				ksm_size;	/* magazine size */
	u_long				ksm_index;	/* magazine index */
	int 				ksm_flags;	/* magazine flags */
	int 				ksm_rounds; 	/* magazine rounds */
};

/* magazine flags */
#define MAGAZINE_FULL		0x10	/* magazine full ((i.e. space left) rounds is greater than 0) */
#define MAGAZINE_EMPTY		0x12	/* magazine empty ((i.e. no space left) rounds equals 0)*/

#define MAGAZINE_CAPACITY 	64      /* magazine capacity (number of rounds in each magazine) */
#define MAGAZINE_DEPOT_MAX	2 	/* number of active depots in use at once per cpu */

/* Slab Magazine Depot */
struct kmemmagazine_depot {
	LIST_HEAD(, kmemmagazine) 	ksmd_magazinelist_empty; /* magazine empty list */
	LIST_HEAD(, kmemmagazine) 	ksmd_magazinelist_full; /* magazine full list */
	struct kmemmagazine 		ksmd_magazine;	/* magazine back-pointer */
	int 				ksmd_capacity;	/* capacity per magazine */
	int 				ksmd_magcount; 	/* number of magazines counter */
};

/* Slab CPU Cache */
struct kmemcpu_cache {
	struct kmemmagazine 		*kscp_magazine_current;	/* cpu's currently loaded magazine */
	struct kmemmagazine 		*kscp_magazine_previous;/* cpu's previously loaded magazine */
	int 				kscp_rounds_current;	/* rounds left in current magazine */
	int 				kscp_rounds_previous;	/* rounds left in previous magazine */
};

/* Slab Cache Operations */
struct kmem_ops {
	void (*kmops_init)(struct kmemcache *, vm_size_t);
	void (*kmops_alloc)(struct kmemcache *, void *, u_long, u_long, int);
	void (*kmops_free)(struct kmemcache *, void *, u_long, u_long, int);
	void (*kmops_destroy)(struct kmemcache *, void *, u_long, u_long, int);
};

/* Slab Cache */
struct kmemcache {
	CIRCLEQ_HEAD(, kmemslabs) 	ksc_slablist_empty;	/* slab empty list */
	CIRCLEQ_HEAD(, kmemslabs) 	ksc_slablist_partial;	/* slab partial list */
	CIRCLEQ_HEAD(, kmemslabs) 	ksc_slablist_full;	/* slab full list */
	struct kmemslabs 		ksc_slab;		/* slab back-pointer */
	int 				ksc_slabcount; 		/* number of slabs counter */
	struct kmemmagazine_depot 	ksc_depot;		/* magazine depot */
	int				ksc_depotcount; 	/* number of magazine depot counter */
	struct kmem_ops 		*ksc_ops;		/* cache ops */
	struct kmemcpu_cache 		*ksc_percpu;		/* percpu cache */
};

/* slab flags */
#define SLAB_FULL               0x01        	/* slab full */
#define SLAB_PARTIAL            0x02        	/* slab partially full */
#define SLAB_EMPTY              0x04        	/* slab empty */

/* slab object types */
#define SLAB_SMALL              0x06        	/* slab contains small objects */
#define SLAB_LARGE              0x08        	/* slab contains large objects */

#ifdef _KERNEL
#define BUCKETSIZE(indx)	(powertwo(indx))
#define	MINALLOCSIZE		(1 << MINBUCKET)
#define BUCKETINDX(size) 							\
	((size) <= (MINALLOCSIZE * 128) 					\
		? (size) <= (MINALLOCSIZE * 8) 					\
			? (size) <= (MINALLOCSIZE * 2) 				\
				? (size) <= (MINALLOCSIZE * 1) 			\
					? (MINBUCKET + 0) 			\
					: (MINBUCKET + 1) 			\
				: (size) <= (MINALLOCSIZE * 4) 			\
					? (MINBUCKET + 2) 			\
					: (MINBUCKET + 3) 			\
			: (size) <= (MINALLOCSIZE* 32) 				\
				? (size) <= (MINALLOCSIZE * 16) 		\
					? (MINBUCKET + 4) 			\
					: (MINBUCKET + 5) 			\
				: (size) <= (MINALLOCSIZE * 64) 		\
					? (MINBUCKET + 6) 			\
					: (MINBUCKET + 7) 			\
		: (size) <= (MINALLOCSIZE * 2048) 				\
			? (size) <= (MINALLOCSIZE * 512) 			\
				? (size) <= (MINALLOCSIZE * 256) 		\
					? (MINBUCKET + 8) 			\
					: (MINBUCKET + 9) 			\
				: (size) <= (MINALLOCSIZE * 1024) 		\
					? (MINBUCKET + 10) 			\
					: (MINBUCKET + 11) 			\
			: (size) <= (MINALLOCSIZE * 8192) 			\
				? (size) <= (MINALLOCSIZE * 4096) 		\
					? (MINBUCKET + 12) 			\
					: (MINBUCKET + 13) 			\
				: (size) <= (MINALLOCSIZE * 16384) 		\
					? (MINBUCKET + 14) 			\
					: (MINBUCKET + 15))

//#define kmemxtob(alloc)	(kmembase + (alloc) * NBPG)
//#define btokmemx(addr)	(((caddr_t)(addr) - kmembase) / NBPG)
#define btokup(addr)	(&kmemusage[((caddr_t)(addr) - kmembase) >> CLSHIFT])

/* slab object macros */
#define SMALL_OBJECT(s)        	(BUCKETINDX((s)) < 10)
#define LARGE_OBJECT(s)        	(BUCKETINDX((s)) >= 10)

/* slot macros */
/* Number of slots in a bucket */
#define BUCKET_SLOTS(bsize, size)     		((bsize)/BUCKETINDX(size))
/* Number slots taken by space to be allocated */
#define BUCKET_SLOTS_ALLOCATED(bsize, size)	(BUCKET_SLOTS(bsize, size)/BUCKETINDX(size))
/* free slots in bucket */
#define BUCKET_SLOTS_FREE(bsize, size)  	(BUCKET_SLOTS(bsize, size) - BUCKET_SLOTS_ALLOCATED(bsize, size))

#if defined(KMEMSTATS) || defined(DIAGNOSTIC)
#define	MALLOC(space, cast, size, type, flags) 						\
	(space) = (cast)malloc((u_long)(size), type, flags)
#define FREE(addr, type) 			free((caddr_t)(addr), (type))

#ifdef OVERLAY
#define OVERLAY_MALLOC(space, cast, size, type, flags)					\
	(space) = (cast)overlay_malloc((u_long)(size), type, flags)
#define OVERLAY_FREE(addr, type)	overlay_free((caddr_t)(addr), (type))
#endif

#else /* do not collect statistics */

#define	MALLOC(space, cast, size, type, flags) { 					\
	register struct kmembuckets *kbp = slabbucket[BUCKETINDX(size)].ksl_bucket; 	\
	long s = splimp(); 								\
	if (kbp->kb_next == NULL) { 							\
		(space) = (cast)malloc((u_long)(size), type, flags); 			\
	} else { 									\
		(space) = (cast)kbp->kb_next; 						\
		kbp->kb_next = *(caddr_t *)(space); 					\
	} 										\
	splx(s); 									\
}

#define FREE(addr, type) { 								\
	register struct kmembuckets *kbp; 						\
	register struct kmemusage *kup = btokup(addr); 					\
	long s = splimp(); 								\
	if (1 << kup->ku_indx > MAXALLOCSAVE) { 					\
		free((caddr_t)(addr), type); 						\
	} else { 									\
		kbp = slabbucket[kup->ku_indx].ksl_bucket; 				\
		if (kbp->kb_next == NULL) 						\
			kbp->kb_next = (caddr_t)(addr); 				\
		else 									\
			*(caddr_t *)(kbp->kb_last) = (caddr_t)(addr); 			\
		*(caddr_t *)(addr) = NULL; 						\
		kbp->kb_last = (caddr_t)(addr); 					\
	} 										\
	splx(s); 									\
}

#endif /* do not collect statistics */

extern struct kmemcache slabcache;
extern struct kmembuckets bucket[];
extern struct kmemslabs	slabbucket[];
extern struct kmemmeta metabucket[];
extern struct kmemmagazine magazinebucket[];
extern struct kmemusage *kmemusage;
extern char *kmembase;
extern struct lock_object malloc_slock;

extern void *malloc(unsigned long, int, int);
extern void free(void *, int);
extern void *realloc(void *, unsigned long, int, int);
extern void *calloc(int, unsigned long, int, int);

#ifdef OVERLAY
extern void *overlay_malloc(unsigned long, int, int);
extern void overlay_free(void *, int);
extern void *overlay_realloc(void *, unsigned long, int, int);
extern void *overlay_calloc(int, unsigned long, int, int);
#endif

/* subr_slab.c */
void kmemslab_init(struct kmemcache *, vm_size_t);
struct kmemslabs *kmemslab_create(struct kmemcache *, u_long, u_long, int);
void kmemslab_destroy(struct kmemcache *, struct kmemslabs *, u_long, u_long, int);
void kmemslab_insert(struct kmemcache *, struct kmemslabs *, u_long, u_long, int);
void kmemslab_remove(struct kmemcache *, struct kmemslabs *, u_long, u_long, int);
struct kmemslabs *kmemslab_lookup(struct kmemcache *, u_long, u_long, int);
#endif /* KERNEL */

#endif /* !_SYS_MALLOC_H_ */
