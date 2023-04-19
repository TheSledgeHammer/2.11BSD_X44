/*-
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
 *	@(#)pmap.c	8.1 (Berkeley) 6/11/93
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

#include <amd64/include/param.h>
#include <amd64/include/pmap.h>

uint64_t 	ptp_masks[] = PTP_MASK_INITIALIZER;
uint64_t 	ptp_shifts[] = PTP_SHIFT_INITIALIZER;
long 		NBPD[] = NBPD_INITIALIZER;
pd_entry_t 	*NPDE[] = PDES_INITIALIZER;
pd_entry_t 	*APDE[] = APDES_INITIALIZER;
long 		NKPTP[] = NKPTP_INITIALIZER;
long 		NKPTPMAX[] = NKPTPMAX_INITIALIZER;

static int pmap_nx_enable = -1;		/* -1 = auto */

/*
 * Get PDEs and PTEs for user/kernel address space
 */
#define pdlvlshift(lvl)         ((((lvl) * PTP_SHIFT) + PGSHIFT) - PTP_SHIFT)
#define pmap_ptab(m, v)         (&((m)->pm_ptab[((vm_offset_t)(v) >> PGSHIFT)]))
//#define	pmap_pde(m, v, lvl)		(&((m)->pm_pdir[((vm_offset_t)(v) >> ((((lvl) * PTP_SHIFT) + PGSHIFT) - PTP_SHIFT))]))
#define pmap_pde(m, v, lvl)     (&((m)->pm_pdir[PL_I(v, lvl)]))

#define	PAT_INDEX_SIZE	8
static int 		pat_index[PAT_INDEX_SIZE];	/* cache mode to PAT index conversion */

vm_offset_t 	kernphys;					/* phys addr of kernel page tables */
vm_offset_t 	KERNend;					/* and the end */

vm_offset_t 	avail_start;				/* PA of first available physical page */
vm_offset_t		avail_end;					/* PA of last available physical page */
vm_offset_t		virtual_avail;  			/* VA of first avail page (after kernel bss)*/
vm_offset_t		virtual_end;				/* VA of last avail page (end of kernel AS) */
vm_offset_t		vm_first_phys;				/* PA of first managed page */
vm_offset_t		vm_last_phys;				/* PA just past last managed page */

bool_t			pmap_initialized = FALSE;	/* Has pmap_init completed? */
char			*pmap_attributes;			/* reference and modify bits */


static u_int64_t KPTphys;					/* phys addr of kernel level 1 */
static u_int64_t KPDphys;					/* phys addr of kernel level 2 */
static u_int64_t KPDPphys;					/* phys addr of kernel level 3 */
u_int64_t 		 KPML4phys;					/* phys addr of kernel level 4 */

/* linked list of all non-kernel pmaps */
struct pmap	kernel_pmap_store;
static struct pmap_list pmap_header;

static pd_entry_t 	*pmap_valid_entry(pmap_t, pd_entry_t *, vm_offset_t);
static pml4_entry_t *pmap_pml4(pmap_t, vm_offset_t);
static pdp_entry_t 	*pmap_pdp(pmap_t, vm_offset_t);
static pd_entry_t 	*pmap_ptp(pmap_t, vm_offset_t);
static pt_entry_t 	*pmap_pte(pmap_t, vm_offset_t);

pt_entry_t *
pmap_vtopte(va)
	vm_offset_t va;
{
	KASSERT(va < (L4_SLOT_KERN * NBPD_L4));

	return (PTE_BASE + PL1_I(va));
}

pt_entry_t *
pmap_kvtopte(va)
	vm_offset_t va;
{
	KASSERT(va >= (L4_SLOT_KERN * NBPD_L4));

	return (PTE_BASE + PL1_I(va));
}

pt_entry_t *
pmap_avtopte(va)
	vm_offset_t va;
{
	KASSERT(va < (VA_SIGN_NEG(L4_SLOT_KERNBASE * NBPD_L4)));

	return (APTE_BASE + PL1_I(va));
}

__inline static void
pmap_apte_flush(pmap)
	pmap_t pmap;
{
#ifdef SMP
	struct pmap_tlb_shootdown_q *pq;
	struct cpu_info *ci, *self;
	CPU_INFO_ITERATOR cii;
	int s;

	self = curcpu();
#endif

	tlbflush();		/* flush TLB on current processor */

#ifdef SMP
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == self) {
			continue;
		}

		if (pmap == kernel_pmap || pmap->pm_active == ci->cpu_cpumask) {
			pq = &pmap_tlb_shootdown_q[ci->cpu_cpuid];
			s = splipi();

#ifdef SMP
			cpu_lock();
#endif
			pq->pq_flushu++;
#ifdef SMP
			cpu_unlock();
#endif
			splx(s);
			i386_send_ipi(ci, I386_IPI_TLB);
		}
	}
