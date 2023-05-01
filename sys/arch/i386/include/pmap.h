/*	$NetBSD: pmap.h,v 1.103 2008/10/26 06:57:30 mrg Exp $	*/

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

/*
 * pmap.h: see pmap.c for the history of this pmap module.
 */

#ifndef _I386_PMAP_H_
#define _I386_PMAP_H_

/*
 * see pte.h for a description of i386 MMU terminology and hardware
 * interface.
 *
 * a pmap describes a processes' 4GB virtual address space.  when PAE
 * is not in use, this virtual address space can be broken up into 1024 4MB
 * regions which are described by PDEs in the PDP.  the PDEs are defined as
 * follows:
 *
 * (ranges are inclusive -> exclusive, just like vm_map_entry start/end)
 * (the following assumes that KERNBASE is 0xc0000000)
 *
 * PDE#s	VA range		usage
 * 0->766	0x0 -> 0xbfc00000	user address space
 * 767		0xbfc00000->		recursive mapping of PDP (used for
 *			0xc0000000	linear mapping of PTPs)
 * 768->1023	0xc0000000->		kernel address space (constant
 *			0xffc00000	across all pmap's/processes)
 * 1023		0xffc00000->		"alternate" recursive PDP mapping
 *			<end>		(for other pmaps)
 *
 *
 * note: a recursive PDP mapping provides a way to map all the PTEs for
 * a 4GB address space into a linear chunk of virtual memory.  in other
 * words, the PTE for page 0 is the first int mapped into the 4MB recursive
 * area.  the PTE for page 1 is the second int.  the very last int in the
 * 4MB range is the PTE that maps VA 0xfffff000 (the last page in a 4GB
 * address).
 *
 * all pmap's PD's must have the same values in slots 768->1023 so that
 * the kernel is always mapped in every process.  these values are loaded
 * into the PD at pmap creation time.
 *
 * at any one time only one pmap can be active on a processor.  this is
 * the pmap whose PDP is pointed to by processor register %cr3.  this pmap
 * will have all its PTEs mapped into memory at the recursive mapping
 * point (slot #767 as show above).  when the pmap code wants to find the
 * PTE for a virtual address, all it has to do is the following:
 *
 * address of PTE = (767 * 4MB) + (VA / PAGE_SIZE) * sizeof(pt_entry_t)
 *                = 0xbfc00000 + (VA / 4096) * 4
 *
 * what happens if the pmap layer is asked to perform an operation
 * on a pmap that is not the one which is currently active?  in that
 * case we take the PA of the PDP of non-active pmap and put it in
 * slot 1023 of the active pmap.  this causes the non-active pmap's
 * PTEs to get mapped in the final 4MB of the 4GB address space
 * (e.g. starting at 0xffc00000).
 *
 * the following figure shows the effects of the recursive PDP mapping:
 *
 *   PDP (%cr3)
 *   +----+
 *   |   0| -> PTP#0 that maps VA 0x0 -> 0x400000
 *   |    |
 *   |    |
 *   | 767| -> points back to PDP (%cr3) mapping VA 0xbfc00000 -> 0xc0000000
 *   | 768| -> first kernel PTP (maps 0xc0000000 -> 0xc0400000)
 *   |    |
 *   |1023| -> points to alternate pmap's PDP (maps 0xffc00000 -> end)
 *   +----+
 *
 * note that the PDE#767 VA (0xbfc00000) is defined as "PTE_BASE"
 * note that the PDE#1023 VA (0xffc00000) is defined as "APTE_BASE"
 *
 * starting at VA 0xbfc00000 the current active PDP (%cr3) acts as a
 * PTP:
 *
 * PTP#767 == PDP(%cr3) => maps VA 0xbfc00000 -> 0xc0000000
 *   +----+
 *   |   0| -> maps the contents of PTP#0 at VA 0xbfc00000->0xbfc01000
 *   |    |
 *   |    |
 *   | 767| -> maps contents of PTP#767 (the PDP) at VA 0xbfeff000
 *   | 768| -> maps contents of first kernel PTP
 *   |    |
 *   |1023|
 *   +----+
 *
 * note that mapping of the PDP at PTP#767's VA (0xbfeff000) is
 * defined as "PDP_BASE".... within that mapping there are two
 * defines:
 *   "PDP_PDE" (0xbfeffbfc) is the VA of the PDE in the PDP
 *      which points back to itself.
 *   "APDP_PDE" (0xbfeffffc) is the VA of the PDE in the PDP which
 *      establishes the recursive mapping of the alternate pmap.
 *      to set the alternate PDP, one just has to put the correct
 *	PA info in *APDP_PDE.
 *
 * note that in the APTE_BASE space, the APDP appears at VA
 * "APDP_BASE" (0xfffff000).
 *
 * When PAE is in use, the L3 page directory breaks up the address space in
 * 4 1GB * regions, each of them broken in 512 2MB regions by the L2 PD
 * (the size of the pages at the L1 level is still 4K).
 * The kernel virtual space is mapped by the last entry in the L3 page,
 * the first 3 entries mapping the user VA space.
 * Because the L3 has only 4 entries of 1GB each, we can't use recursive
 * mappings at this level for PDP_PDE and APDP_PDE (this would eat 2 of the
 * 4GB virtual space). There's also restrictions imposed by Xen on the
 * last entry of the L3 PD, which makes it hard to use one L3 page per pmap
 * switch %cr3 to switch pmaps. So we use one static L3 page which is
 * always loaded in %cr3, and we use it as 2 virtual PD pointers: one for
 * kenrel space (L3[3], always loaded), and one for user space (in fact the
 * first 3 entries of the L3 PD), and we claim the VM has only a 2-level
 * PTP (with the L2 index extended by 2 bytes).
 * PTE_BASE and APTE_BASE will need 4 entries in the L2 page table.
 * In addition, we can't recursively map L3[3] (Xen wants the ref count on
 * this page to be exactly once), so we use a shadow PD page for the last
 * L2 PD. The shadow page could be static too, but to make pm_pdir[]
 * contigous we'll allocate/copy one page per pmap.
 */

