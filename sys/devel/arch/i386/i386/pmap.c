/*	$NetBSD: pmap.c,v 1.74.4.4 2012/02/24 17:29:32 sborrill Exp $	*/

/*
 * Copyright (c) 2007 Manuel Bouyer.
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
 *      This product includes software developed by Manuel Bouyer.
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
 *
 */

/*
 * Copyright (c) 2006 Mathieu Ropert <mro@adviseo.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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
 *    must display the following acknowledgement:
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
 * Copyright 2001 (c) Wasabi Systems, Inc.
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
 * This is the i386 pmap modified and generalized to support x86-64
 * as well. The idea is to hide the upper N levels of the page tables
 * inside pmap_get_ptp, pmap_free_ptp and pmap_growkernel. The rest
 * is mostly untouched, except that it uses some more generalized
 * macros and interfaces.
 *
 * This pmap has been tested on the i386 as well, and it can be easily
 * adapted to PAE.
 *
 * fvdl@wasabisystems.com 18-Jun-2001
 */

/*
 * pmap.c: i386 pmap module rewrite
 * Chuck Cranor <chuck@ccrc.wustl.edu>
 * 11-Aug-97
 *
 * history of this pmap module: in addition to my own input, i used
 *    the following references for this rewrite of the i386 pmap:
 *
 * [1] the NetBSD i386 pmap.   this pmap appears to be based on the
 *     BSD hp300 pmap done by Mike Hibler at University of Utah.
 *     it was then ported to the i386 by William Jolitz of UUNET
 *     Technologies, Inc.   Then Charles M. Hannum of the NetBSD
 *     project fixed some bugs and provided some speed ups.
 *
 * [2] the FreeBSD i386 pmap.   this pmap seems to be the
 *     Hibler/Jolitz pmap, as modified for FreeBSD by John S. Dyson
 *     and David Greenman.
 *
 * [3] the Mach pmap.   this pmap, from CMU, seems to have migrated
 *     between several processors.   the VAX version was done by
 *     Avadis Tevanian, Jr., and Michael Wayne Young.    the i386
 *     version was done by Lance Berc, Mike Kupfer, Bob Baron,
 *     David Golub, and Richard Draves.    the alpha version was
 *     done by Alessandro Forin (CMU/Mach) and Chris Demetriou
 *     (NetBSD/alpha).
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/msgbuf.h>
#include <sys/memrange.h>
#include <sys/sysctl.h>
#include <sys/cputopo.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

#include <i386/include/pte.h>

#include <devel/arch/i386/include/param.h>
#include <devel/arch/i386/include/pmap.h>

#include <arch/i386/include/pmap.h>
#include <arch/i386/include/pmap_tlb.h>

/*
 * Allocate various and sundry SYSMAPs used in the days of old VM
 * and not yet converted.  XXX.
 */
#ifdef DEBUG
struct {
	int kernel;		/* entering kernel mapping */
	int user;		/* entering user mapping */
	int overlay;	/* entering overlay mapping */
	int ptpneeded;	/* needed to allocate a PT page */
	int pwchange;	/* no mapping change, just wiring or protection */
	int wchange;	/* no mapping change, just wiring */
	int mchange;	/* was mapped but mapping to different page */
	int managed;	/* a managed page */
	int firstpv;	/* first mapping for this PA */
	int secondpv;	/* second mapping for this PA */
	int ci;			/* cache inhibited */
	int unmanaged;	/* not a managed page */
	int flushes;	/* cache flushes */
} enter_stats;
struct {
	int calls;
	int removes;
	int pvfirst;
	int pvsearch;
	int ptinvalid;
	int uflushes;
	int sflushes;
} remove_stats;

int debugmap = 0;
int pmapdebug = 0;
#define PDB_FOLLOW	0x0001
#define PDB_INIT	0x0002
#define PDB_ENTER	0x0004
#define PDB_REMOVE	0x0008
#define PDB_CREATE	0x0010
#define PDB_PTPAGE	0x0020
#define PDB_CACHE	0x0040
#define PDB_BITS	0x0080
#define PDB_COLLECT	0x0100
#define PDB_PROTECT	0x0200
#define PDB_PDRTAB	0x0400
#define PDB_PARANOIA	0x2000
#define PDB_WIRING	0x4000
#define PDB_PVDUMP	0x8000

