/*
 * Copyright (c) 1987 Carnegie-Mellon University
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

#ifndef SYS_DEVEL_ARCH_I386_PTE_H_
#define SYS_DEVEL_ARCH_I386_PTE_H_

/* param.h */
#define NBSEG			0x400000	/* bytes/segment */
#define	SEGOFSET		(NBSEG-1)	/* byte offset into segment */
#define	SEGSHIFT		22			/* LOG2(NBSEG) */


/* pte.h */
/* segment table */
struct ste {
	unsigned int
	sg_v:2,			/* valid bits */
	sg_prot:1,		/* write protect bit */
	sg_u:1,			/* hardware modified (dirty) bit */
	:8,				/* reserved at 0 */
	:1,				/* reserved at 1 */
	sg_pfnum:20;	/* page table frame number */
};


typedef struct ste st_entry_t;	/* segment table entry */

#define	ST_ENTRY_NULL	((st_entry_t *) 0)

#define	SG_V		0x00000002	/* segment is valid */
#define	SG_NV		0x00000000
#define	SG_PROT		0x00000004	/* access protection mask */
#define	SG_RO		0x00000004
#define	SG_RW		0x00000000
#define	SG_U		0x00000008	/* modified bit (68040) */
#define	SG_FRAME	0xfffff000
#define	SG_IMASK	0xffc00000
#define	SG_ISHIFT	22
#define	SG_PMASK	0x003ff000
#define	SG_PSHIFT	12

/* pmap.h */
#define I386_PAGE_SIZE		NBPG
#define I386_SEG_SIZE		NBSEG

#define i386_trunc_seg(x)	(((unsigned)(x)) & ~(I386_SEG_SIZE-1))
#define i386_round_seg(x)	i386_trunc_seg((unsigned)(x) + I386_SEG_SIZE-1)

/*
 * Pmap stuff
 */
struct pmap {
	struct pte				*pm_ptab;		/* KVA of page table */
	struct ste				*pm_stab;		/* KVA of segment table */
	int						pm_stchanged;	/* ST changed */
	int						pm_stfree;		/* 040: free lev2 blocks */
	struct ste				*pm_stpa;		/* 040: ST phys addr */
	short					pm_sref;		/* segment table ref count */
	short					pm_count;		/* pmap reference count */
	simple_lock_data_t		pm_lock;		/* lock on pmap */
	struct pmap_statistics	pm_stats;		/* pmap statistics */
	long					pm_ptpages;		/* more stats: PT pages */
};

typedef struct pmap	*pmap_t;

/*
 * On the 040 we keep track of which level 2 blocks are already in use
 * with the pm_stfree mask.  Bits are arranged from LSB (block 0) to MSB
 * (block 31).  For convenience, the level 1 table is considered to be
 * block 0.
 *
 * MAX[KU]L2SIZE control how many pages of level 2 descriptors are allowed.
 * for the kernel and users.  8 implies only the initial "segment table"
 * page is used.  WARNING: don't change MAXUL2SIZE unless you can allocate
 * physically contiguous pages for the ST in pmap.c!
 */
#define	MAXKL2SIZE	32
#define MAXUL2SIZE	8
#define l2tobm(n)	(1 << (n))
#define	bmtol2(n)	(ffs(n) - 1)

typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	struct pmap		*pv_pmap;	/* pmap where mapping lies */
	vm_offset_t		pv_va;		/* virtual address for mapping */
	struct ste		*pv_ptste;	/* non-zero if VA maps a PT page */
	struct pmap		*pv_ptpmap;	/* if pv_ptste, pmap for PT page */
	int				pv_flags;	/* flags */
} *pv_entry_t;

#endif /* SYS_DEVEL_ARCH_I386_PTE_H_ */
