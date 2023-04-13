/*	$NetBSD: pmap.h,v 1.1 2003/04/26 18:39:46 fvdl Exp $	*/

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
 *    must display the following acknowledgment:
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
 */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMD64_PMAP_H_
#define _AMD64_PMAP_H_

#define L1_SHIFT	12
#define	L2_SHIFT	21
#define	L3_SHIFT	30
#define	L4_SHIFT	39

#define	NBPD_L1		(1ULL << L1_SHIFT) 		/* # bytes mapped by L1 ent (4K) */
#define	NBPD_L2		(1ULL << L2_SHIFT) 		/* # bytes mapped by L2 ent (2MB) */
#define	NBPD_L3		(1ULL << L3_SHIFT) 		/* # bytes mapped by L3 ent (1G) */
#define	NBPD_L4		(1ULL << L4_SHIFT) 		/* # bytes mapped by L4 ent (512G) */

#define L4_MASK		0x0000ff8000000000
#define L3_MASK		0x0000007fc0000000
#define L2_MASK		0x000000003fe00000
#define L1_MASK		0x00000000001ff000

#define L4_FRAME	L4_MASK
#define L3_FRAME	(L4_FRAME|L3_MASK)
#define L2_FRAME	(L3_FRAME|L2_MASK)
#define L1_FRAME	(L2_FRAME|L1_MASK)

typedef uint64_t 	pt_entry_t;				/* PTE (L1) */
typedef uint64_t 	pd_entry_t;				/* PDE (L2) */
typedef uint64_t 	pdp_entry_t;			/* PDP (L3) */
typedef uint64_t 	pml4_entry_t;			/* L4 */

/*
 * Mask to get rid of the sign-extended part of addresses.
 */
#define VA_SIGN_MASK		0xffff000000000000
#define VA_SIGN_NEG(va)		((va) | VA_SIGN_MASK)

#define VA_SIGN_POS(va)		((va) & ~VA_SIGN_MASK)

#define L4_SLOT_PTE			255
#define L4_SLOT_APTE		510
#define L4_SLOT_KERN		256
#define L4_SLOT_KERNBASE	511

#define PDIR_SLOT_KERN		L4_SLOT_KERN
#define PDIR_SLOT_PTE		L4_SLOT_PTE
#define PDIR_SLOT_APTE		L4_SLOT_APTE

/*
 * the following defines give the virtual addresses of various MMU
 * data structures:
 * PTE_BASE and APTE_BASE: the base VA of the linear PTE mappings
 * PTD_BASE and APTD_BASE: the base VA of the recursive mapping of the PTD
 * PDP_PDE and APDP_PDE: the VA of the PDE that points back to the PDP/APDP
 *
 */
#define L1_BASE			((pt_entry_t *)(L4_SLOT_PTE * NBPD_L4))
#define L2_BASE 		((pd_entry_t *)((char *)L1_BASE + L4_SLOT_PTE * NBPD_L3))
#define L3_BASE 		((pdp_entry_t *)((char *)L2_BASE + L4_SLOT_PTE * NBPD_L2))
#define L4_BASE 		((pml4_entry_t *)((char *)L3_BASE + L4_SLOT_PTE * NBPD_L1))

#define AL1_BASE		((pt_entry_t *) (VA_SIGN_NEG((L4_SLOT_APTE * NBPD_L4))))
#define AL2_BASE 		((pd_entry_t *)((char *)AL1_BASE + L4_SLOT_PTE * NBPD_L3))
#define AL3_BASE 		((pdp_entry_t *)((char *)AL2_BASE + L4_SLOT_PTE * NBPD_L2))
#define AL4_BASE 		((pml4_entry_t *)((char *)AL3_BASE + L4_SLOT_PTE * NBPD_L1))

/*
 * PL*_1: generate index into pde/pte arrays in virtual space
 */
#define PL1_I(VA)		(((VA_SIGN_POS(VA)) & L1_FRAME) >> L1_SHIFT)
#define PL2_I(VA)		(((VA_SIGN_POS(VA)) & L2_FRAME) >> L2_SHIFT)
#define PL3_I(VA)		(((VA_SIGN_POS(VA)) & L3_FRAME) >> L3_SHIFT)
#define PL4_I(VA)		(((VA_SIGN_POS(VA)) & L4_FRAME) >> L4_SHIFT)
#define PL_I(va, lvl) 	(((VA_SIGN_POS(va)) & ptp_masks[(lvl)-1]) >> ptp_shifts[(lvl)-1])

