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
#define ku_freecnt 		ku_un.freecnt
#define ku_pagecnt 		ku_un.pagecnt

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
	int                 		ksm_bslots;     /* bucket slots available */
	int				ksm_aslots;	/* bucket slots to be allocated */
	int                 		ksm_fslots;     /* bucket slots free */
	u_long 				ksm_bsize; 	/* bucket size */
	u_long 				ksm_bindx;	/* bucket index */
	u_long 				ksm_min;	/* bucket minimum boundary */
	u_long 				ksm_max;	/* bucket maximum boundary */
};

/* A Slab for each set of buckets */
struct kmemslabs {
	CIRCLEQ_ENTRY(kmemslabs) ksl_list;
    struct kmembuckets			*ksl_bucket;	/* slab kmembucket */
    struct kmemmeta     		ksl_meta;       /* slab metadata */

    u_long              		ksl_size;	/* slab size */
    int					ksl_mtype;      /* malloc type */
    int                 		ksl_stype;      /* slab type: see below */

    int					ksl_flags;	/* slab flags */
    int                 		ksl_usecount;   /* usage counter for slab caching */
};

/* WIP */
/* Slab Magazine */
struct kmemslabs_magazine {
    CIRCLEQ_ENTRY(kmemslabs_magazine) ksm_empty;    	/* slab empty list */
    CIRCLEQ_ENTRY(kmemslabs_magazine) ksm_partial;  	/* slab partial list */
    CIRCLEQ_ENTRY(kmemslabs_magazine) ksm_full;     	/* slab full list */
    int 				ksm_stype;      /* slab type: see below */
    int 				ksm_bslots;     /* bucket slots available counter */
    int 				ksm_aslots;     /* bucket slots allocated counter */
    int 				ksm_fslots;     /* bucket slots free counter */
    int 				ksm_refcount;   /* number of magazines counter */
};

/* Slab Cache */
struct kmemslabs_cache {
	CIRCLEQ_HEAD(, kmemslabs) 	ksc_slablist;	/* slab list head */
	CIRCLEQ_HEAD(, kmemslabs_magazine) ksc_maglist;	/* magazine list head */
	struct kmemslabs 		ksc_slab;	/* slab back-pointer */
	struct kmemslabs_magazine 	ksc_magazine;	/* magazine back-pointer */
	int                		ksc_refcount;	/* number of slabs counter */
};

/* slab flags */
#define SLAB_FULL               0x01        	/* slab full */
#define SLAB_PARTIAL            0x02        	/* slab partially full */
#define SLAB_EMPTY              0x04        	/* slab empty */

/* slab object types */
#define SLAB_SMALL              0x08        	/* slab contains small objects */
#define SLAB_LARGE              0x16        	/* slab contains large objects */

#ifdef _KERNEL
#define BUCKETSIZE(indx)	(powertwo(indx))
#define	MINALLOCSIZE		(1 << MINBUCKET)
#define BUCKETINDX(size) 							\
	((size) <= (MINALLOCSIZE * 128) 				\
		? (size) <= (MINALLOCSIZE * 8) 				\
			? (size) <= (MINALLOCSIZE * 2) 			\
				? (size) <= (MINALLOCSIZE * 1) 		\
					? (MINBUCKET + 0) 				\
					: (MINBUCKET + 1) 				\
				: (size) <= (MINALLOCSIZE * 4) 		\
					? (MINBUCKET + 2) 				\
					: (MINBUCKET + 3) 				\
			: (size) <= (MINALLOCSIZE* 32) 			\
				? (size) <= (MINALLOCSIZE * 16) 	\
					? (MINBUCKET + 4) 				\
					: (MINBUCKET + 5) 				\
				: (size) <= (MINALLOCSIZE * 64) 	\
					? (MINBUCKET + 6) 				\
					: (MINBUCKET + 7) 				\
		: (size) <= (MINALLOCSIZE * 2048) 			\
			? (size) <= (MINALLOCSIZE * 512) 		\
				? (size) <= (MINALLOCSIZE * 256) 	\
					? (MINBUCKET + 8) 				\
					: (MINBUCKET + 9) 				\
				: (size) <= (MINALLOCSIZE * 1024) 	\
					? (MINBUCKET + 10) 				\
					: (MINBUCKET + 11) 				\
			: (size) <= (MINALLOCSIZE * 8192) 		\
				? (size) <= (MINALLOCSIZE * 4096) 	\
					? (MINBUCKET + 12) 				\
					: (MINBUCKET + 13) 				\
				: (size) <= (MINALLOCSIZE * 16384) 	\
					? (MINBUCKET + 14) 				\
					: (MINBUCKET + 15))

