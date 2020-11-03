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

/**********************************************/
/* Planned (New): Yet to be Implemented: */

#define M_VMVOLGRP		/* VM Volume Group */

#define M_OVLMAP		/* OVL Map */
#define M_OVLOBJ		/* OVL Object */

#define M_AVMMAP		/* AVM Map (anonymous map: uvm_amap) */
#define M_AVMOBJ		/* AVM Object (anonymous object: uvm_aobj) */
/**********************************************/

#define KMEMSTATS

#define MINBUCKET	    4				/* 4 => min allocation of 16 bytes */
#define MAXALLOCSAVE	(2 * CLBYTES)

/* Tertiary Tree Two-bit Type Field: */
#define TYPE_11    	 	11      /* split from 2k sized block */
#define TYPE_01     	01      /* left split from a 3 2k-3 block */
#define TYPE_10     	10      /* right split from a 3 2k-3 block  */

/* flags to malloc */
#define	M_WAITOK		0x0000
#define	M_NOWAIT		0x0001
#define M_CANFAIL		0x0002
#define M_ZERO			0x0004

/* Types of memory to be allocated */
#define	M_FREE			0	/* should be on free list */
#define	M_LAST			71	/* Must be last type + 1 */

#define INITKMEMNAMES {						\
	"free",			/* 0 M_FREE */ 			\
	"temp",			/* 70 M_TEMP */ 		\
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
	short ku_indx;			/* bucket index */
	union {
		u_short freecnt;	/* for small allocations, free pieces in page */
		u_short pagecnt;	/* for large allocations, pages alloced */
	} ku_un;
};
#define ku_freecnt ku_un.freecnt
#define ku_pagecnt ku_un.pagecnt

/* Set of buckets for each size of memory block that is retained */
struct kmembuckets {
	struct kmemtree *kb_trbtree;		/* kmemtree */

	caddr_t 		kb_next;			/* list of free blocks */
	caddr_t 		kb_last;			/* last free block */

	long			kb_calls;			/* total calls to allocate this size */
	long			kb_total;			/* total number of blocks allocated */
	long			kb_totalfree;		/* # of free elements in this bucket */
	long			kb_elmpercl;		/* # of elements in this sized allocation */
	long			kb_highwat;			/* high water mark */
	long			kb_couldfree;		/* over high water mark and could free */
};

/* Tertiary Buddy Tree within each kmembucket allocated */
struct kmemtree {
    struct kmemtree *kt_left;		    /* free blocks on left child */
    struct kmemtree *kt_middle;		    /* free blocks on middle child */
    struct kmemtree *kt_right;		    /* free blocks on right child */

    struct asl      *kt_freelist1;      /* ptr to asl for 2k blocks */
    struct asl      *kt_freelist2;      /* ptr to asl for 3(2k-3) blocks */

    caddr_t 		kt_addr;			/* address */
    long 			kt_size;			/* size of memory */
    int 			kt_entries;			/* # of child nodes in tree */

    int 			kt_type;			/* Two-bit Type field for different block sizes */

    unsigned long 	kt_bsize;			/* bucket size this tree belongs too */
    long 			kt_bindx;			/* bucket indx this tree belongs too */
};

/* Available Space List */
struct asl {
	struct asl 		*asl_next;
	struct asl 		*asl_prev;
	unsigned long  	asl_size;
	caddr_t 		asl_addr;

	long			asl_spare0;
	long			asl_spare1;
	short			asl_type;
};

#ifdef KERNEL
#define	MINALLOCSIZE		(1 << MINBUCKET)
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

#define BUCKETSIZE(indx) 	(isPowerOfTwo(indx))

#define kmemxtob(alloc)		(kmembase + (alloc) * NBPG)
#define btokmemx(addr)		(((char)(addr) - kmembase) / NBPG)
#define btokup(addr)		(&kmemusage[((char)(addr) - kmembase) >> CLSHIFT])

#define SplitLeft(n)    	(n / 2)
#define SplitMiddle(n)  	(((SplitLeft(n)) * 2) / 3)
#define SplitRight(n)   	(((SplitMiddle(n) / 2)) + (((SplitLeft(n) + SplitMiddle(n) + n) % 2) + 1))
#define SplitTotal(n)   	(SplitLeft(n) + SplitMiddle(n) + SplitRight(n))

#define LOG2(n)         	(n >> 2)

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

extern struct kmembuckets bucket[];
extern struct kmemusage *kmemusage;
extern char *kmembase;

extern void *malloc (unsigned long size, int type, int flags);
extern void free (void *addr, int type);

#endif /* KERNEL */
#endif /* SYS_MALLOC2_H_ */
