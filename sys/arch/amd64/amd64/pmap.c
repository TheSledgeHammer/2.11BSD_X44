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

#include <amd64/include/pte.h>

#include <amd64/include/vmparam.h>

#include <amd64/include/param.h>
#include <amd64/include/pmap.h>

/*
 * CPUID instruction 7 Structured Extended Features, leaf 0 ecx info
 */
#define	CPUID_STDEXT2_LA57		0x00010000
#define	CR4_LA57                0x00001000	/* Enable 5-level paging */

uint64_t 	ptp_masks[] = PTP_MASK_INITIALIZER;
uint64_t 	ptp_shifts[] = PTP_SHIFT_INITIALIZER;
pd_entry_t 	*NPDE[] = PDES_INITIALIZER;
pd_entry_t 	*APDE[] = APDES_INITIALIZER;

pt_entry_t pg_nx;
static pt_entry_t pg_g;
static int pg_ps_enabled = 1;
static int pmap_nx_enable = -1;		/* -1 = auto */
int la57 = 0;

static int ndmpdp;
static vm_offset_t dmaplimit;

/*
 * Get PDEs and PTEs for user/kernel address space
 */
#define pmap_pdir(m, v, lvl)     (&((m)->pm_pdir[PL_I(v, lvl)]))

#define	PAT_INDEX_SIZE	8
static int 		pat_index[PAT_INDEX_SIZE];	/* cache mode to PAT index conversion */

vm_offset_t 	kernphys;					/* phys addr of kernel page tables */

vm_offset_t 	kernel_vm_end;
vm_offset_t		physfree;					/* phys addr of next free page */
vm_offset_t		KERNend;					/* and the end */

vm_offset_t 	avail_start;				/* PA of first available physical page */
vm_offset_t		avail_end;					/* PA of last available physical page */
vm_offset_t		virtual_avail;  			/* VA of first avail page (after kernel bss)*/
vm_offset_t		virtual_end;				/* VA of last avail page (end of kernel AS) */
vm_offset_t		vm_first_phys;				/* PA of first managed page */
vm_offset_t		vm_last_phys;				/* PA just past last managed page */

bool_t			pmap_initialized = FALSE;	/* Has pmap_init completed? */

static uint64_t  KPTphys;					/* phys addr of kernel level 1 */
static uint64_t  KPDphys;					/* phys addr of kernel level 2 */
static uint64_t  KPDPTphys;					/* phys addr of kernel level 3 */
uint64_t 		 KPML4phys;					/* phys addr of kernel level 4 */
uint64_t 		 KPML5phys;					/* phys addr of kernel level 5, if supported */

static uint64_t	 DMPDphys;					/* phys addr of direct mapped level 2 */
static uint64_t	 DMPDPTphys;				/* phys addr of direct mapped level 3 */

/* Idle Maps */
pt_entry_t 		*KPTmap;					/* address of kernel page tables */
pd_entry_t 		*IdlePTD;
pdpt_entry_t 	*IdlePDPT;
pml4_entry_t 	*IdlePML4;
pml5_entry_t 	*IdlePML5;

/* LA57 */
extern const char la57_trampoline[], la57_trampoline_gdt_desc[], la57_trampoline_gdt[], la57_trampoline_end[];
static bool_t pmap_is_la57(pmap_t);
static void pmap_cold_la57(vm_offset_t);
static void create_pagetables_la57(vm_offset_t);
static void pmap_enable_la57(uint64_t);
static int 	pmap_check_la57(void);
static void pmap_bootstrap_la57(vm_offset_t);

/* linked list of all non-kernel pmaps */
struct pmap	kernel_pmap_store;
static struct pmap_list pmap_header;


static pd_entry_t 	*pmap_valid_entry(pmap_t, pd_entry_t *, vm_offset_t);
static pd_entry_t 	*pmap_table(pmap_t, vm_offset_t, int);
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

vm_offset_t
pmap_pdirpa(pmap, index)
	register pmap_t pmap;
	unsigned long index;
{
	if (pmap_is_la57(pmap)) {
		return (pmap_pdirpa_la57(pmap, index));
	}

	return (pmap_pdirpa_la48(pmap, index));
}

static __inline bool_t
pmap_emulate_ad_bits(pmap_t pmap)
{
	return ((pmap->pm_flags & PMAP_EMULATE_AD_BITS) != 0);
}