#ifdef PMAP_PAE_COMP
#define PAE_MODE 	0	/* pae enabled */
#else
#undef PMAP_PAE_COMP
#define PAE_MODE 	1	/* pae disabled */
#endif

/*
 * Mask to get rid of the sign-extended part of addresses.
 */
#define VA_SIGN_MASK			0
#define VA_SIGN_NEG(va)			((va) | VA_SIGN_MASK)
/*
 * XXXfvdl this one's not right.
 */
#define VA_SIGN_POS(va)			((va) & ~VA_SIGN_MASK)

#ifndef NKPDE
#define NKPDE					(KVA_PAGES)			/* number of page tables/pde's */
#endif

#ifndef PMAP_PAE_COMP /* PMAP_NOPAE */

/* NOPAE Constants */
#define	PD_SHIFT				SEGSHIFT			/* LOG2(NBPDR) (22) */

#define	NTRPPTD					1
#define	LOWPTDI					1
#define	KERNPTDI				2

#define NPGPTD					1
#define NPGPTD_SHIFT			10

#define	PDRSHIFT				PD_SHIFT
#define NBPDR					(1 << PD_SHIFT)		/* bytes/page dir */

#define KVA_PAGES				(256*4)

#ifndef NKPT
#define	NKPT					30
#endif

#define	L1_SHIFT				12
#define	L2_SHIFT				PD_SHIFT
#define	NBPD_L1					(1ULL << L1_SHIFT) 	/* # bytes mapped by L1 ent (4K) */
#define	NBPD_L2					(1ULL << L2_SHIFT) 	/* # bytes mapped by L2 ent (4MB) */

#define L2_MASK					0xffc00000			/* PG_PS_FRAME: large (4MB) page frame mask */
#define L1_MASK					0x003ff000

#define L2_FRAME				(L2_MASK)
#define L1_FRAME				(L2_FRAME | L1_MASK)

