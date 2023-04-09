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

#ifndef _AMD64_PMAP_H_
#define _AMD64_PMAP_H_

/*
 * We use the same numbering of the page table pages for 5-level and
 * 4-level paging structures.
 */
#define	NUPML5E		(NPML5EPG / 2)			/* number of userland PML5 pages */
#define	NUPML4E		(NUPML5E * NPML4EPG)	/* number of userland PML4 pages */
#define	NUPDPE		(NUPML4E * NPDPEPG)		/* number of userland PDPpages */
#define	NUPDE		(NUPDPE * NPDEPG)		/* number of userland PD entries */
#define	NUP4ML4E	(NPML4EPG / 2)


typedef u_int64_t pd_entry_t;
typedef u_int64_t pt_entry_t;
typedef u_int64_t pdp_entry_t;
typedef u_int64_t pml4_entry_t;
typedef u_int64_t pml5_entry_t;


#ifndef LOCORE
#include <sys/queue.h>

/*
 * Pmap stuff
 */
struct pmap_head;
LIST_HEAD(pmap_head, pmap); 				/* struct pmap_head: head of a pmap list */
struct pmap {
	LIST_ENTRY(pmap) 		pm_list;		/* List of all pmaps */
	pd_entry_t 				*pm_pdir;		/* KVA of page directory */
	pt_entry_t				*pm_ptab;		/* KVA of page table */
	pdp_entry_t				*pm_pdp;		/* KVA of page directory page */
	pml4_entry_t			*pm_pl4;		/* KVA of level 4 page table */
	pml5_entry_t			*pm_pl5;		/* KVA of level 5 page table */

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

#define	PD_ENTRY_NULL		((pd_entry_t) 0)
#define	PT_ENTRY_NULL		((pt_entry_t) 0)
#define	PDP_ENTRY_NULL		((pdp_entry_t) 0)
#define	PML4_ENTRY_NULL		((pml4_entry_t) 0)
#define	PML5_ENTRY_NULL		((pml5_entry_t) 0)

extern u_int64_t KPML4phys;	/* physical address of kernel level 4 */
extern u_int64_t KPML5phys;	/* physical address of kernel level 5 */

#endif 	/* !_LOCORE */
#endif /* _AMD64_PMAP_H_ */