int pmapvacflush = 0;
#define	PVF_ENTER	0x01
#define	PVF_REMOVE	0x02
#define	PVF_PROTECT	0x04
#define	PVF_TOTAL	0x80
#endif

uint64_t 	ptp_masks[] = PTP_MASK_INITIALIZER;
uint64_t 	ptp_shifts[] = PTP_SHIFT_INITIALIZER;
long 		NBPD[] = NBPD_INITIALIZER;
pd_entry_t 	*NPDE[] = PDES_INITIALIZER;
pd_entry_t 	*APDE[] = APDES_INITIALIZER;
long 		NKPTP[] = NKPTP_INITIALIZER;
long 		NKPTPMAX[] = NKPTPMAX_INITIALIZER;

/*
 * Get PDEs and PTEs for user/kernel address space
 */
#define pmap_ptab(m, v)         (&((m)->pm_ptab[((vm_offset_t)(v) >> PGSHIFT)]))
#define pmap_pde(m, v, lvl)     (&((m)->pm_pdir[PL_I(v, lvl)]))

/*
 * Given a map and a machine independent protection code,
 * convert to a vax protection code.
 */
#define pte_prot(m, p)	(protection_codes[p])
int	protection_codes[8];

/*
 * LAPIC virtual address, and fake physical address.
 */
volatile vm_offset_t local_apic_va;
vm_offset_t local_apic_pa;

static int pgeflag = 0;			/* PG_G or-in */
static int pseflag = 0;			/* PG_PS or-in */

static int nkpt = NKPT;

#ifdef PMAP_PAE_COMP
pt_entry_t pg_nx;
#else
#define	pg_nx	0
#endif

int pat_works;
int pg_ps_enabled;
int elf32_nxstack;
int i386_pmap_PDRSHIFT;

#define	PAT_INDEX_SIZE	8
static int 				pat_index[PAT_INDEX_SIZE];	/* cache mode to PAT index conversion */

extern char 			_end[]; 					/* boot.ldscript */
vm_offset_t 			kernel_vm_end;
u_long 					physfree;					/* phys addr of next free page */
u_long 					vm86phystk;					/* PA of vm86/bios stack */
u_long 					vm86paddr;					/* address of vm86 region */
int 					vm86pa;						/* phys addr of vm86 region */
u_long 					KPTphys;					/* phys addr of kernel page tables */
u_long 					KERNend;
u_long 					tramp_idleptd;

vm_offset_t 			avail_start;				/* PA of first available physical page */
vm_offset_t				avail_end;					/* PA of last available physical page */
vm_offset_t				virtual_avail;  			/* VA of first avail page (after kernel bss)*/
vm_offset_t				virtual_end;				/* VA of last avail page (end of kernel AS) */
vm_offset_t				vm_first_phys;				/* PA of first managed page */
vm_offset_t				vm_last_phys;				/* PA just past last managed page */

int						i386pagesperpage;			/* PAGE_SIZE / I386_PAGE_SIZE */
bool_t					pmap_initialized = FALSE;	/* Has pmap_init completed? */
char					*pmap_attributes;			/* reference and modify bits */

pd_entry_t 				*IdlePTD;					/* phys addr of kernel PTD */
#ifdef PMAP_PAE_COMP
pdpt_entry_t 			*IdlePDPT;					/* phys addr of kernel PDPT */
#endif
pt_entry_t 				*KPTmap;					/* address of kernel page tables */
pv_entry_t				pv_table;					/* array of entries, one per page */

/* linked list of all non-kernel pmaps */
struct pmap	kernel_pmap_store;
static struct pmap_list pmap_header;

/*
 * All those kernel PT submaps that BSD is so fond of
 */
static pd_entry_t 	*KPTD;
pt_entry_t			*CMAP1, *CMAP2, *CMAP3, *cmmap;
caddr_t				CADDR1, CADDR2, CADDR3, cvmmap;
pt_entry_t			*msgbufmap;
struct msgbuf		*msgbufp;

static pt_entry_t 	*PMAP1 = NULL, *PMAP2, *PMAP3;
static pt_entry_t 	*PADDR1 = NULL, *PADDR2, *PADDR3;