#define PTP_MASK_INITIALIZER	{ L1_FRAME, L2_FRAME, L3_FRAME, L4_FRAME }
#define PTP_SHIFT_INITIALIZER	{ L1_SHIFT, L2_SHIFT, L3_SHIFT, L4_SHIFT }
#define NBPD_INITIALIZER		{ NBPD_L1, NBPD_L2, NBPD_L3, NBPD_L4 }
#define PDES_INITIALIZER		{ L2_BASE, L3_BASE, L4_BASE }
#define APDES_INITIALIZER		{ AL2_BASE, AL3_BASE, AL4_BASE }

#define PTP_LEVELS	4
#define PTP_SHIFT	9

#define	vtopte(va)	(PTE_BASE + PL1_I(va))
#define	kvtopte(va)	vtopte(va)

#define	avtopte(va)	(APTE_BASE + PL1_I(va))

#ifndef LOCORE
#include <sys/queue.h>

/*
 * Pmap stuff
 */
struct pmap_list;
LIST_HEAD(pmap_list, pmap); 				/* struct pmap_head: head of a pmap list */
struct pmap {
	LIST_ENTRY(pmap) 		pm_list;		/* List of all pmaps */
	pd_entry_t 				*pm_pdir;		/* KVA of page directory */
	pt_entry_t				*pm_ptab;		/* KVA of page table */

	bool_t					pm_pdchanged;	/* pdir changed */
	short					pm_dref;		/* page directory ref count */
	short					pm_count;		/* pmap reference count */
	simple_lock_data_t		pm_lock;		/* lock on pmap */
	struct pmap_statistics	pm_stats;		/* pmap statistics */
	long					pm_ptpages;		/* more stats: PT pages */
	int 					pm_flags;		/* see below */
	int						pm_active;		/* active on cpus */
	u_int32_t 				pm_cpus;		/* mask of CPUs using pmap */
};
typedef struct pmap 		*pmap_t;

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */
struct pv_entry {
	struct pv_entry			*pv_next;		/* next pv_entry */
	struct pmap				*pv_pmap;		/* pmap where mapping lies */
	vm_offset_t				pv_va;			/* virtual address for mapping */
	int						pv_flags;		/* flags */
};
typedef struct pv_entry		*pv_entry_t;

#define	PT_ENTRY_NULL		((pt_entry_t) 0)
#define	PD_ENTRY_NULL		((pd_entry_t) 0)
#define	PDP_ENTRY_NULL		((pdp_entry_t) 0)
#define	PML4_ENTRY_NULL		((pml4_entry_t) 0)

#define PTE_BASE			L1_BASE
#define PDP_PDE				(L4_BASE + PDIR_SLOT_PTE)
#define PDP_BASE			L4_BASE

#define APTE_BASE			AL1_BASE
#define APDP_PDE			(L4_BASE + PDIR_SLOT_APTE)
#define APDP_BASE			AL4_BASE

#ifdef _KERNEL

extern struct pmap  		kernel_pmap_store;
#define kernel_pmap 		(&kernel_pmap_store)
extern bool_t 				pmap_initialized;		/* Has pmap_init completed? */

extern int 					nkpt;					/* Initial number of kernel page tables */
extern uint64_t 			ptp_masks[];
extern uint64_t				ptp_shifts[];
extern uint64_t				nbpds[];
extern pd_entry_t 			*pdes[];
extern pd_entry_t 			*apdes[];

#define	pmap_resident_count(pmap)	\
	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		\
	((pmap)->pm_stats.wired_count)

#define pmap_lock_init(pmap, name) 	(simple_lock_init(&(pmap)->pm_lock, (name)))
#define pmap_lock(pmap)				(simple_lock(&(pmap)->pm_lock))
#define pmap_unlock(pmap)			(simple_unlock(&(pmap)->pm_lock))


#endif	/* KERNEL */
#endif 	/* !_LOCORE */
#endif /* _AMD64_PMAP_H_ */
