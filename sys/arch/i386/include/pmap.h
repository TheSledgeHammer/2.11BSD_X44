/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
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
 *	@(#)pmap.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 * from hp300:	@(#)pmap.h	7.2 (Berkeley) 12/16/90
 */

#ifndef _I386_PMAP_H_
#define _I386_PMAP_H_

/*
 * virtual address to page table entry and
 * to physical address. Likewise for alternate address space.
 * Note: these work recursively, thus vtopte of a pte will give
 * the corresponding pde that in turn maps it.
 */
#define	vtopte(va)			(PTmap + i386_btop(va))
#define	kvtopte(va)			vtopte(va)
#define	ptetov(pt)			(i386_ptob(pt - PTmap))
#define	vtophys(va)  		(i386_ptob(vtopte(va)) | ((int)(va) & PGOFSET))
#define ispt(va)			((va) >= UPT_MIN_ADDRESS && (va) <= KPT_MAX_ADDRESS)

#define	avtopte(va)			(APTmap + i386_btop(va))
#define	ptetoav(pt)	 		(i386_ptob(pt - APTmap))
#define	avtophys(va)  		(i386_ptob(avtopte(va)) | ((int)(va) & PGOFSET))

#ifndef LOCORE
#include <sys/queue.h>

/*
 * Pmap stuff
 */
struct pmap_head;
LIST_HEAD(pmap_head, pmap); 				/* struct pmap_head: head of a pmap list */
struct pmap {
	LIST_ENTRY(pmap) 		pm_list;		/* List of all pmaps */
#ifdef PMAP_PAE_COMP
	uint64_t				*pm_pdir;		/* KVA of page directory */
	uint64_t				*pm_ptab;		/* KVA of page table */
	uint64_t				*pm_pdpt;		/* KVA of page director pointer table */
#else
	uint32_t				*pm_pdir;		/* KVA of page directory */
	uint32_t				*pm_ptab;		/* KVA of page table */
#endif
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

#ifdef _KERNEL
extern struct pmap  		*kernel_pmap_store;
#define kernel_pmap 		(&kernel_pmap_store)
extern u_long 				physfree;		/* phys addr of next free page */
extern u_long 				vm86phystk;		/* PA of vm86/bios stack */
//extern u_long 				vm86paddr;		/* address of vm86 region */
extern int 					vm86pa;			/* phys addr of vm86 region */
extern u_long 				KPTphys;		/* phys addr of kernel page tables */
extern u_long 				KERNend;
extern vm_offset_t 			kernel_vm_end;
extern u_long 				tramp_idleptd;
extern int 					pae_mode;
extern int 					i386_pmap_PDRSHIFT;
pv_entry_t					pv_table;		/* array of entries, one per page */

#define pa_index(pa)		atop(pa - vm_first_phys)
#define pa_to_pvh(pa)		(&pv_table[pa_index(pa)])

#define	pmap_resident_count(pmap)	\
	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		\
	((pmap)->pm_stats.wired_count)

#define pmap_lock_init(pmap, name) 	(simple_lock_init((pmap)->pm_lock, (name)))
#define pmap_lock(pmap)				(simple_lock((pmap)->pm_lock))
#define pmap_unlock(pmap)			(simple_unlock((pmap)->pm_lock))

struct cpu_info;
struct pcb;
/* proto types */
void        pmap_activate(pmap_t, struct pcb *);
void        pmap_kenter(vm_offset_t, vm_offset_t);
void		pmap_kremove(vm_offset_t);
void        pmap_changebit(vm_offset_t, int, bool_t);
void        pmap_bios16_enter(void);
void        pmap_bios16_leave(void *);
/* SMP */
void        pmap_invalidate_page(pmap_t, vm_offset_t);
void        pmap_invalidate_range(pmap_t, vm_offset_t, vm_offset_t);
void        pmap_invalidate_all(pmap_t);
/* TLB */
void		pmap_tlb_init(void);
void        pmap_tlb_shootnow(pmap_t, int32_t);
void        pmap_tlb_shootdown(pmap_t, vm_offset_t, vm_offset_t, int32_t *);
void        pmap_do_tlb_shootdown(pmap_t, struct cpu_info *);
/* misc.c */
void		clearseg(int);
void		copyseg(int, int);
void		physcopyseg(int, int);
//int			copyout(const void *, void *, size_t);
#endif	/* KERNEL */
#endif 	/* !LOCORE */
#endif 	/* _I386_PMAP_H_ */