/* static methods */
static vm_offset_t	pmap_bootstrap_valloc(size_t);
static vm_offset_t	pmap_bootstrap_palloc(size_t);
static pd_entry_t 	*pmap_valid_entry(pmap_t, pd_entry_t *, vm_offset_t);
static pdpt_entry_t *pmap_pdpt(pmap_t, vm_offset_t);
static pt_entry_t 	*pmap_pte(pmap_t, vm_offset_t);

static vm_offset_t
pa_index(pa)
	vm_offset_t pa;
{
	vm_offset_t index;
	index = atop(pa - vm_first_phys);
	return (index);
}

static pv_entry_t
pa_to_pvh(pa)
	vm_offset_t pa;
{
	pv_entry_t pvh;
	pvh = &pv_table[pa_index(pa)];
	return (pvh);
}

__inline static void
pmap_apte_flush(pmap)
	pmap_t pmap;
{
	pmap_tlb_shootdown(pmap, (vm_offset_t)-1LL, 0, 0);
	pmap_tlb_shootnow(pmap, 0);
}

pt_entry_t *
pmap_map_pte(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pd_entry_t *pde, *apde;
	pd_entry_t opde, npde;

	if (pmap && pmap_pde_v(pmap_pde(pmap, va, 1))) {
		/* are we current address space or kernel? */
		if ((pmap->pm_pdir >= PTE_BASE && pmap->pm_pdir < L2_BASE)) {
			pde = pmap_vtopte(va);
			return (pde);
		} else {
			/* otherwise, we are alternate address space */
			if (pmap->pm_pdir >= APTE_BASE && pmap->pm_pdir < AL2_BASE) {
				apde = pmap_avtopte(va);
				tlbflush();
			}
			opde = *APDP_PDE & PG_FRAME;
			if (!(opde & PG_V) || opde != pmap_pdirpa(pmap, 0)) {
				npde = (pd_entry_t)(pmap->pm_pdirpa | PG_RW | PG_V | PG_A | PG_M);
				apde = npde;
				if ((opde & PG_V)) {
					pmap_apte_flush(pmap);
				}
			}
			return (apde);
		}
	}
	return (NULL);
}

pd_entry_t *
pmap_map_pde(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pd_entry_t *pde, *apde;
	pd_entry_t opde, npde;
	unsigned long index;
	int i;

	for (i = PTP_LEVELS; i > 1; i--) {
		index = PL_I(va, i);
		if (pmap && pmap_pde_v(pmap_pde(pmap, va, i))) {
			/* are we current address space or kernel? */
			if (pmap->pm_pdir <= PDP_PDE && pmap->pm_pdir == &NPDE[i - 2][index]) {
				pde = &NPDE[i - 2][index];
				return (pde);
			} else {
				/* otherwise, we are alternate address space */
				if (pmap->pm_pdir != APDP_PDE && pmap->pm_pdir == &APDE[i - 2][index]) {
					apde = &APDE[i - 2][index];
					tlbflush();
				}
				opde = *APDP_PDE & PG_FRAME;
				if (!(opde & PG_V) || opde != pmap_pdirpa(pmap, index)) {
					npde = (pd_entry_t)(pmap_pdirpa(pmap, index) | PG_RW | PG_V | PG_A | PG_M);
					apde = npde;
					if ((opde & PG_V)) {
						pmap_apte_flush(pmap);
					}
				}
				return (apde);
			}
		}
	}
	return (NULL);
}

static void
pmap_pinit_pdir(pdir, pdirpa)
	pd_entry_t *pdir;
	vm_offset_t pdirpa;
{
	int i;
	int npde;

	/*
	 * No need to allocate page table space yet but we do need a
	 * valid page directory table.
	 */
	pdir = (pd_entry_t *)kmem_alloc(kernel_map, PDIR_SLOT_PTE * sizeof(pd_entry_t));
	pdirpa = pdir[PDIR_SLOT_PTE] & PG_FRAME;

	/* zero init area */
	bzero(pdir, PDIR_SLOT_PTE * sizeof(pd_entry_t));

	/* put in recursive PDE to map the PTEs */
	pdir[PDIR_SLOT_PTE] = pdirpa | PG_V | PG_KW;

	npde = NKPTP[PTP_LEVELS - 1];

	bcopy(&PDP_BASE[PDIR_SLOT_KERN], &pdir[PDIR_SLOT_KERN], npde * sizeof(pd_entry_t));

	/* install self-referential address mapping entry */
	for (i = 0; i < NPGPTD; i++) {
		pdir[PDIR_SLOT_PTE + i] = pmap_extract(kernel_pmap, (vm_offset_t)pdir[i]);
#ifdef PMAP_PAE_COMP
		pdirpa[i] = pdir[PDIR_SLOT_PTE + i] & PG_FRAME;
#endif
	}

	/* zero the rest */
	bzero(&pdir[PDIR_SLOT_KERN + npde], (NPDEPG - (PDIR_SLOT_KERN + npde)) * sizeof(pd_entry_t));
}