#define L2_SLOT_PTE				(KERNBASE/NBPD_L2-1)/* 767: for recursive PDP map */
#define L2_SLOT_KERN			(KERNBASE/NBPD_L2)  /* 768: start of kernel space */
#define	L2_SLOT_KERNBASE 		L2_SLOT_KERN
#define L2_SLOT_APTE			1007

typedef uint32_t 				pt_entry_t;			/* PTE */
typedef uint32_t 				pd_entry_t;			/* PDE */
typedef	uint32_t 				pdpt_entry_t;		/* PDPT */
#ifdef OVERLAY
typedef uint32_t 				ovl_entry_t;		/* OVL */
#endif

#else /* PMAP_PAE */

/* PAE Constants  */
#define	PD_SHIFT				(SEGSHIFT-1)		 /* LOG2(NBPDR) (21) */

#define	NTRPPTD					2					/* Number of PTDs for trampoline mapping */
#define	LOWPTDI					2					/* low memory map pde */
#define	KERNPTDI				4					/* start of kernel text pde */

#define NPGPTD					4					/* Num of pages for page directory */
#define NPGPTD_SHIFT			9

#define	PDRSHIFT				PD_SHIFT
#define NBPDR					(1 << PD_SHIFT)		/* bytes/page dir */

#define KVA_PAGES				(512*4)

#ifndef NKPT
#define	NKPT					240
#endif

#define	L1_SHIFT				12
#define	L2_SHIFT				PD_SHIFT
#define	L3_SHIFT				30
#define	NBPD_L1					(1ULL << L1_SHIFT) 	/* # bytes mapped by L1 ent (4K) */
#define	NBPD_L2					(1ULL << L2_SHIFT) 	/* # bytes mapped by L2 ent (2MB) */
#define	NBPD_L3					(1ULL << L3_SHIFT) 	/* # bytes mapped by L3 ent (1GB) */

#define	L3_MASK					0xc0000000
#define	L2_REALMASK				0x3fe00000
#define	L2_MASK					(L2_REALMASK | L3_MASK)
#define	L1_MASK					0x001ff000

#define	L3_FRAME				(L3_MASK)
#define	L2_FRAME				(L3_FRAME | L2_MASK)
#define	L1_FRAME				(L2_FRAME | L1_MASK)

//#define	PG_FRAME_PAE			(0x000ffffffffff000ull)
//#define	PG_PS_FRAME				(0x000fffffffe00000ull)

/* macros to get real L2 and L3 index, from our "extended" L2 index */
#define l2tol3(idx)				((idx) >> (L3_SHIFT - L2_SHIFT))
#define l2tol2(idx)				((idx) & (L2_REALMASK >>  L2_SHIFT))

#define L2_SLOT_PTE				(KERNBASE/NBPD_L2-4) /* 1532: for recursive PDP map */
#define L2_SLOT_KERN			(KERNBASE/NBPD_L2)   /* 1536: start of kernel space */
#define	L2_SLOT_KERNBASE 		L2_SLOT_KERN
#define L2_SLOT_APTE			1960

typedef uint64_t 				pt_entry_t;			/* PTE */
typedef uint64_t 				pd_entry_t;			/* PDE */
typedef uint64_t 				pdpt_entry_t;		/* PDPT */
#ifdef OVERLAY
typedef uint64_t 				ovl_entry_t;		/* OVL */
#endif
#endif

#define PDIR_SLOT_KERN			L2_SLOT_KERN
#define PDIR_SLOT_PTE			L2_SLOT_PTE
#define PDIR_SLOT_APTE			L2_SLOT_APTE

#define L1_BASE     			((pt_entry_t *)(PDIR_SLOT_PTE * NBPD_L2))
#define L2_BASE     			((pd_entry_t *)((char *)L1_BASE + L2_SLOT_PTE * NBPD_L1))

#define AL1_BASE    			((pt_entry_t *)(VA_SIGN_NEG((PDIR_SLOT_APTE * NBPD_L2))))
#define AL2_BASE    			((pd_entry_t *)((char *)AL1_BASE + L2_SLOT_PTE * NBPD_L1))