static __inline pt_entry_t
pmap_valid_bit(pmap_t pmap)
{
	pt_entry_t mask;

	switch (pmap->pm_type) {
	case PT_X86:
	case PT_RVI:
		mask = PG_V;
		break;
	case PT_EPT:
		if (pmap_emulate_ad_bits(pmap))
			mask = EPT_PG_EMUL_V;
		else
			mask = EPT_PG_READ;
		break;
	default:
		panic("pmap_valid_bit: invalid pm_type %d", pmap->pm_type);
	}

	return (mask);
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
	pd_entry_t opde;

	if (pmap && pmap_pde_v(pmap_pdir(pmap, va, 1))) {
		/* are we current address space or kernel? */
		if ((pmap->pm_pdir >= PTE_BASE && pmap->pm_pdir < L2_BASE)) {
			pde = vtopte(va);
			return (pde);
		} else {
			/* otherwise, we are alternate address space */
			if (pmap->pm_pdir >= APTE_BASE && pmap->pm_pdir < AL2_BASE) {
				apde = avtopte(va);
				tlbflush();
			}
			opde = *APDP_PDE & PG_FRAME;
			if (!(opde & PG_V) || opde != pmap_pdirpa(pmap, 0)) {
				apde = (pd_entry_t *)(pmap_pdirpa(pmap, 0) | PG_RW | PG_V | PG_A | PG_M);
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
	pd_entry_t opde;
	unsigned long index;
	int i;

	for (i = PTP_LEVELS; i > 1; i--) {
		index = PL_I(va, i);
		if (pmap && pmap_pde_v(pmap_pdir(pmap, va, i))) {
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
					apde = (pd_entry_t *)(pmap_pdirpa(pmap, index) | PG_RW | PG_V | PG_A | PG_M);
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
allocpages(cnt, physfree)
	u_long cnt;
	vm_offset_t *physfree;
{
	uint64_t res;

	res = *physfree;
	*physfree += (PGSIZE * cnt);
	bzero((void *)res, (PGSIZE * cnt));
	return (res);
}

void
pmap_cold(void)
{
	physfree = (vm_offset_t)&_end;
	physfree = roundup(physfree, NBPDR);
	KERNend = physfree;

	/* Allocate Kernel Page Tables */
    KPTphys = allocpages(NKPT, &physfree);
    KPDphys = allocpages(NKPDPE, &physfree);
    KPDPTphys = allocpages(1, &physfree);
    KPML4phys = allocpages(1, &physfree);

    pmap_cold_la57(physfree);
}

void
create_pagetables(firstaddr)
	vm_offset_t firstaddr;
{
	int i;

	ndmpdp = (ptoa(Maxmem) + NBPD_L3 - 1) >> L2_SHIFT;
	if (ndmpdp < 4) {		/* Minimum 4GB of dirmap */
		ndmpdp = 4;
	}

	DMPDPTphys = allocpages(L4_DMAP_SLOTS, &physfree);
	DMPDphys = allocpages(ndmpdp, &physfree);
	dmaplimit = (vm_offset_t)ndmpdp << L3_SHIFT;

    KPTmap = (pt_entry_t *)KPTphys;
    IdlePTD = (pd_entry_t *)KPDphys;
    IdlePDPT = (pdpt_entry_t *)KPDPTphys;
    IdlePML4 = (pml4_entry_t *)KPML4phys;

    for (i = 0; ptoa(i) < firstaddr; i++) {
		KPTphys[i] = ptoa(i);
        KPTphys[i] |= PG_RW | PG_V | PG_G;
	}

    /*
	 * Connect the zero-filled PT pages to their PD entries.  This
	 * implicitly maps the PT pages at their correct locations within
	 * the PTmap.
	 */
    for (i = 0; i < NKPT; i++) {
        IdlePTD[i] = (KPTphys + ptoa(i));
        IdlePTD[i] |= PG_RW | PG_V | PG_G;
    }

    for (i = 0; (i << PD_SHIFT) < firstaddr; i++) {
        IdlePTD[i] = (i << PD_SHIFT);
        IdlePTD[i] |= PG_RW | PG_V | PG_PS | PG_G;
    }

    /* And connect up the PD to the PDPT */
    for (i = 0; i < NKPDPE; i++) {
        IdlePDPT[i + PDIR_SLOT_APTE] = (IdlePTD + ptoa(i));
        IdlePDPT[i + PDIR_SLOT_APTE] |= PG_RW | PG_V | PG_U;
    }

	/* Now set up the direct map space using 2MB pages */
	for (i = 0; i < NPDEPG * ndmpdp; i++) {
		DMPDphys[i] = (vm_offset_t)i << L3_SHIFT;
		DMPDphys[i] |= PG_RW | PG_V | PG_PS | PG_G;
	}

	/* And the direct map space's PDP */
	for (i = 0; i < ndmpdp; i++) {
		DMPDPTphys[i] = DMPDphys + (i << L1_SHIFT);
		DMPDPTphys[i] |= PG_RW | PG_V | PG_U;
	}

    /* And recursively map PML4 to itself in order to get PTmap */
    IdlePML4[PDIR_SLOT_KERN] = KPML4phys;
	IdlePML4[PDIR_SLOT_KERN] |= PG_RW | PG_V | pg_nx;

	/* Connect the Direct Map slot up to the PML4 */
	KPML4phys[PDIR_SLOT_DIRECT] = DMPDPTphys;
	KPML4phys[PDIR_SLOT_DIRECT] |= PG_RW | PG_V | PG_U;

    /* Connect the KVA slots up to the PML4 */
	for (i = 0; i < NKL4_MAX_ENTRIES; i++) {
		IdlePML4[PDIR_SLOT_KERNBASE + i] = (IdlePDPT + ptoa(i));
		IdlePML4[PDIR_SLOT_KERNBASE + i] |= PG_RW | PG_V;
	}
}

/*
 * Enable PG_G global pages, then switch to the kernel page
 * table from the bootstrap page table.  After the switch, it
 * is possible to enable SMEP and SMAP since PG_U bits are
 * correct now.
 */
static void
pmap_enable_pg_g(cr4)
	uint64_t cr4;
{
	cr4 = rcr4();
	cr4 |= CR4_PGE;
	load_cr4(cr4);
	load_cr3(KPML4phys);
	if (cpu_stdext_feature & CPUID_STDEXT_SMEP) {
		cr4 |= CR4_SMEP;
	}
	if (cpu_stdext_feature & CPUID_STDEXT_SMAP) {
		cr4 |= CR4_SMAP;
	}
	load_cr4(cr4);
}

void
pmap_bootstrap(firstaddr)
	vm_offset_t firstaddr;
{
	vm_offset_t va;
	pt_entry_t *pte;
	u_long res;
	uint64_t cr4;
	int i;

	//res = atop(KERNend - (vm_offset_t)kernphys);
	res = atop(firstaddr - (vm_offset_t)KERNLOAD);

	avail_start = *firstaddr;
	virtual_avail = (vm_offset_t)firstaddr;
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	create_pagetables(firstaddr);

	if (pmap_check_la57()) {
		create_pagetables_la57(firstaddr);
		pmap_bootstrap_la57(firstaddr);
		pmap_enable_pg_g(cr4);
		pmap_enable_la57(cr4);

		/*
		 * Initialize protection array.
		 */
		amd64_protection_init();

		kernel_pmap->pm_pdir = (pd_entry_t *)(KERNBASE + IdlePTD);
		kernel_pmap->pm_pml4 = (pml4_entry_t *)(KERNBASE + IdlePML4);
		kernel_pmap->pm_pml5 = (pml5_entry_t *)(KERNBASE + IdlePML5);
		kernel_pmap->pm_pdirpa[0] = (vm_offset_t)IdlePTD;

	} else {
		pmap_enable_pg_g(cr4);

		/*
		 * Initialize protection array.
		 */
		amd64_protection_init();

		kernel_pmap->pm_pdir = (pd_entry_t *)(KERNBASE + IdlePTD);
		kernel_pmap->pm_pml4 = (pml4_entry_t *)(KERNBASE + IdlePML4);
		kernel_pmap->pm_pdirpa[0] = (vm_offset_t)IdlePTD;
	}
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
	 * Initialize the TLB shootdown queues.
	 */
	pmap_tlb_init();

	/*
	 * Initialize the PAT MSR.
	 * pmap_init_pat() clears and sets CR4_PGE, which, as a
	 * side-effect, invalidates stale PG_G TLB entries that might
	 * have been created in our pre-boot environment.
	 */
	pmap_init_pat();
}

/*
 * pmap_bootstrap_alloc: allocate vm space into bootstrap area.
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
	vm_size_t pvsize, size, npg;
	int i;

	addr = (vm_offset_t)KERNBASE + KPTphys;
	vm_object_reference(kernel_object);
	vm_map_find(kernel_map, kernel_object, addr, &addr, 2 * NBPG, FALSE);

	pvsize = (vm_size_t)(sizeof(struct pv_entry));
	npg = atop(phys_end - phys_start);
	size = (pvsize * npg + npg);
	size = round_page(size);

	pv_table = (pv_entry_t)kmem_alloc(kernel_map, size);
	addr = (vm_offset_t) pv_table;
	addr += sizeof(struct pv_entry) * npg;
	pv_table->pv_attr = (char)addr;

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

	pmap = (pmap_t)malloc(sizeof(*pmap), M_VMPMAP, M_WAITOK);

	bzero(pmap, sizeof(*pmap));
	pmap_pinit(pmap);
	return (pmap);
}

void
pmap_pinit_pdir(pdir, pdirpa)
	pd_entry_t *pdir;
	vm_offset_t pdirpa;
{
	int i;

	pdir = (pd_entry_t *)kmem_alloc(kernel_map, NBPTD);
	pdirpa = pdir[PDIR_SLOT_PTE] & PG_FRAME;

	/* zero init area */
	bzero(pdir, PDIR_SLOT_PTE * sizeof(pd_entry_t));

	/* put in recursive PDE to map the PTEs */
	pdir[PDIR_SLOT_PTE] = pdirpa | PG_V | PG_KW;

	/* put in kernel VM PDEs */
	bcopy(&PDP_BASE[PDIR_SLOT_KERN], &pdir[PDIR_SLOT_KERN], sizeof(pd_entry_t));

	/* install self-referential address mapping entry */
	for (i = 0; i < PDIR_SLOT_PTE - 1; i++) {
		pdir[i] = pmap_extract(kernel_pmap, (vm_offset_t)pdir[i]);
	}

	/* zero the rest */
	bzero(&pdir[PDIR_SLOT_KERN], (NPDEPG - (PDIR_SLOT_KERN)) * sizeof(pd_entry_t));
}

void
pmap_pinit_pml4(pml4)
	pml4_entry_t *pml4;
{
	int i;

	pml4 = (pml4_entry_t *)kmem_alloc(kernel_map, (vm_offset_t)(NKL4_MAX_ENTRIES * sizeof(pml4_entry_t)));

	for (i = 0; i < NKL4_MAX_ENTRIES; i++) {
		pml4[PDIR_SLOT_KERNBASE + i] = pmap_extract(kernel_map, (vm_offset_t)(IdlePDPT + ptoa(i))) | PG_RW | PG_V;
	}

	/* install self-referential address mapping entry(s) */
	pml4[L4_SLOT_KERN] = pmap_extract(kernel_map, (vm_offset_t)pml4) | PG_RW | PG_V | PG_A | PG_M;
}

void
pmap_pinit_pml5(pml5)
	pml5_entry_t *pml5;
{
	int i;

	pml5 = (pml5_entry_t *)kmem_alloc(kernel_map, (vm_offset_t)(NKL5_MAX_ENTRIES * sizeof(pml5_entry_t)));

	/*
	 * Add pml5 entry at top of KVA pointing to existing pml4 table,
	 * entering all existing kernel mappings into level 5 table.
	 */
	for (i = 0; i < NKL5_MAX_ENTRIES; i++) {
		pml5[PDIR_SLOT_KERNBASE + i] = pmap_extract(kernel_map, (vm_offset_t)(IdlePML4 + ptoa(i))) | PG_V | PG_RW | PG_A | PG_M | pg_g;
	}

	/* install self-referential address mapping entry(s) */
	pml5[PDIR_SLOT_KERN] = pmap_extract(kernel_map, (vm_offset_t)pml5) | PG_RW | PG_V | PG_A | PG_M;
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
void
pmap_pinit_type(pmap, pm_type)
	pmap_t pmap;
	enum pmap_type pm_type;
{
	pmap->pm_type = pm_type;

	pmap_pinit_pdir(pmap->pm_pdir, pmap->pm_pdirpa);

	switch (pm_type) {
	case PT_X86:
		if (pmap_is_la57(pmap)) {
			pmap_pinit_pml5(pmap->pm_pml5);
		} else {
			pmap_pinit_pml4(pmap->pm_pml4);
		}
		break;

	case PT_EPT:
	case PT_RVI:
	}
}

void
pmap_pinit(pmap)
	register pmap_t pmap;
{
	pmap_pinit_type(pmap, PT_X86);

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
	int i;

	kmem_free(kernel_map, (vm_offset_t)pmap->pm_pdir, NBPTD);

	if (pmap_is_la57(pmap)) {
		kmem_free(kernel_map, (vm_offset_t)pmap->pm_pml5, NBPTD * sizeof(pml5_entry_t));
	} else {
		kmem_free(kernel_map, (vm_offset_t)pmap->pm_pml4, NBPTD * sizeof(pml4_entry_t));
	}
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
	int anyvalid;

	if (pmap == NULL) {
		return;
	}

	anyvalid = 0;
	for (va = sva; va < eva; va += PAGE_SIZE) {

		if (!pmap_pde_v(pmap_map_pde(pmap, va))) {
			continue;
		}

		pte = pmap_pte(pmap, va);
		if (pte == 0) {
			continue;
		} else {
			anyvalid = 1;
		}

		pa = pmap_pte_pa(pte);
		if (pa == 0) {
			continue;
		} else {
			anyvalid = 1;
		}

		/*
		 * Update statistics
		 */
		if (pmap_pte_w(pte)) {
			pmap->pm_stats.wired_count--;
		}
		pmap->pm_stats.resident_count--;
		pmap_invalidate_page(pmap, va);
	}
	if (anyvalid) {
		pmap_invalidate_all(pmap);
	}
}

void
pmap_remove_all(pa)
	vm_offset_t pa;
{
	register pv_entry_t pv;
	int s;

	/*
	 * Not one of ours
	 */
	if (pa < vm_first_phys || pa >= vm_last_phys) {
		return;
	}

	pv = pa_to_pvh(pa);
	s = splimp();

	/*
	 * Do it the easy way for now
	 */
	while (pv->pv_pmap != NULL) {
		pmap_invalidate_page(pv->pv_pmap, pv->pv_va);
		pmap_remove(pv->pv_pmap, pv->pv_va, pv->pv_va + PAGE_SIZE);
	}
	splx(s);
}

void
pmap_copy_on_write(pa)
	vm_offset_t pa;
{
	pmap_changebit(pa, PG_RO, TRUE);
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

void
pmap_page_protect(phys, prot)
	vm_offset_t     phys;
    vm_prot_t       prot;
{
	switch (prot) {
	case VM_PROT_READ:
	case VM_PROT_READ | VM_PROT_EXECUTE:
		pmap_copy_on_write(phys);
		break;
	case VM_PROT_ALL:
		break;
	default:
		pmap_remove_all(phys);
		break;
	}
}

void
pmap_change_wiring(pmap, va, wired)
	register pmap_t	pmap;
	vm_offset_t	va;
	bool_t	wired;
{
	register pt_entry_t *pte;
	register int ix;

	if (pmap == NULL) {
		return;
	}

	pte = pmap_pte(pmap, va);
	if ((wired && !pmap_pte_w(pte)) || (!wired && pmap_pte_w(pte))) {
		if (wired) {
			pmap->pm_stats.wired_count++;
		} else {
			pmap->pm_stats.wired_count--;
		}
	}
	/*
	 * Wiring is not a hardware characteristic so there is no need
	 * to invalidate TLB.
	 */
	ix = 0;
	do {
		pmap_pte_set_w(pte++, wired);
	} while (++ix != 1);
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

static pd_entry_t *
pmap_table(pmap, va, lvl)
	register pmap_t pmap;
	vm_offset_t va;
	int lvl;
{
	pd_entry_t *ptab;

	ptab = pmap_pdir(pmap, va, lvl);
	if (pmap_valid_entry(pmap, ptab, va)) {
		return (ptab);
	}
	return (NULL);
}

pml4_entry_t *
pmap_pml4e(pmap_t pmap, vm_offset_t va)
{
	pml4_entry_t *pml4;

	pml4 = pmap_table(pmap, va, 4);
	return (&pml4[PL4_E(va)]);
}

pdpt_entry_t *
pmap_pdpt(pmap_t pmap, vm_offset_t va)
{
	pml4_entry_t *pml4e;
	pdpt_entry_t *pdpt;

	pml4e = pmap_pml4e(pmap, va);
	if (pml4e == NULL) {
		return (NULL);
	}
	pdpt = (pdpt_entry_t *)PHYS_TO_DMAP(*pml4e & PG_FRAME);
	return (&pdpt[PL3_E(va)]);
}

pd_entry_t *
pmap_pde(pmap_t pmap, vm_offset_t va)
{
	pdpt_entry_t *pdpt;
	pd_entry_t *pde;

	pdpt = pmap_pdpt(pmap, va);
	if (pdpt == NULL) {
		return (NULL);
	}
	pde = (pd_entry_t *)PHYS_TO_DMAP(*pdpt & PG_FRAME);
	return (&pde[PL2_E(va)]);
}

/*
 *	Routine:	pmap_pte
 *	Function:
 *		Extract the page table entry associated
 *		with the given map/virtual_address pair.
 * [ what about induced faults -wfj]
 */

pt_entry_t *
pmap_pte(pmap_t pmap, vm_offset_t va)
{
	pd_entry_t *pde;
	pt_entry_t *pte;

	pde = pmap_pde(pmap, va);
	if (pde == NULL) {
		return (NULL);
	}
	if ((*pde & PG_PS) != 0) {
		return ((pt_entry_t *)pde);
	}
	pte = (pt_entry_t *)PHYS_TO_DMAP(*pde & PG_FRAME);
	return (&pte[PL1_E(va)]);
}

pt_entry_t *
pmap_pte_pde(pmap_t pmap, vm_offset_t va, pd_entry_t *ptepde)
{
	pd_entry_t *pde;
	pt_entry_t *pte;

	pde = pmap_pde(pmap, va);
	if (pde == NULL) {
		return NULL;
	}
	*ptepde = *pde;
	if ((*pde & PG_PS) != 0) {	/* compat with i386 pmap_pte() */
		return ((pt_entry_t *)pde);
	}
	pte = (pt_entry_t *)PHYS_TO_DMAP(*pde & PG_FRAME);
	return (&pte[PL1_E(va)]);
}

pml4_entry_t *
pmap_pml5e_to_pml4e(pml5_entry_t *pml5e, vm_offset_t va)
{
	pml4_entry_t *pml4e;

	/* XXX MPASS(pmap_is_la57(pmap); */
	pml4e = (pml4_entry_t *)PHYS_TO_DMAP(*pml5e & PG_FRAME);
	return (&pml4e[PL4_E(va)]);
}

pml5_entry_t *
pmap_pml4e_to_pml5e(pmap_t pmap, vm_offset_t va)
{
	pml4_entry_t *pml4;
	pml5_entry_t *pml5;

	pml4 = pmap_pml4e(pmap, va);
	if (pmap_is_la57(pmap)) {
		pml5 = &pml4[l4etol5e(PL4_E(va))];
		PG_V = pmap_valid_bit(pmap);
	}

	return (pml5);
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
	register vm_offset_t pa;
	register pv_entry_t pv;
	register int *pte;
	vm_offset_t kpa;
	int s;

	if (pmap != kernel_pmap) {
		return;
	}

	s = splimp();
	for (pa = vm_first_phys; pa < vm_last_phys; pa += PAGE_SIZE) {
		/*
		 * Locate physical pages which are being used as kernel
		 * page table pages.
		 */
		pv = pa_to_pvh(pa);
		if (pv->pv_pmap != kernel_pmap || !(pv->pv_flags & PG_PTPAGE)) {
			continue;
		}
		do {
			if (pv->pv_pmap == kernel_pmap) {
				break;
			}
		} while (pv == pv->pv_next);
		if (pv == NULL) {
			continue;
		}
		pte = (int *)(pv->pv_va + PAGE_SIZE);
		while (--pte >= (int*) pv->pv_va /* && *pte == PG_NV */)
			;
		if (pte >= (int*) pv->pv_va) {
			continue;
		}
		kpa = pmap_extract(pmap, pv->pv_va);
		pmap_remove_all(kpa);
	}
	splx(s);
}


void
pmap_activate(pmap, pcbp)
	register pmap_t pmap;
	struct pcb *pcbp;
{
	if (pmap != NULL && pmap->pm_pdchanged) {
		if (pmap_is_la57(pmap)) {
			pcbp->pcb_cr3 = pmap_extract(kernel_pmap, (vm_offset_t)pmap->pm_pml5); /* PML5?? */
		} else {
			pcbp->pcb_cr3 = pmap_extract(kernel_pmap, (vm_offset_t)pmap->pm_pml4); /* PML4?? */
		}
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
	pmap_t		pmap;
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

		if ((pmap_map_pte(pmap, sva) == 0)
				&& (pmap_map_pde(pmap, sva) == 0)) {
			return;
		}
		if (pmap_map_pte(pmap, sva) != 0) {
			pa = pmap_pte_pa(pmap_map_pte(pmap, sva));
			if (pa < vm_first_phys || pa >= vm_last_phys) {
				return;
			}
		} else if (pmap_map_pde(pmap, sva) != 0) {
			pa = pmap_pte_pa(pmap_map_pde(pmap, sva));
			if (pa < vm_first_phys || pa >= vm_last_phys) {
				return;
			}
		} else {
			return;
		}
		pv = pa_to_pvh(pa);
		/*
		 * Mark it unmodified to avoid pageout
		 */
		pmap_clear_modify(pa);
	}
}

void
pmap_clear_modify(pa)
	vm_offset_t	pa;
{
	pmap_changebit(pa, PG_M, FALSE);
}

void
pmap_clear_reference(pa)
	vm_offset_t	pa;
{
	pmap_changebit(pa, PG_U, FALSE);
}

bool_t
pmap_is_referenced(pa)
	vm_offset_t	pa;
{
	return (pmap_testbit(pa, PG_U));
}

bool_t
pmap_is_modified(pa)
	vm_offset_t	pa;
{
	return (pmap_testbit(pa, PG_M));
}


vm_offset_t
pmap_phys_address(ppn)
	unsigned long ppn;
{
	return (amd64_ptob(ppn));
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
	register pv_entry_t pv;
	register int *pte, ix;
	int s;

	if (pa < vm_first_phys || pa >= vm_last_phys)
		return (FALSE);

	pv = pa_to_pvh(pa);
	s = splimp();

	/*
	 * Check saved info first
	 */
	if (pv->pv_attr & bit) {
		splx(s);
		return (TRUE);
	}
	/*
	 * Not found, check current mappings returning
	 * immediately if found.
	 */
	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
			pte = (int*) pmap_pte(pv->pv_pmap, pv->pv_va);
			ix = 0;
			do {
				if (*pte++ & bit) {
					splx(s);
					return (TRUE);
				}
			} while (++ix != 1);
		}
	}
	splx(s);
	return (FALSE);
}

void
pmap_changebit(pa, bit, setem)
	register vm_offset_t pa;
	int bit;
	bool_t setem;
{
	register pv_entry_t pv;
	register int *pte, npte, ix;
	vm_offset_t va;
	int s;
	bool_t firstpage = TRUE;

	struct proc *p;

	p = curproc;

	if (pa < vm_first_phys || pa >= vm_last_phys) {
		return;
	}

	pv = pa_to_pvh(pa);
	s = splimp();
	/*
	 * Clear saved attributes (modify, reference)
	 */
	if (!setem) {
		pv->pv_attr &= ~bit;
	}
	/*
	 * Loop over all current mappings setting/clearing as appropos
	 * If setting RO do we need to clear the VAC?
	 */
	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
			va = pv->pv_va;

			/*
			 * XXX don't write protect pager mappings
			 */
			if (bit == PG_RO) {
				extern vm_offset_t pager_sva, pager_eva;

				if (va >= pager_sva && va < pager_eva) {
					continue;
				}
			}

			pte = (int*) pmap_pte(pv->pv_pmap, va);
			ix = 0;
			do {
				if (setem) {
					npte = *pte | bit;
					pmap_invalidate_page(pv->pv_pmap, pv->pv_va);
				} else {
					npte = *pte & ~bit;
					pmap_invalidate_page(pv->pv_pmap, pv->pv_va);
				}
				if (*pte != npte) {
					*pte = npte;
				}
				va += PAGE_SIZE;
				pte++;
			} while (++ix != 1);

			if (pv->pv_pmap == &p->p_vmspace->vm_pmap) {
				pmap_activate(pv->pv_pmap, (struct pcb*) &p->p_addr->u_pcb);
			}
		}
	}
	splx(s);
}

/*
 * LA57: Level-5 Page Table Routines
 */

static bool_t
pmap_is_la57(pmap)
	pmap_t pmap;
{
	if (pmap->pm_type == PT_X86) {
		return (la57);
	}
	return (FALSE);		/* XXXKIB handle EPT */
}

static void
pmap_cold_la57(physfree)
	vm_offset_t		physfree;
{
	uint64_t cr4;

    /* Check if CR4_LA57 is present */
	cr4 = rcr4();
	cr4 |= CR4_LA57;
	if (cr4 == 0) {
		KPML5phys = NULL;
		return;
	}

	/* Allocated Page Table Level 5 */
	KPML5phys = allocpages(1, &physfree);
}

static void
create_pagetables_la57(firstaddr)
	vm_offset_t *firstaddr;
{
	int i;

	if (KPML5phys != NULL) {
		/* And recursively map PML5 to itself in order to get PTmap */
		IdlePML5 = (pml5_entry_t *)KPML5phys;
		IdlePML5[PDIR_SLOT_KERN] = KPML5phys;
		IdlePML5[PDIR_SLOT_KERN] |= PG_RW | PG_V | pg_nx;

		/* Connect the KVA slots up to the PML5 */
		for (i = 0; i < NKL5_MAX_ENTRIES; i++) {
			IdlePML5[PDIR_SLOT_KERNBASE + i] = IdlePML4 + ptoa(i);
			IdlePML5[PDIR_SLOT_KERNBASE + i] |= PG_RW | PG_V;
		}
	}
}

static void
pmap_enable_la57(cr4)
	uint64_t cr4;
{
	cr4 = rcr4();
	cr4 |= CR4_LA57;
	if (cr4 != 0) {
		load_cr4(cr4);
		load_cr3(IdlePML5);
	}
}

static int
pmap_check_la57(void)
{
	if ((cpu_stdext_feature2 & CPUID_STDEXT2_LA57) == 0) {
		return (1);
	}

	if (!la57) {
		return (1);
	}
	return (0);
}

static void
pmap_bootstrap_la57(firstaddr)
	vm_offset_t firstaddr;
{
	pml5_entry_t *v_pml5;
	pml4_entry_t *v_pml4;
	pdpt_entry_t *v_pdpt;
	pd_entry_t *v_pde;
	pt_entry_t *v_pte;
	vm_offset_t m_pml5, m_pml4, m_pdpt, m_pde, m_pte;
	char *v_code;
	vm_offset_t *m_code, *temp;
	struct region_descriptor region;
	void (*la57_tramp)(uint64_t pml5);

	setregion(&region, gdt, NGDT * sizeof(gdt)-1);
	m_code =  kmem_alloc(kernel_map, (la57_trampoline_end - la57_trampoline));
	v_code = (char *)PHYS_TO_DMAP(m_code);

	/*
	 * Map m_code 1:1, it appears below 4G in KVA due to physical
	 * address being below 4G.  Since kernel KVA is in upper half,
	 * the pml4e should be zero and free for temporary use.
	 */
	m_pml5 = pmap_extract(kernel_pmap, (vm_offset_t)IdlePML5);
	v_pml5 = (pml5_entry_t *)PHYS_TO_DMAP(m_pml5);

	m_pml4 = pmap_extract(kernel_pmap, (vm_offset_t)IdlePML4);
	v_pml4 = (pml4_entry_t *)PHYS_TO_DMAP(m_pml4);

	m_pdpt = pmap_extract(kernel_pmap, (vm_offset_t)IdlePDPT);
	v_pdpt = (pdpt_entry_t *)PHYS_TO_DMAP(m_pdpt);

	m_pde = pmap_extract(kernel_pmap, (vm_offset_t)IdlePTD);
	v_pde = (pd_entry_t *)PHYS_TO_DMAP(m_pde);

	m_pte = pmap_extract(kernel_pmap, (vm_offset_t)KPTmap);
	v_pte = (pt_entry_t *)PHYS_TO_DMAP(m_pte);

	v_pml4[PL4_E(m_code)] = m_pdpt | PG_V | PG_RW | PG_A | PG_M;
	v_pdpt[PL3_E(m_code)] = m_pde | PG_V | PG_RW | PG_A | PG_M;
	v_pde[PL2_E(m_code)] = m_pte | PG_V | PG_RW | PG_A | PG_M;
	v_pte[PL1_E(m_code)] = m_code | PG_V | PG_RW | PG_A | PG_M;

	/*
	 * Add pml5 entry at top of KVA pointing to existing pml4 table,
	 * entering all existing kernel mappings into level 5 table.
	 */
	v_pml5[l4etol5e(PL4_E(UPT_MAX_ADDRESS))] = IdlePML4 | PG_V | PG_RW | PG_A | PG_M | pg_g;

	/*
	 * Add pml5 entry for 1:1 trampoline mapping after LA57 is turned on.
	 */
	v_pml5[l4etol5e(PL4_E(m_code))] = m_pml4 | PG_V | PG_RW | PG_A | PG_M;
	v_pml4[PL4_E(m_code)] = m_pdpt | PG_V | PG_RW | PG_A | PG_M;

	/*
	 * Copy and call the 48->57 trampoline, hope we return there, alive.
	 */
	bcopy(la57_trampoline, v_code, (la57_trampoline_end - la57_trampoline));
	m_code = (v_code + 2 + (la57_trampoline_gdt_desc - la57_trampoline));
	temp = (la57_trampoline_gdt - la57_trampoline) + m_code;
	m_code = temp;
	temp = NULL;
	la57_tramp = (void (*)(uint64_t))pmap_extract(kernel_pmap, m_code);
	invlpg((vm_offset_t)la57_tramp);
	la57_tramp(KPML5phys);
	/*
	 * gdt was necessary reset, switch back to our gdt.
	 */
	lgdt(&region);
	wrmsr(MSR_GSBASE, (uint64_t)&__percpu[0]);
	load_ds(_udatasel);
	load_es(_udatasel);
	load_fs(_ufssel);
	//ssdtosyssd(&gdt_segs[GPROC0_SEL], (struct system_segment_descriptor *)&__pcpu[0].pc_gdt[GPROC0_SEL]);
	//ltr(GSEL(GPROC0_SEL, SEL_KPL));

	/*
	 * Now unmap the trampoline, and free the pages.
	 * Clear pml5 entry used for 1:1 trampoline mapping.
	 */
	//pte_clear(&v_pml5[l4etol5e(PL4_E(m_code))]);
	invlpg((vm_offset_t)v_code);
	kmem_free(kernel_pmap, m_code, sizeof(m_code));
	kmem_free(kernel_pmap, m_pml4, sizeof(m_pml4));
	kmem_free(kernel_pmap, m_pdpt, sizeof(m_pdpt));
	kmem_free(kernel_pmap, m_pde, sizeof(m_pde));
	kmem_free(kernel_pmap, m_pte, sizeof(m_pte));

	/*
	 * Recursively map PML5 to itself in order to get PTmap and
	 * PDmap.
	 */
	v_pml5[PDIR_SLOT_KERN] = IdlePML5 | PG_RW | PG_V | pg_nx;

	IdlePML5 = (pml5_entry_t *)v_pml5;
	IdlePML4 = (pml4_entry_t *)v_pml4;
	IdlePDPT = (pdpt_entry_t *)v_pdpt;
	IdlePTD = (pd_entry_t *)v_pde;
	KPTmap = (pt_entry_t *)v_pte;
}
