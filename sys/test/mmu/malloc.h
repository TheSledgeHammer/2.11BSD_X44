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

#define KMEMSTATS

#define MINBUCKET	    4		/* 4 => min allocation of 16 bytes */
#define MAXALLOCSAVE	(2 * CLBYTES)

/* flags to malloc */
#define	M_WAITOK	0x0000
#define	M_NOWAIT	0x0001

/* Two-bit Type field to distinguish between different splits of sized blocks */
#define TYPE_11     11      /* split from 2k sized block */
#define TYPE_01     01      /* left split from a 3 2k-3 block */
#define TYPE_10     10      /* right split from a 3 2k-3 block  */

/* Types of memory to be allocated */
#define	M_FREE		0		/* should be on free list */
#define M_RMAP		1		/* rminit */
#define M_RMALLOC	2		/* rmalloc */
#define M_RMFREE	3		/* rmfree */
#define	M_MBUF		4		/* mbuf */

#define	M_TEMP		74		/* misc temporary data buffers */
#define	M_LAST		75		/* Must be last type + 1 */

#define INITKMEMNAMES { 			\
	"free",		/* 0 M_FREE */ 		\
	"rmap",		/* 1 M_RMAP */		\
	"rmalloc",	/* 2 M_RMALLOC */	\
	"rmfree",	/* 3 M_RMFREE */	\
	"mbuf",		/* 4 M_MBUF */ 		\
	"temp",		/* 74 M_TEMP */ 	\
}

struct kmemstats {
	long	ks_inuse;		/* # of packets of this type currently in use */
	long	ks_calls;		/* total packets of this type ever allocated */
	long 	ks_memuse;		/* total memory held in bytes */
	u_short	ks_limblocks;	/* number of times blocked for hitting limit */
	u_short	ks_mapblocks;	/* number of times blocked for kernel map */
	long	ks_maxused;		/* maximum number ever used */
	long	ks_limit;		/* most that are allowed to exist */
	long	ks_size;		/* sizes of this thing that are allocated */
	long	ks_spare;
};

/* Array of descriptors that describe the contents of each page */
struct kmemusage {
	short ku_indx;		/* bucket index */
	union {
		u_short freecnt;/* for small allocations, free pieces in page */
		u_short pagecnt;/* for large allocations, pages alloced */
	} ku_un;
};
#define ku_freecnt ku_un.freecnt
#define ku_pagecnt ku_un.pagecnt


/* Set of buckets for each size of memory block that is retained */
struct kmembuckets {
    struct kmembuckets  *kb_front;
    struct kmembuckets  *kb_back;

	caddr_t 			kb_next;		/* list of free blocks */
	caddr_t 			kb_last;		/* last free block */

	long				kb_calls;		/* total calls to allocate this size */
	long				kb_total;		/* total number of blocks allocated */
	long				kb_totalfree;	/* # of free elements in this bucket */
	long				kb_elmpercl;	/* # of elements in this sized allocation */
	long				kb_highwat;		/* high water mark */
	long				kb_couldfree;	/* over high water mark and could free */
};

struct kmemtree_entry {
    struct kmembuckets    kte_head;
    struct kmembuckets    kte_tail;

#define kteb_next         kte_head.kb_next		/* list of free blocks */
#define kteb_last         kte_head.kb_last		/* last free block */
};

/* Tertiary Tree within each bucket, for each size of memory block that is retained */
struct kmemtree {
    struct kmemtree_entry 	kt_parent;         	/* parent tree_entry */
    struct kmemtree 		*kt_left;		    /* free blocks on left child */
    struct kmemtree 		*kt_middle;		    /* free blocks on middle child */
    struct kmemtree 		*kt_right;		    /* free blocks on right child */

    int 					kt_type;			/* Two-bit Type field for different block sizes */
    boolean_t				kt_space;			/* Determines if tertiary tree needs to be created/used */

    long 					kt_size;			/* size of memory */
    int 					kt_entries;			/* # of child nodes in tree */

    unsigned long 			kt_bucket_size;     /* bucketmap size in bytes */
    long 					kt_bucket_idx;      /* bucketmap index */
};