/* largest value (-1 for APTP space) */
#define NKL2_MAX_ENTRIES		(KVA_PAGES - (KERNBASE/NBPD_L2) - 1)
#define NKL1_MAX_ENTRIES		(unsigned long)(NKL2_MAX_ENTRIES * NPDEPG)

#define NKL2_START_ENTRIES		0	/* XXX computed on runtime */
#define NKL1_START_ENTRIES		0	/* XXX unused */

/*
 * PL*_1: generate index into pde/pte arrays in virtual space
 */
#define PL1_I(VA)			(((VA_SIGN_POS(VA)) & L1_FRAME) >> L1_SHIFT)
#define PL2_I(VA)			(((VA_SIGN_POS(VA)) & L2_FRAME) >> L2_SHIFT)
#define PL_I(va, lvl) 		(((VA_SIGN_POS(va)) & ptp_masks[(lvl)-1]) >> ptp_shifts[(lvl)-1])

#define PTP_MASK_INITIALIZER	{ L1_FRAME, L2_FRAME }
#define PTP_SHIFT_INITIALIZER	{ L1_SHIFT, L2_SHIFT }
#define NBPD_INITIALIZER		{ NBPD_L1, NBPD_L2 }
#define PDES_INITIALIZER		{ L2_BASE }
#define APDES_INITIALIZER		{ AL2_BASE }
#define NKPTP_INITIALIZER		{ NKL1_START_ENTRIES, NKL2_START_ENTRIES }
#define NKPTPMAX_INITIALIZER	{ NKL1_MAX_ENTRIES, NKL2_MAX_ENTRIES }

#define PTP_LEVELS			2
#define PTP_SHIFT			9

/*
 * One page directory, shared between
 * kernel and user modes.
 */
#define I386_PAGE_SIZE		NBPG
#define I386_PDR_SIZE		NBPDR

#define	PTDPTDI		        (NPDEPTD - NTRPPTD - NPGPTD)            /* ptd entry that points to ptd */

/*
 * virtual address to page table entry and
 * to physical address. Likewise for alternate address space.
 * Note: these work recursively, thus vtopte of a pte will give
 * the corresponding pde that in turn maps it.
 */
#define	vtopte(va)			(PTE_BASE + i386_btop(va))
#define	kvtopte(va)			vtopte(va)
#define	ptetov(pt)			(i386_ptob(pt - PTE_BASE))
#define	vtophys(va)  		(i386_ptob(vtopte(va)) | ((int)(va) & PGOFSET))

#define	avtopte(va)			(APTE_BASE + i386_btop(va))
#define	ptetoav(pt)	 		(i386_ptob(pt - APTE_BASE))
#define	avtophys(va)  		(i386_ptob(avtopte(va)) | ((int)(va) & PGOFSET))

#ifndef LOCORE
#include <sys/queue.h>

/*
 * Pmap stuff
 */
struct pmap_list;
LIST_HEAD(pmap_list, pmap); 				/* struct pmap_head: head of a pmap list */
struct pmap {
	LIST_ENTRY(pmap) 		pm_list;		/* List of all pmaps */
#ifdef OVERLAY
	ovl_entry_t				*pm_ovltab;		/* KVA of Overlay */
#endif
#ifdef PMAP_PAE_COMP
	pt_entry_t				*pm_ptab;		/* KVA of page table */
	pd_entry_t				*pm_pdir;		/* KVA of page directory */
	pdpt_entry_t			*pm_pdpt;		/* KVA of page director pointer table */
#else
	pt_entry_t				*pm_ptab;		/* KVA of page table */
	pd_entry_t				*pm_pdir;		/* KVA of page directory */
#endif
	vm_offset_t				pm_pdirpa;		/* PA of PD (read-only after create) */

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
	char					pv_attr;		/* attrs array */
};
typedef struct pv_entry		*pv_entry_t;

#define	PT_ENTRY_NULL		((pt_entry_t)0)
#define	PD_ENTRY_NULL		((pd_entry_t)0)
#define	PDPT_ENTRY_NULL		((pdpt_entry_t)0)

/*
 * PTmap is recursive pagemap at top of virtual address space.
 * Within PTmap, the page directory can be found (third indirection).
 */