#endif
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
			if (!(opde & PG_V) || opde != pmap->pm_pdirpa) {
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
				if (!(opde & PG_V) || opde != pmap->pm_pdirpa) {
					npde = (pd_entry_t)(pmap->pm_pdirpa | PG_RW | PG_V | PG_A | PG_M);
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

static uint64_t
allocpages(firstaddr, n)
	vm_offset_t *firstaddr;
	long n;
{
	uint64_t ret;

	ret = *firstaddr;
	bzero((void *)ret, n * PAGE_SIZE);
	*firstaddr += n * PAGE_SIZE;
	return (ret);
}

#define NKPML4E				PTP_LEVELS
#define	PML4PML4I			(NPML4EPG / 2)		/* Index of recursive pml4 mapping */
#define	KPML4BASE			(NPML4EPG-NKPML4E) 	/* KVM at highest addresses */
#define	NKPDPE(ptpgs)		howmany(ptpgs, L4_SLOT_KERNBASE+1) /* 512 slots */
#define	KPML4I				(NPML4EPG-1)
#define	KPDPI				(NPDPEPG-2)

static void
create_pagetables(firstaddr)
	vm_offset_t *firstaddr;
{
	pd_entry_t *pd_p;
	pdp_entry_t *pdp_p;
	pml4_entry_t *p4_p;
	long nkpt, nkpd, nkpdpe, pt_pages;
	vm_offset_t pax;
	int i, j;

	/* Allocate pages. */
	KPML4phys = allocpages(firstaddr, 1);			/* recursive PML4 map */
	KPDPphys = allocpages(firstaddr, NKPML4E);		/* kernel PDP pages */

	pt_pages = howmany(firstaddr - kernphys, NBPDR) + 1;
	pt_pages += NKPDPE(pt_pages);
	pt_pages += 32;
	nkpt = pt_pages;
	nkpdpe = NKPDPE(nkpt);

	KPTphys = allocpages(firstaddr, nkpt);			/* KVA start */
	KPDphys = allocpages(firstaddr, nkpdpe);		/* kernel PD pages */

	pd_p = (pd_entry_t *)KPDphys;
	for (i = 0; i < nkpt; i++) {
		pd_p[i] = (KPTphys + ptoa(i)) | PG_RW | PG_V;
	}

	pd_p[0] = PG_V | PG_PS | pg_g | PG_M | PG_A | PG_RW | pg_nx;
	for (i = 1, pax = kernphys; pax < KERNend; i++, pax += NBPDR) {
		pd_p[i] = pax | PG_V | PG_PS | pg_g | PG_M | PG_A | bootaddr_rwx(pax);
	}

	pdp_p = (pdp_entry_t *)(KPDPphys + ptoa(KPML4I - KPML4BASE));
	for (i = 0; i < nkpdpe; i++) {
		pdp_p[i + KPDPI] = (KPDphys + ptoa(i)) | PG_RW | PG_V;
	}

	/* And recursively map PML4 to itself in order to get PTmap */
	p4_p = (pml4_entry_t *)KPML4phys;
	p4_p[PML4PML4I] = KPML4phys;
	p4_p[PML4PML4I] |= PG_RW | PG_V | pg_nx;

	/* Connect the KVA slots up to the PML4 */
	for (i = 0; i < NKPML4E; i++) {
		p4_p[KPML4BASE + i] = KPDPphys + ptoa(i);
		p4_p[KPML4BASE + i] |= PG_RW | PG_V;
	}
}

void
pmap_bootstrap(firstaddr)
	vm_offset_t firstaddr;
{
	vm_offset_t va;
	pt_entry_t *pte;
	u_long res;
	int i;
	uint64_t cr4;

	res = atop(KERNend - (vm_offset_t)kernphys);
	avail_start = *firstaddr;
	virtual_avail = (vm_offset_t)firstaddr;

	create_pagetables(firstaddr);

	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Initialize protection array.
	 */
	amd64_protection_init();

	kernel_pmap->pm_pdir = (pd_entry_t *)(KERNBASE + IdlePTD);
	kernel_pmap->pm_ptab = (pt_entry_t *)(KERNBASE + IdlePTD + NBPG);
	kernel_pmap->pm_pml4 = (pml4_entry_t *)(KERNBASE + IdlePML4);
	pmap_lock_init(kernel_pmap, "kernel_pmap_lock");
	LIST_INIT(&pmap_header);
	kernel_pmap->pm_count = 1;
	kernel_pmap->pm_stats.resident_count = res;

#define	SYSMAP(c, p, v, n)	\
	v = (c)va; va += ((n)*PAGE_SIZE); p = pte; pte += (n);

	va = virtual_avail;
	pte = pmap_pte(kernel_pmap, va);


	virtual_avail = va;

	/*
	 * Initialize the PAT MSR.
	 * pmap_init_pat() clears and sets CR4_PGE, which, as a
	 * side-effect, invalidates stale PG_G TLB entries that might
	 * have been created in our pre-boot environment.
	 */
	pmap_init_pat();
}

/*
 * pmap_bootstrap_allocate: allocate vm space into bootstrap area.
 * va: is taken from virtual_avail.
 * size: is the size of the virtual address to allocate
 */
void *
pmap_bootstrap_alloc(size)
	u_long size;
{
	vm_offset_t va;
	int i;
	extern bool_t vm_page_startup_initialized;

	if (vm_page_startup_initialized) {
		panic("pmap_bootstrap_alloc: called after startup initialized");
	}

	size = round_page(size);

	for (i = 0; i < size; i += PAGE_SIZE) {
		va = pmap_map(va, avail_start, avail_start + PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE);
		avail_start += PAGE_SIZE;
	}
	bzero((caddr_t)va, size);
	return ((void *)va);
}

/*
 * Setup the PAT MSR.
 */
void
pmap_init_pat(void)
{
	uint64_t pat_msr;
	u_long cr0, cr4;
	int i;

	/* Bail if this CPU doesn't implement PAT. */
	if ((cpu_feature & CPUID_PAT) == 0)
		panic("no PAT??");

	/* Set default PAT index table. */
	for (i = 0; i < PAT_INDEX_SIZE; i++)
		pat_index[i] = -1;
	pat_index[PAT_WRITE_BACK] = 0;
	pat_index[PAT_WRITE_THROUGH] = 1;
	pat_index[PAT_UNCACHEABLE] = 3;
	pat_index[PAT_WRITE_COMBINING] = 6;
	pat_index[PAT_WRITE_PROTECTED] = 5;
	pat_index[PAT_UNCACHED] = 2;

	/*
	 * Initialize default PAT entries.
	 * Leave the indices 0-3 at the default of WB, WT, UC-, and UC.
	 * Program 5 and 6 as WP and WC.
	 *
	 * Leave 4 and 7 as WB and UC.  Note that a recursive page table
	 * mapping for a 2M page uses a PAT value with the bit 3 set due
	 * to its overload with PG_PS.
	 */
	pat_msr = PAT_VALUE(0, PAT_WRITE_BACK) |
	    PAT_VALUE(1, PAT_WRITE_THROUGH) |
	    PAT_VALUE(2, PAT_UNCACHED) |
	    PAT_VALUE(3, PAT_UNCACHEABLE) |
	    PAT_VALUE(4, PAT_WRITE_BACK) |
	    PAT_VALUE(5, PAT_WRITE_PROTECTED) |
	    PAT_VALUE(6, PAT_WRITE_COMBINING) |
	    PAT_VALUE(7, PAT_UNCACHEABLE);

	/* Disable PGE. */
	cr4 = rcr4();
	lcr4(cr4 & ~CR4_PGE);

	/* Disable caches (CD = 1, NW = 0). */
	cr0 = rcr0();
	lcr0((cr0 & ~CR0_NW) | CR0_CD);

	/* Flushes caches and TLBs. */
	wbinvd();
	invltlb();

	/* Update PAT and index table. */
	wrmsr(MSR_PAT, pat_msr);

	/* Flush caches and TLBs again. */
	wbinvd();
	invltlb();

	/* Restore caches and PGE. */
	lcr0(cr0);
	lcr4(cr4);
}

void
pmap_init(phys_start, phys_end)
	vm_offset_t	phys_start, phys_end;
{
	vm_offset_t addr;
	vm_size_t npg, s;
	int i;

	addr = (vm_offset_t) KERNBASE + KPTphys;
	vm_object_reference(kernel_object);
	vm_map_find(kernel_map, kernel_object, addr, &addr, 2 * NBPG, FALSE);

	npg = atop(phys_end - phys_start);
	s = (vm_size_t) (sizeof(struct pv_entry) * npg + npg);
	s = round_page(s);

	pv_table = (pv_entry_t)kmem_alloc(kernel_map, s);
	addr = (vm_offset_t) pv_table;
	addr += sizeof(struct pv_entry) * npg;
	pmap_attributes = (char *)addr;

	/*
	 * Now it is safe to enable pv_table recording.
	 */
	vm_first_phys = phys_start;
	vm_last_phys = phys_end;

	pmap_initialized = TRUE;
}

vm_offset_t
pmap_map(virt, start, end, prot)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int			prot;
{
	vm_offset_t va, sva;

	va = sva = virt;
	while (start < end) {
		pmap_enter(kernel_pmap, va, start, prot, FALSE);
		va += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	pmap_invalidate_range(kernel_pmap, sva, va);
	virt = va;
	return (sva);
}

pmap_t
pmap_create(size)
	vm_size_t	size;
{
	register pmap_t pmap;

	if (size) {
		return (NULL);
	}

	pmap = (pmap_t) malloc(sizeof(*pmap), M_VMPMAP, M_WAITOK);

	bzero(pmap, sizeof(*pmap));
	pmap_pinit(pmap);
	return (pmap);
}
void
pmap_pinit_pdir(pdir, pdirpa)
	pd_entry_t *pdir;
	vm_offset_t pdirpa;
{
	int npde, i;

	pdir = (pd_entry_t *)kmem_alloc(kernel_map, PDIR_SLOT_PTE * sizeof(pd_entry_t));
	pdirpa = pdir[PDIR_SLOT_PTE] & PG_FRAME;

	/* zero init area */
	bzero(pdir, PDIR_SLOT_PTE * sizeof(pd_entry_t));

	/* put in recursive PDE to map the PTEs */
	pdir[PDIR_SLOT_PTE] = pdirpa | PG_V | PG_KW;

	npde = NKPTP[PTP_LEVELS - 1];

	/* put in kernel VM PDEs */
	bcopy(&PDP_BASE[PDIR_SLOT_KERN], &pdir[PDIR_SLOT_KERN], npde * sizeof(pd_entry_t));

	/* install self-referential address mapping entry */
	for (i = 0; i < PDIR_SLOT_PTE - 1; i++) {
		pdir[i] = pmap_extract(kernel_pmap, (vm_offset_t)pdir[i]);
	}

	/* zero the rest */
	bzero(&pdir[PDIR_SLOT_KERN + npde], (NPDEPG - (PDIR_SLOT_KERN + npde)) * sizeof(pd_entry_t));
}

void
pmap_pinit_pml4(pml4)
	pml4_entry_t *pml4;
{
	int i;

	for (i = 0; i < PTP_LEVELS; i++) {
		pml4[L4_SLOT_KERNBASE + i] =  (KPDPphys + ptoa(i)) | PG_RW | PG_V;
	}

	/* install self-referential address mapping entry(s) */
	pml4[L4_SLOT_KERN] = VM_PAGE_TO_PHYS(pml4pg) | PG_V | PG_RW | PG_A | PG_M;
}

void
pmap_pinit(pmap)
	register pmap_t pmap;
{
	pmap_pinit_pdir(pmap->pm_pdir, pmap->pm_pdirpa);
	pmap_pinit_pml4(pmap->pm_pml4);

	pmap->pm_active = 0;
	pmap->pm_count = 1;
	pmap_lock_init(pmap, "pmap_lock");
	bzero(&pmap->pm_stats, sizeof(pmap->pm_stats));

	pmap_lock(pmap);
	LIST_INSERT_HEAD(&pmap_header, pmap, pm_list);
	pmap_unlock(pmap);
}

void
pmap_destroy(pmap)
	register pmap_t pmap;
{
	if (pmap == NULL) {
		return;
	}

	pmap_lock(pmap);
	LIST_REMOVE(pmap, pm_list);
	count = --pmap->pm_count;
	pmap_unlock(pmap);
	if (count == 0) {
		pmap_release(pmap);
		free((caddr_t)pmap, M_VMPMAP);
	}
}

void
pmap_release(pmap)
	register pmap_t pmap;
{

}

void
pmap_reference(pmap)
	pmap_t	pmap;
{
	if (pmap != NULL) {
		pmap_lock(pmap);
		pmap->pm_count++;
		pmap_unlock(pmap);
	}
}

void
pmap_remove(pmap, sva, eva)
	register pmap_t pmap;
	vm_offset_t sva, eva;
{
	register vm_offset_t pa, va;
	register pt_entry_t *pte;

	if (pmap == NULL) {
		return;
	}

	for (va = sva; va < eva; va += PAGE_SIZE) {

	}
}

void
pmap_remove_all(pa)
	vm_offset_t pa;
{

}

void
pmap_copy_on_write(pa)
	vm_offset_t pa;
{

}

void
pmap_protect(pmap, sva, eva, prot)
	register pmap_t	pmap;
	vm_offset_t	sva, eva;
	vm_prot_t	prot;
{

}

void
pmap_enter(pmap, va, pa, prot, wired)
	register pmap_t pmap;
	vm_offset_t va;
	register vm_offset_t pa;
	vm_prot_t prot;
	bool_t wired;
{

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

static pml4_entry_t *
pmap_pml4(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pml4_entry_t *pml4;

	pml4 = (pml4_entry_t *)pmap_pde(pmap, va, 4);
	if (pmap_valid_entry(pmap, (pml4_entry_t *)pml4, va)) {
		return (pml4);
	}
	return (NULL);
}

static pdp_entry_t *
pmap_pdp(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pdp_entry_t *pdp;

	pdp = (pdp_entry_t *)pmap_pde(pmap, va, 3);
	if (pmap_valid_entry(pmap, (pdp_entry_t *)pdp, va)) {
		return (pdp);
	}
	return (NULL);
}

static pd_entry_t *
pmap_ptp(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pd_entry_t *ptp;

	ptp = (pd_entry_t *)pmap_pde(pmap, va, 2);
	if (pmap_valid_entry(pmap, ptp, va)) {
		return (ptp);
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

void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t		dst_pmap;
	pmap_t		src_pmap;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
{

}

void
pmap_update(void)
{

}

void
pmap_collect(pmap)
	pmap_t	pmap;
{

}

void
pmap_activate(pmap, pcbp)
	register pmap_t pmap;
	struct pcb *pcbp;
{
	if (pmap != NULL && pmap->pm_pdchanged) {
		pcbp->pcb_cr3 = pmap_extract(kernel_pmap, (vm_offset_t)pmap->pm_pdir); /* PML4?? */
		if (pmap == &curproc->p_vmspace->vm_pmap) {
			lcr3(pcbp->pcb_cr3);
		}
		pmap->pm_pdchanged = FALSE;
	}
}

void
pmap_zero_page(phys)
	register vm_offset_t phys;
{

}

void
pmap_copy_page(src, dst)
	register vm_offset_t src, dst;
{

}

void
pmap_pageable(pmap, sva, eva, pageable)
	pmap_t pmap;
	vm_offset_t	sva, eva;
	bool_t	pageable;
{
	/*
	 * If we are making a PT page pageable then all valid
	 * mappings must be gone from that page.  Hence it should
	 * be all zeros and there is no need to clean it.
	 * Assumptions:
	 *	- we are called with only one page at a time
	 *	- PT pages have only one pv_table entry
	 */
	if (pmap == kernel_pmap && pageable && sva + PAGE_SIZE == eva) {
		register pv_entry_t pv;
		register vm_offset_t pa;
	}
}

void
pmap_clear_modify(pa)
	vm_offset_t	pa;
{

}

void
pmap_clear_reference(pa)
	vm_offset_t	pa;
{

}

bool_t
pmap_is_referenced(pa)
	vm_offset_t	pa;
{

}

bool_t
pmap_is_modified(pa)
	vm_offset_t	pa;
{

}

void
amd64_protection_init(void)
{
	uint64_t *kp;
	int prot;

	kp = protection_codes;
	for (prot = 0; prot < 8; prot++) {
		switch (prot) {
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE:
			*kp++ = 0;
			break;
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE:
			if (pmap_nx_enable >= 1) {
				*kp++ = PG_NX;
			}
			break;
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE:
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE:
			*kp++ = PG_RO;
			break;
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE:
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE:
			*kp++ = PG_RW;
			if (pmap_nx_enable >= 2) {
				*kp++ |= PG_NX;
			}
			break;
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE:
			*kp++ = PG_RW;
			break;
		}
	}
}

bool_t
pmap_testbit(pa, bit)
	register vm_offset_t pa;
	int bit;
{

}

void
pmap_changebit(pa, bit, setem)
	register vm_offset_t pa;
	int bit;
	bool_t setem;
{

}