#ifdef PMAP_PAE_COMP
static void
pmap_pinit_pdpt(pdpt)
	pdpt_entry_t *pdpt;
{
	int i;

	pdpt = (pdpt_entry_t *)kmem_alloc(kernel_map, (vm_offset_t)(NPGPTD * sizeof(pdpt_entry_t)));
	KASSERT(((vm_offset_t)pdpt & ((NPGPTD * sizeof(pdpt_entry_t)) - 1)) == 0/*, ("pmap_pinit: pdpt misaligned")*/);
	KASSERT(pmap_extract(kernel_pmap, (vm_offset_t)pdpt) < (4ULL<<30)/*, ("pmap_pinit: pdpt above 4g")*/);

	/* install self-referential address mapping entry */
	for (i = 0; i < NPGPTD; i++) {
		pdpt[i] = pmap_extract(kernel_pmap, (vm_offset_t)pdpt) | PG_V;
	}
}
#endif

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
void
pmap_pinit(pmap)
	register pmap_t pmap;
{
	pmap_pinit_pdir(pmap->pm_pdir, pmap->pm_pdirpa);

#ifdef PMAP_PAE_COMP
	pmap_pinit_pdpt(pmap->pm_pdpt);
#endif

	pmap->pm_active = 0;
	pmap->pm_count = 1;
	pmap_lock_init(pmap, "pmap_lock");
	bzero(&pmap->pm_stats, sizeof(pmap->pm_stats));

	pmap_lock(pmap);
	LIST_INSERT_HEAD(&pmaps, pmap, pm_list);
	pmap_unlock(pmap);
}

static pd_entry_t *
pmap_valid_entry(pmap, pdir, va)
	register pmap_t pmap;
	pd_entry_t *pdir;
	vm_offset_t va;
{
	if (pdir && pmap_pte_v(pdir)) {
		if (pdir == pmap_map_pte(pmap, va) || pmap == kernel_pmap) {
			return ((pd_entry_t *)pmap_map_pte(pmap, va));
		} else if (pdir == pmap_map_pde(pmap, va)) {
			return (pmap_map_pde(pmap, va));
		} else {
			return (NULL);
		}
	}
	return (NULL);
}

static pdpt_entry_t *
pmap_pdpt(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pdpt_entry_t *pdpt;

	pdpt = (pdpt_entry_t *)pmap_pde(pmap, va, 2);
	if (pmap_valid_entry(pmap, (pdpt_entry_t *)pdpt, va)) {
		return (pdpt);
	}
	return (NULL);
}

/*
 *	Routine:	pmap_pte
 *	Function:
 *		Extract the page table entry associated
 *		with the given map/virtual_address pair.
 * [ what about induced faults -wfj]
 */
static pt_entry_t *
pmap_pte(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pt_entry_t *pte;

	pte = (pt_entry_t *)pmap_pde(pmap, va, 1);
	if (pmap_valid_entry(pmap, (pt_entry_t *)pte, va)) {
		return (pte);
	}
	return (NULL);
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */
vm_offset_t
pmap_extract(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	register vm_offset_t pa;

	pa = 0;
	if (pmap && pmap == kernel_pmap) {
		if (pmap_pde_v(pmap_map_pte(pmap, va))) {
			pa = *(int *)pmap_map_pte(pmap, va);
		}
	} else {
		if (pmap_pde_v(pmap_map_pde(pmap, va))) {
			pa = *(int *)pmap_map_pde(pmap, va);
		}
	}
	if (pa) {
		pa = (pa & PG_FRAME) | (va & ~PG_FRAME);
	}
	return (pa);
}