#define PTE_BASE			L1_BASE
#define PDP_PDE				(L2_BASE + PDIR_SLOT_PTE)
#define PDP_BASE			L2_BASE

#define PTmap				PTE_BASE
#define PTD					PDP_BASE
#define PTDpde				PDP_PDE

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 */
#define APTE_BASE			AL1_BASE
#define APDP_PDE			(L2_BASE + PDIR_SLOT_APTE)
#define APDP_BASE			AL2_BASE

#define APTmap      		APTE_BASE
#define APTD        		APDP_BASE
#define APTDpde				APDP_PDE

#ifdef _KERNEL

extern pd_entry_t 			*IdlePTD;
extern pt_entry_t 			*KPTmap;
#ifdef PMAP_PAE_COMP
extern pdpt_entry_t 		*IdlePDPT;
#endif
extern struct pmap  		kernel_pmap_store;
#define kernel_pmap 		(&kernel_pmap_store)
extern bool_t 				pmap_initialized;		/* Has pmap_init completed? */

extern u_long 				physfree;		/* phys addr of next free page */
extern u_long 				vm86phystk;		/* PA of vm86/bios stack */
extern int 					vm86pa;			/* phys addr of vm86 region */
extern u_long 				KPTphys;		/* phys addr of kernel page tables */
extern u_long 				KERNend;
extern vm_offset_t 			kernel_vm_end;
extern u_long 				tramp_idleptd;
extern int 					pae_mode;
//extern pv_entry_t			pv_table;		/* array of entries, one per page */

extern uint32_t 			ptp_masks[];
extern uint32_t				ptp_shifts[];
extern pd_entry_t 			*NPDE[];
extern pd_entry_t 			*APDE[];
extern long 				NBPD[], NKPTP[], NKPTPMAX[];

#ifdef PMAP_PAE_COMP
#define pmap_pdirpa(pmap, index) 	((pmap)->pm_pdirpa[l2tol3(index)] + l2tol2(index) * sizeof(pd_entry_t))
#else
#define pmap_pdirpa(pmap, index) 	((pmap)->pm_pdirpa + (index) * sizeof(pd_entry_t))
#endif

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)
#define pmap_lock_init(pmap, name) 	(simple_lock_init(&(pmap)->pm_lock, (name)))
#define pmap_lock(pmap)				(simple_lock(&(pmap)->pm_lock))
#define pmap_unlock(pmap)			(simple_unlock(&(pmap)->pm_lock))

struct cpu_info;
struct pcb;
/* proto types */
void        pmap_activate(pmap_t, struct pcb *);
void        pmap_kenter(vm_offset_t, vm_offset_t);
void	    pmap_kremove(vm_offset_t);
void 	    pmap_init_pat(void);
void	    pmap_set_nx(void);
void	    pmap_remap_lower(bool_t);
/* SMP */
void        pmap_invalidate_page(pmap_t, vm_offset_t);
void        pmap_invalidate_range(pmap_t, vm_offset_t, vm_offset_t);
void        pmap_invalidate_all(pmap_t);
/* TLB */
void		pmap_tlb_init(void);
void        pmap_tlb_shootnow(pmap_t, int32_t);
pt_entry_t	pmap_tlb_pte(vm_offset_t, vm_offset_t);
void        pmap_tlb_shootdown(pmap_t, vm_offset_t, pt_entry_t, int32_t *);
void        pmap_do_tlb_shootdown(pmap_t, struct cpu_info *);
/* misc */
void	    i386_protection_init(void);
bool_t	    pmap_testbit(vm_offset_t, int);
void        pmap_changebit(vm_offset_t, int, bool_t);
void        *pmap_bios16_enter(void);
void        pmap_bios16_leave(void *);
/* Overlay */
#ifdef OVERLAY
extern u_long			OVLphys;
extern ovl_entry_t		*IdleOVL;

void		pmap_pinit_ovltab(ovl_entry_t *);
#endif 	/* OVERLAY */
#endif	/* KERNEL */
#endif 	/* !_LOCORE */
#endif /* _I386_PMAP_H_ */