//#define kmemxtob(alloc)	(kmembase + (alloc) * NBPG)
//#define btokmemx(addr)	(((caddr_t)(addr) - kmembase) / NBPG)
#define btokup(addr)	(&kmemusage[((caddr_t)(addr) - kmembase) >> CLSHIFT])

/* slab object macros */
#define SMALL_OBJECT(s)        	(BUCKETINDX((s)) < 10)
#define LARGE_OBJECT(s)        	(BUCKETINDX((s)) >= 10)

/* slot macros */
#define BUCKET_SLOTS(bsize)     ((bsize)/BUCKETINDX(bsize))       				/* Number of slots in a bucket */
#define ALLOCATED_SLOTS(size)	(BUCKET_SLOTS(size)/BUCKETINDX(size))			/* Number slots taken by space to be allocated */
#define SLOTSFREE(bsize, size)  (BUCKET_SLOTS(bsize) - ALLOCATED_SLOTS(size)) 	/* free slots in bucket */

#if defined(KMEMSTATS) || defined(DIAGNOSTIC)
#define	MALLOC(space, cast, size, type, flags) 						\
	(space) = (cast)malloc((u_long)(size), type, flags)
#define FREE(addr, type) 			free((caddr_t)(addr), (type))

#ifdef OVERLAY
#define OVERLAY_MALLOC(space, cast, size, type, flags)				\
	(space) = (cast)overlay_malloc((u_long)(size), type, flags)
#define OVERLAY_FREE(addr, type)	overlay_free((caddr_t)(addr), (type))
#endif

#else /* do not collect statistics */
#define	MALLOC(space, cast, size, type, flags) { 					\
	register struct kmembuckets *kbp = &slabbucket[BUCKETINDX(size)].ksl_bucket; 	\
	long s = splimp(); 												\
	if (kbp->kb_next == NULL) { 									\
		(space) = (cast)malloc((u_long)(size), type, flags); 		\
	} else { 														\
		(space) = (cast)kbp->kb_next; 								\
		kbp->kb_next = *(caddr_t *)(space); 						\
	} 																\
	splx(s); 														\
}

#define FREE(addr, type) { 											\
	register struct kmembuckets *kbp; 								\
	register struct kmemusage *kup = btokup(addr); 					\
	long s = splimp(); 												\
	if (1 << kup->ku_indx > MAXALLOCSAVE) { 						\
		free((caddr_t)(addr), type); 								\
	} else { 														\
		kbp = &slabbucket[kup->ku_indx].ksl_bucket; 				\
		if (kbp->kb_next == NULL) 									\
			kbp->kb_next = (caddr_t)(addr); 						\
		else 														\
			*(caddr_t *)(kbp->kb_last) = (caddr_t)(addr); 			\
		*(caddr_t *)(addr) = NULL; 									\
		kbp->kb_last = (caddr_t)(addr); 							\
	} 																\
	splx(s); 														\
}

#endif /* do not collect statistics */

extern struct kmemslabs_cache   *slabcache;
extern struct kmemslabs			slabbucket[];
extern struct kmemusage 		*kmemusage;
extern char 					*kmembase;

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
#endif /* KERNEL */

#endif /* !_SYS_MALLOC_H_ */