/* Maps each bucket created (used by tertiary search tree)*/
struct kmembucketmap {
    unsigned long 	bucket_size;
    long 			bucket_index;
};

//#ifdef KERNEL
#define	MINALLOCSIZE	(1 << MINBUCKET)
#define BUCKETINDX(size) \
	((size) <= (MINALLOCSIZE * 128) \
		? (size) <= (MINALLOCSIZE * 8) \
			? (size) <= (MINALLOCSIZE * 2) \
				? (size) <= (MINALLOCSIZE * 1) \
					? (MINBUCKET + 0) \
					: (MINBUCKET + 1) \
				: (size) <= (MINALLOCSIZE * 4) \
					? (MINBUCKET + 2) \
					: (MINBUCKET + 3) \
			: (size) <= (MINALLOCSIZE* 32) \
				? (size) <= (MINALLOCSIZE * 16) \
					? (MINBUCKET + 4) \
					: (MINBUCKET + 5) \
				: (size) <= (MINALLOCSIZE * 64) \
					? (MINBUCKET + 6) \
					: (MINBUCKET + 7) \
		: (size) <= (MINALLOCSIZE * 2048) \
			? (size) <= (MINALLOCSIZE * 512) \
				? (size) <= (MINALLOCSIZE * 256) \
					? (MINBUCKET + 8) \
					: (MINBUCKET + 9) \
				: (size) <= (MINALLOCSIZE * 1024) \
					? (MINBUCKET + 10) \
					: (MINBUCKET + 11) \
			: (size) <= (MINALLOCSIZE * 8192) \
				? (size) <= (MINALLOCSIZE * 4096) \
					? (MINBUCKET + 12) \
					: (MINBUCKET + 13) \
				: (size) <= (MINALLOCSIZE * 16384) \
					? (MINBUCKET + 14) \
					: (MINBUCKET + 15))


struct kmembucketmap bucketmap[] = {
        { 16, 	 BUCKETINDX(16) },
        { 32, 	 BUCKETINDX(32) },
        { 64, 	 BUCKETINDX(64) },
        { 128, 	 BUCKETINDX(128) },
        { 256, 	 BUCKETINDX(256) },
        { 512, 	 BUCKETINDX(512) },
        { 1024,  BUCKETINDX(1024) },
        { 2048,  BUCKETINDX(2048) },
        { 4096,  BUCKETINDX(4096) },
        { 8192,  BUCKETINDX(8192) },
        { 16384, BUCKETINDX(16384) },
        { 16385, BUCKETINDX(16385) },
};

#define SplitLeft(n)    (n / 2)
#define SplitMiddle(n)  (((SplitLeft(n)) * 2) / 3)
#define SplitRight(n)   (((SplitMiddle(n) / 2)) + (((SplitLeft(n) + SplitMiddle(n) + n) % 2) + 1))
#define SplitTotal(n)   (SplitLeft(n) + SplitMiddle(n) + SplitRight(n))

#define kmemxtob(alloc)	(kmembase + (alloc) * NBPG)
#define btokmemx(addr)	(((char)(addr) - kmembase) / NBPG)
#define btokup(addr)	(&kmemusage[((char)(addr) - kmembase) >> CLSHIFT])

#if defined(KMEMSTATS) || defined(DIAGNOSTIC)
#define	MALLOC(space, cast, size, type, flags) 						\
	(space) = (cast)malloc((u_long)(size), type, flags)
#define FREE(addr, type) free((caddr_t)(addr), type)

#else /* do not collect statistics */
#define	MALLOC(space, cast, size, type, flags) { 					\
	register struct kmembuckets *kbp = &bucket[BUCKETINDX(size)]; 	\
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
		kbp = &bucket[kup->ku_indx]; 								\
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

extern struct kmemtree_entry tree_bucket_entry[];
extern struct kmembuckets bucket[];
extern struct kmemusage *kmemusage;
extern char *kmembase;
extern void *malloc __P((unsigned long size, int type, int flags));
extern void free __P((void *addr, int type));
//#endif /* KERNEL */
#endif /* !_SYS_MALLOC_H_ */
