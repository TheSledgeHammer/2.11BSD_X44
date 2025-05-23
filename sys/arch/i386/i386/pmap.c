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

/*
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 *	Derived from: hp300/@(#)pmap.c	7.1 (Berkeley) 12/5/90
 */

/*
 *	Reno i386 version, from Mike Hibler's hp300 version.
 */

/*
 *	Manages physical address maps.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information as
 *	to which processors are currently using which maps,
 *	and to when physical maps must be made correct.
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

#ifdef OVERLAY
#include <ovl/include/ovl.h>
#include <ovl/include/ovl_overlay.h>
#endif

#include <machine/bootinfo.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/cputypes.h>
#include <machine/cpuvar.h>
#include <machine/pte.h>
#ifdef SMP
#include <machine/smp.h>
#endif
#include <machine/pmap.h>
#include <machine/pmap_tlb.h>
#include <machine/pmap_hat.h>

#include <machine/apic/lapicvar.h>
#include <machine/isa/isa_machdep.h>

uint32_t 	ptp_masks[] = PTP_MASK_INITIALIZER;
uint32_t 	ptp_shifts[] = PTP_SHIFT_INITIALIZER;
pd_entry_t 	*NPDE[] = PDES_INITIALIZER;
pd_entry_t 	*APDE[] = APDES_INITIALIZER;

/*
 * Get PDEs and PTEs for user/kernel address space
 */
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

#ifdef PMAP_PAE_COMP
pt_entry_t pg_nx;
#else
#define	pg_nx	0
#endif

int pat_works;
int pg_ps_enabled;
int elf32_nxstack;

#define	PAT_INDEX_SIZE	8
static int 		pat_index[PAT_INDEX_SIZE];	/* cache mode to PAT index conversion */

extern char 	_end[]; 			/* boot.ldscript */
vm_offset_t 	kernel_vm_end;
u_long 			physfree;			/* phys addr of next free page */
u_long 			vm86phystk;			/* PA of vm86/bios stack */
u_long 			vm86paddr;			/* address of vm86 region */
int 			vm86pa;				/* phys addr of vm86 region */
u_long 			KPTphys;			/* phys addr of kernel page tables */
u_long 			KERNend;
u_long 			tramp_idleptd;

vm_offset_t 	avail_start;		/* PA of first available physical page */
vm_offset_t		avail_end;			/* PA of last available physical page */

vm_offset_t		virtual_avail;  	/* VA of first avail page (after kernel bss)*/
vm_offset_t		virtual_end;		/* VA of last avail page (end of kernel AS) */
vm_offset_t		vm_first_phys;		/* PA of first managed page */
vm_offset_t		vm_last_phys;		/* PA just past last managed page */

struct pmap_hat_list 	vmhat_list;

#ifdef OVERLAY
u_long			OVLphys;			/* phys addr of overlay tables */
ovl_entry_t		*IdleOVL;

vm_offset_t		overlay_avail;  	/* VA of first avail page (after kernel bss)*/
vm_offset_t		overlay_end;
vm_offset_t		ovl_first_phys;
vm_offset_t		ovl_last_phys;
struct pmap_hat_list	ovlhat_list;
#endif

bool_t			pmap_initialized = FALSE;	/* Has pmap_init completed? */

pd_entry_t 		*IdlePTD;			/* phys addr of kernel PTD */
#ifdef PMAP_PAE_COMP
pdpt_entry_t 	*IdlePDPT;			/* phys addr of kernel PDPT */
#endif
pt_entry_t 		*KPTmap;			/* address of kernel page tables */
pv_entry_t		pv_table;			/* array of entries, one per page */


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
static pd_entry_t 	*pmap_table(pmap_t, vm_offset_t, int);
static pt_entry_t 	*pmap_pte(pmap_t, vm_offset_t);

vm_offset_t
pa_index(pa)
	vm_offset_t pa;
{
	return (pmap_hat_pa_index(pa, PMAP_HAT_VM));
}

pv_entry_t
pa_to_pvh(pa)
	vm_offset_t pa;
{
	return (pmap_hat_pa_to_pvh(pa, PMAP_HAT_VM));
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
	pd_entry_t opde;

	if (pmap && pmap_pde_v(pmap_pde(pmap, va, 1))) {
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

static u_long
allocpages(cnt, physfree)
	u_int cnt;
	u_long *physfree;
{
	u_long res;

	res = *physfree;
	*physfree += PGSIZE * cnt;
	bzero((void *)res, PGSIZE * cnt);
	return (res);
}

static void
pmap_cold_map(pa, va, cnt)
	u_long pa, va, cnt;
{
	pt_entry_t *pt;
	for (pt = (pt_entry_t *)KPTphys + atop(va); cnt > 0; cnt--, pt++, va += PGSIZE, pa += PGSIZE) {
		*pt = pa | PG_V | PG_RW | PG_A | PG_M;
	}
}

static void
pmap_cold_mapident(pa, cnt)
	u_long pa, cnt;
{
	pmap_cold_map(pa, pa, cnt);
}

void
pmap_remap_lower(enable)
	bool_t enable;
{
	int i;

	for (i = 0; i < LOWPTDI; i++) {
		IdlePTD[i] = enable ? IdlePTD[LOWPTDI + i] : 0;
	}
	lcr3(rcr3()); /* invalidate TLB */
}

#ifdef notyet
static void
pmap_bootinfo(boot)
	struct bootinfo *boot;
{
	if (boot->bi_esymtab != 0) {
		physfree = boot->bi_esymtab;
	}
	if (boot->bi_kernend != 0) {
		physfree = boot->bi_kernend;
	}
}
#endif

void
pmap_cold(void)
{
	pt_entry_t *pt;
	u_long a;
	u_int cr3, ncr4;

	physfree = (u_long)&_end;
	//pmap_bootinfo(&i386boot);
	physfree = roundup(physfree, NBPDR);
	KERNend = physfree;

#ifdef OVERLAY
	/* Allocate Overlay Space */
	OVLphys	= allocpages(1, &physfree);
	IdleOVL = (ovl_entry_t *)allocpages(NPGOVL, &physfree);
#endif

	/* Allocate Kernel Page Tables */
	KPTphys = allocpages(NKPT, &physfree);
	KPTmap = (pt_entry_t *)KPTphys;

	/* Allocate Page Table Directory */
#ifdef PMAP_PAE_COMP
	/* XXX only need 32 bytes (easier for now) */
	IdlePDPT = (pdpt_entry_t *)allocpages(1, &physfree);
#endif
	IdlePTD = (pd_entry_t *)allocpages(NPGPTD, &physfree);

	/*
	 * Allocate KSTACK.  Leave a guard page between IdlePTD and
	 * proc0kstack, to control stack overflow for thread0 and
	 * prevent corruption of the page table.  We leak the guard
	 * physical memory due to 1:1 mappings.
	 */
	allocpages(1, &physfree);
	proc0kstack = allocpages(KSTACK_PAGES, &physfree);

	/* vm86/bios stack */
	vm86phystk = allocpages(1, &physfree);

	/* pgtable + ext + IOPAGES */
	vm86paddr = vm86pa = allocpages(3, &physfree);

#ifdef OVERLAY
	/* Install Overlay Space */
	for (a = 0; a < NPGOVL; a++) {
		IdleOVL[a] = (OVLphys + ptoa(a));
	}
#endif

	/* Install page tables into PTD.  Page table page 1 is wasted. */
	for (a = 0; a < NKPT; a++) {
		IdlePTD[a] = (KPTphys + ptoa(a)) | PG_V | PG_RW | PG_A | PG_M;
	}

#ifdef PMAP_PAE_COMP
	/* PAE install PTD pointers into PDPT */
	for (a = 0; a < NPGPTD; a++) {
		IdlePDPT[a] = ((u_int)IdlePTD + ptoa(a)) | PG_V;
	}
#endif

	/*
	 * Install recursive mapping for kernel page tables into
	 * itself.
	 */
	for (a = 0; a < NPGPTD; a++) {
		IdlePTD[PTDPTDI + a] = ((u_int)IdlePTD + ptoa(a)) | PG_V | PG_RW;
	}

	/*
	 * Initialize page table pages mapping physical address zero
	 * through the (physical) end of the kernel.  Many of these
	 * pages must be reserved, and we reserve them all and map
	 * them linearly for convenience.  We do this even if we've
	 * enabled PSE above; we'll just switch the corresponding
	 * kernel PDEs before we turn on paging.
	 *
	 * This and all other page table entries allow read and write
	 * access for various reasons.  Kernel mappings never have any
	 * access restrictions.
	 */
	pmap_cold_mapident(0, atop(NBPDR) * LOWPTDI);
	pmap_cold_map(0, NBPDR * LOWPTDI, atop(NBPDR) * LOWPTDI);
	pmap_cold_mapident(KERNBASE, atop(KERNend - KERNBASE));

	/* Map page table directory */
#ifdef PMAP_PAE_COMP
	pmap_cold_mapident((u_long)IdlePDPT, 1);
#endif
	pmap_cold_mapident((u_long)IdlePTD, NPGPTD);

	/* Map early KPTmap.  It is really pmap_cold_mapident. */
	pmap_cold_map(KPTphys, (u_long)KPTmap, NKPT);

	/* Map proc0kstack */
	pmap_cold_mapident(proc0kstack, KSTACK_PAGES);
	/* ISA hole already mapped */

	pmap_cold_mapident(vm86phystk, 1);
	pmap_cold_mapident(vm86pa, 3);

	/* Map page 0 into the vm86 page table */
	*(pt_entry_t *)vm86pa = 0 | PG_RW | PG_U | PG_A | PG_M | PG_V;

	/* ...likewise for the ISA hole for vm86 */
	for (pt = (pt_entry_t *)vm86pa + atop(ISA_HOLE_START), a = 0; a < atop(ISA_HOLE_LENGTH); a++, pt++) {
		*pt = (ISA_HOLE_START + ptoa(a)) | PG_RW | PG_U | PG_A | PG_M | PG_V;
	}

	/* Enable PSE, PGE, VME, and PAE if configured. */
	ncr4 = 0;
	if ((cpu_feature & CPUID_PSE) != 0) {
		ncr4 |= CR4_PSE;
		pseflag = PG_PS;
		/*
		 * Superpage mapping of the kernel text.  Existing 4k
		 * page table pages are wasted.
		 */
		for (a = KERNBASE; a < KERNend; a += NBPDR) {
			IdlePTD[a >> PDRSHIFT] = a | PG_PS | PG_A | PG_M | PG_RW | PG_V;
		}
	}
	if ((cpu_feature & CPUID_PGE) != 0) {
		ncr4 |= CR4_PGE;
		pgeflag = PG_G;
	}
	ncr4 |= (cpu_feature & CPUID_VME) != 0 ? CR4_VME : 0;
#ifdef PMAP_PAE_COMP
	ncr4 |= CR4_PAE;
#endif
	if (ncr4 != 0)
		lcr4(rcr4() | ncr4);

	/* Now enable paging */
#ifdef PMAP_PAE_COMP
	cr3 = (u_int) IdlePDPT;
	if ((cpu_feature & CPUID_PAT) == 0)
		wbinvd();
#else
	cr3 = (u_int) IdlePTD;
#endif
	tramp_idleptd = cr3;
	lcr3(cr3);
	lcr0(rcr0() | CR0_PG);

	/*
	 * Now running relocated at KERNBASE where the system is
	 * linked to run.
	 */

	/*
	 * Remove the lowest part of the double mapping of low memory
	 * to get some null pointer checks.
	 */
	pmap_remap_lower(FALSE);

	kernel_vm_end = /* 0 + */NKPT * NBPDR;
}

void
pmap_set_nx(void)
{
#ifdef PMAP_PAE_COMP
	if ((amd_feature & AMDID_NX) == 0)
		return;
	pg_nx = PG_NX;
	elf32_nxstack = 1;
	/* EFER.EFER_NXE is set in initializecpu(). */
#endif
}

void
pmap_bootstrap(firstaddr)
	vm_offset_t firstaddr;
{
	vm_offset_t va;
	pt_entry_t *pte, *unused;
	u_long res;
	int i;

	res = atop(firstaddr - (vm_offset_t)KERNLOAD);

	avail_start = firstaddr;

#ifdef OVERLAY
	pmap_overlay_bootstrap(firstaddr);
#else
	virtual_avail = (vm_offset_t)firstaddr;
	virtual_end = VM_MAX_KERNEL_ADDRESS;
#endif

	/*
	 * Initialize protection array.
	 */
	i386_protection_init();

	/*
	 * Initialize the kernel pmap (which is statically allocated).
	 * Count bootstrap data as being resident in case any of this data is
	 * later unmapped (using pmap_remove()) and freed.
	 */
#ifdef OVERLAY
	kernel_pmap->pm_ovltab = (ovl_entry_t *)(KERNBASE + IdleOVL);
#endif
	kernel_pmap->pm_pdir = (pd_entry_t *)(KERNBASE + IdlePTD);
#ifdef PMAP_PAE_COMP
	for (i = 0; i < NPGPTD; i++) {
		kernel_pmap->pm_pdirpa[i] = (vm_offset_t *)IdlePTD + PAGE_SIZE * i;
	}
	kernel_pmap->pm_pdpt = (pdpt_entry_t *)(KERNBASE + IdlePDPT);
#else
	kernel_pmap->pm_pdirpa[0] = (vm_offset_t)IdlePTD;
#endif
	pmap_lock_init(kernel_pmap, "kernel_pmap_lock");
	LIST_INIT(&pmap_header);
	kernel_pmap->pm_count = 1;
	kernel_pmap->pm_stats.resident_count = res;

#define	SYSMAP(c, p, v, n)	\
	v = (c)va; va += ((n) * I386_PAGE_SIZE); p = pte; pte += (n);

	va = virtual_avail;
	pte = pmap_pte(kernel_pmap, va);

	SYSMAP(caddr_t, CMAP1, CADDR1, 1)
	SYSMAP(caddr_t, CMAP2, CADDR2, 1)
	SYSMAP(caddr_t, CMAP3, CADDR3, 1)
	SYSMAP(caddr_t, cmmap, cvmmap, 1)
	SYSMAP(struct msgbuf *, msgbufmap, msgbufp, 1)
	/* XXX: allow for msgbuf */
	avail_end -= i386_round_page(sizeof(struct msgbuf));

	for (i = 0; i < NKPT; i++) {
		KPTD[i] = (KPTphys + ptoa(i)) | PG_RW | PG_V;
	}

	/*
	 * PADDR1 and PADDR2 are used by pmap_pte_quick() and pmap_pte(),
	 * respectively.
	 */
	SYSMAP(pt_entry_t *, PMAP1, PADDR1, 1)
	SYSMAP(pt_entry_t *, PMAP2, PADDR2, 1)
	SYSMAP(pt_entry_t *, PMAP3, PADDR3, 1)

	virtual_avail = va;

	local_apic_va = pmap_bootstrap_valloc(1);
	local_apic_pa = pmap_bootstrap_palloc(1);

	/*
	 * Initialize the TLB shootdown queues.
	 */
	pmap_tlb_init();

	/*
	 * Initialize the PAT MSR if present.
	 * pmap_init_pat() clears and sets CR4_PGE, which, as a
	 * side-effect, invalidates stale PG_G TLB entries that might
	 * have been created in our pre-boot environment.  We assume
	 * that PAT support implies PGE and in reverse, PGE presence
	 * comes with PAT.  Both features were added for Pentium Pro.
	 */
	pmap_init_pat();
}

int
pmap_isvalidphys(addr)
	vm_offset_t addr;
{
	if (addr < 0xa0000)
		return (1);
	if (addr >= 0x100000)
		return (1);
	return(0);
}

/*
 * pmap_bootstrap_allocate: allocate vm space into bootstrap area.
 * va: is taken from virtual_avail.
 * size: is the size of the virtual address to allocate
 */
void *
pmap_bootstrap_allocate(va, size)
	vm_offset_t va;
	u_long 		size;
{
	int i;

	size = round_page(size);

	/* deal with "hole incursion" */
	for (i = 0; i < size; i += PAGE_SIZE) {
		while (!pmap_isvalidphys(avail_start)) {
			avail_start += PAGE_SIZE;
		}
		va = pmap_map(va, avail_start, avail_start + PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE);
		avail_start += PAGE_SIZE;
	}
	bzero((caddr_t) va, size);
	return ((void *) va);
}

void *
pmap_bootstrap_alloc(size)
	u_long size;
{
	extern bool_t vm_page_startup_initialized;

	if (vm_page_startup_initialized) {
		panic("pmap_bootstrap_alloc: called after startup initialized");
	}

	return (pmap_bootstrap_allocate(virtual_avail, size));
}

/*
 * pmap_bootstrap_valloc: allocate a virtual address in the bootstrap area.
 * This function is to be used before any VM system has been set up.
 *
 * The va is taken from virtual_avail.
 */
static vm_offset_t
pmap_bootstrap_valloc(npages)
	size_t npages;
{
	vm_offset_t va = virtual_avail;
	virtual_avail += npages * PAGE_SIZE;
	return (va);
}

/*
 * pmap_bootstrap_palloc: allocate a physical address in the bootstrap area.
 * This function is to be used before any VM system has been set up.
 *
 * The pa is taken from avail_start.
 */
static vm_offset_t
pmap_bootstrap_palloc(npages)
	size_t npages;
{
	vm_offset_t pa = avail_start;
	avail_start += npages * PAGE_SIZE;
	return (pa);
}

/*
 * Setup the PAT MSR.
 */
void
pmap_init_pat(void)
{
	int pat_table[PAT_INDEX_SIZE];
	uint64_t pat_msr;
	u_long cr0, cr4;
	int i;

	/* Set default PAT index table. */
	for (i = 0; i < PAT_INDEX_SIZE; i++)
		pat_table[i] = -1;
	pat_table[PAT_WRITE_BACK] = 0;
	pat_table[PAT_WRITE_THROUGH] = 1;
	pat_table[PAT_UNCACHEABLE] = 3;
	pat_table[PAT_WRITE_COMBINING] = 3;
	pat_table[PAT_WRITE_PROTECTED] = 3;
	pat_table[PAT_UNCACHED] = 3;

	/*
	 * Bail if this CPU doesn't implement PAT.
	 * We assume that PAT support implies PGE.
	 */
	if ((cpu_feature & CPUID_PAT) == 0) {
		for (i = 0; i < PAT_INDEX_SIZE; i++)
			pat_index[i] = pat_table[i];
		pat_works = 0;
		return;
	}

	/*
	 * Due to some Intel errata, we can only safely use the lower 4
	 * PAT entries.
	 *
	 *   Intel Pentium III Processor Specification Update
	 * Errata E.27 (Upper Four PAT Entries Not Usable With Mode B
	 * or Mode C Paging)
	 *
	 *   Intel Pentium IV  Processor Specification Update
	 * Errata N46 (PAT Index MSB May Be Calculated Incorrectly)
	 */
	if (cpu_vendor_id == CPUVENDOR_INTEL &&
			!(CPUID_TO_FAMILY(cpu_id) == 6 && CPUID_TO_MODEL(cpu_id) >= 0xe))
		pat_works = 0;

	/* Initialize default PAT entries. */
	pat_msr = PAT_VALUE(0, PAT_WRITE_BACK) |
			PAT_VALUE(1, PAT_WRITE_THROUGH) |
			PAT_VALUE(2, PAT_UNCACHED) |
			PAT_VALUE(3, PAT_UNCACHEABLE) |
			PAT_VALUE(4, PAT_WRITE_BACK) |
			PAT_VALUE(5, PAT_WRITE_THROUGH) |
			PAT_VALUE(6, PAT_UNCACHED) |
			PAT_VALUE(7, PAT_UNCACHEABLE);

	if (pat_works) {
		/*
		 * Leave the indices 0-3 at the default of WB, WT, UC-, and UC.
		 * Program 5 and 6 as WP and WC.
		 * Leave 4 and 7 as WB and UC.
		 */
		pat_msr &= ~(PAT_MASK(5) | PAT_MASK(6));
		pat_msr |= PAT_VALUE(5, PAT_WRITE_PROTECTED) |
				PAT_VALUE(6, PAT_WRITE_COMBINING);
		pat_table[PAT_UNCACHED] = 2;
		pat_table[PAT_WRITE_PROTECTED] = 5;
		pat_table[PAT_WRITE_COMBINING] = 6;
	} else {
		/*
		 * Just replace PAT Index 2 with WC instead of UC-.
		 */
		pat_msr &= ~PAT_MASK(2);
		pat_msr |= PAT_VALUE(2, PAT_WRITE_COMBINING);
		pat_table[PAT_WRITE_COMBINING] = 2;
	}

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
	for (i = 0; i < PAT_INDEX_SIZE; i++)
		pat_index[i] = pat_table[i];

	/* Flush caches and TLBs again. */
	wbinvd();
	invltlb();

	/* Restore caches and PGE. */
	lcr0(cr0);
	lcr4(cr4);
}

static void
pmap_pj_page_init(void)
{
	int i;

	pj_page = (void *)kmem_alloc(kernel_map, PAGE_SIZE);
	if (pj_page == NULL) {
		panic("pmap_init: pj_page");
	}
	for (i = 0; i < (PAGE_SIZE / sizeof (union pmap_tlb_shootdown_job_al) - 1); i++) {
		pj_page[i].pja_job.pj_nextfree = &pj_page[i + 1].pja_job;
	}
	pj_page[i].pja_job.pj_nextfree = NULL;
	pj_free = &pj_page[0];
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(phys_start, phys_end)
	vm_offset_t	phys_start, phys_end;
{
	vm_offset_t addr1, addr2;
	vm_size_t npg, s;
	int i;

	addr1 = (vm_offset_t)(KERNBASE + KPTphys);
	vm_hat_init(&vmhat_list, phys_start, phys_end, addr1);
	pmap_pj_page_init();
#ifdef OVERLAY
	addr2 = (vm_offset_t)(KERNBASE + OVLphys);
	ovl_hat_init(&ovlhat_list, phys_start, phys_end, addr2);
#endif

	/*
	 * Now it is safe to enable pv_table recording.
	 */
#ifdef OVERLAY
	vm_first_phys = ovl_first_phys = phys_start;
	vm_last_phys = ovl_last_phys = phys_end;
#else
	vm_first_phys = phys_start;
	vm_last_phys = phys_end;
#endif
	pmap_initialized = TRUE;
}

/*
 *	Used to map a range of physical addresses into kernel
 *	virtual address space.
 *
 *	For now, VM is already on, we only need to map the
 *	specified memory.
 */
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

/*
 *	Create and return a physical map.
 *
 *	If the size specified for the map
 *	is zero, the map is an actual physical
 *	map, and may be referenced by the
 *	hardware.
 *
 *	If the size specified is non-zero,
 *	the map will be used in software only, and
 *	is bounded by that size.
 *
 * [ just allocate a ptd and mark it uninitialize -- should we track
 *   with a table which process has which ptd? -wfj ]
 */

pmap_t
pmap_create(size)
	vm_size_t	size;
{
	register pmap_t pmap;

	/*
	 * Software use map does not need a pmap
	 */
	if (size) {
		return (NULL);
	}

	/* XXX: is it ok to wait here? */
	pmap = (pmap_t) malloc(sizeof(*pmap), M_VMPMAP, M_WAITOK);
	bzero(pmap, sizeof(*pmap));
	pmap_pinit(pmap);

	return (pmap);
}

static void
pmap_pinit_pdir(pdir, pdirpa)
	pd_entry_t *pdir;
	vm_offset_t *pdirpa;
{
	int i;

	/*
	 * No need to allocate page table space yet but we do need a
	 * valid page directory table.
	 */

	pdir = (pd_entry_t *)kmem_alloc(kernel_map, NBPTD);
	pdirpa[0] = pdir[PDIR_SLOT_PTE] & PG_FRAME;

	/* zero init area */
	bzero(pdir, PDIR_SLOT_PTE * sizeof(pd_entry_t));

	/* put in recursive PDE to map the PTEs */
	pdir[PDIR_SLOT_PTE] = pdirpa[0] | PG_V | PG_KW;

	bcopy(&PDP_BASE[PDIR_SLOT_KERN], &pdir[PDIR_SLOT_KERN], sizeof(pd_entry_t));

	/* install self-referential address mapping entry */
	for (i = 0; i < NPGPTD; i++) {
		pdir[PDIR_SLOT_PTE + i] = pmap_extract(kernel_pmap, (vm_offset_t)pdir[i]);
#ifdef PMAP_PAE_COMP
		pdirpa[i] = pdir[PDIR_SLOT_PTE + i] & PG_FRAME;
#endif
	}

	/* zero the rest */
	bzero(&pdir[PDIR_SLOT_KERN], (NPDEPG - (PDIR_SLOT_KERN)) * sizeof(pd_entry_t));
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
#ifdef OVERLAY
	pmap_pinit_ovltab(pmap->pm_ovltab);
#endif

	pmap_pinit_pdir(pmap->pm_pdir, pmap->pm_pdirpa);

#ifdef PMAP_PAE_COMP
	pmap_pinit_pdpt(pmap->pm_pdpt);
#endif

	pmap->pm_active = 0;
	pmap->pm_count = 1;
	pmap_lock_init(pmap, "pmap_lock");
	bzero(&pmap->pm_stats, sizeof(pmap->pm_stats));

	pmap_lock(pmap);
	LIST_INSERT_HEAD(&pmap_header, pmap, pm_list);
	pmap_unlock(pmap);
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */
void
pmap_destroy(pmap)
	register pmap_t pmap;
{
	int count;

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

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_release(pmap)
	register pmap_t pmap;
{
	kmem_free(kernel_map, (vm_offset_t)pmap->pm_pdir, NBPTD);

#ifdef PMAP_PAE_COMP
	kmem_free(kernel_map, (vm_offset_t)pmap->pm_pdpt, NBPTD * sizeof(pdpt_entry_t));
#endif
}

/*
 *	Add a reference to the specified pmap.
 */
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

/*
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */
void
pmap_remove(pmap, sva, eva)
	register pmap_t pmap;
	vm_offset_t sva, eva;
{
	register vm_offset_t pa, va;
	register pt_entry_t *pte;
	register pv_entry_t pv, npv;
	register int ix;
	int bits;

	if (pmap == NULL) {
		return;
	}

	for (va = sva; va < eva; va += PAGE_SIZE) {
		/*
		 * Weed out invalid mappings.
		 * Note: we assume that the page directory table is
	 	 * always allocated, and in kernel virtual.
		 */
		if (!pmap_pde_v(pmap_map_pde(pmap, va))) {
			continue;
		}

		pte = pmap_pte(pmap, va);
		if (pte == 0) {
			continue;
		}
		pa = pmap_pte_pa(pte);
		if (pa == 0) {
			continue;
		}
		/*
		 * Update statistics
		 */
		if (pmap_pte_w(pte)) {
			pmap->pm_stats.wired_count--;
		}
		pmap->pm_stats.resident_count--;

		/*
		 * Invalidate the PTEs.
		 * XXX: should cluster them up and invalidate as many
		 * as possible at once.
		 */
		bits = ix = 0;
		do {
			bits |= *(int *)pte & (PG_U | PG_M);
			*(int *)pte++ = 0;
		} while (++ix != 1);

		if (pmap == &curproc->p_vmspace->vm_pmap) {
			pmap_activate(pmap, (struct pcb *)curproc->p_addr);
		}
		tlbflush();

		/*
		 * Remove from the PV table (raise IPL since we
		 * may be called at interrupt time).
		 */
		pmap_hat_remove_pv(pmap, va, pa, bits, PMAP_HAT_VM, vm_first_phys, vm_last_phys);
	}
}

/*
 *	Routine:	pmap_remove_all
 *	Function:
 *		Removes this physical page from
 *		all physical maps in which it resides.
 *		Reflects back modify bits to the pager.
 */
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

/*
 *	Routine:	pmap_copy_on_write
 *	Function:
 *		Remove write privileges from all
 *		physical maps for this physical page.
 */
void
pmap_copy_on_write(pa)
	vm_offset_t pa;
{
	pmap_changebit(pa, PG_RO, TRUE);
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap, sva, eva, prot)
	register pmap_t	pmap;
	vm_offset_t	sva, eva;
	vm_prot_t	prot;
{
	register pt_entry_t *pte;
	register vm_offset_t va;
	register int ix;
	int i386prot;
	bool_t firstpage = TRUE;

	if (pmap == NULL) {
		return;
	}

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE) {
		return;
	}

	for (va = sva; va < eva; va += PAGE_SIZE) {
		/*
		 * Page table page is not allocated.
		 * Skip it, we don't want to force allocation
		 * of unnecessary PTE pages just to set the protection.
		 */
		if (!pmap_pde_v(pmap_map_pde(pmap, va))) {
			/* XXX: avoid address wrap around */
			if (va >= i386_trunc_pdr((vm_offset_t )-1)) {
				break;
			}
			va = i386_round_pdr(va + PAGE_SIZE) - PAGE_SIZE;
			continue;
		} else {
			pte = pmap_pte(pmap, va);
		}

		/*
		 * Page not valid.  Again, skip it.
		 * Should we do this?  Or set protection anyway?
		 */
		if (!pmap_pte_v(pte)) {
			continue;
		}

		ix = 0;
		i386prot = pte_prot(pmap, prot);
		if (va < UPT_MAX_ADDRESS) {
			i386prot |= 2 /*PG_u*/;
		}
		do {
			/* clear VAC here if PG_RO? */
			pmap_pte_set_prot(pte++, i386prot);
		} while (++ix != 1);
	}
out:
	if (pmap == &curproc->p_vmspace->vm_pmap) {
		pmap_activate(pmap, (struct pcb*) curproc->p_addr);
	}
}

/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
void
pmap_enter(pmap, va, pa, prot, wired)
	register pmap_t pmap;
	vm_offset_t va;
	register vm_offset_t pa;
	vm_prot_t prot;
	bool_t wired;
{
	register pt_entry_t *pte;
	register int npte, ix;
	vm_offset_t opa;
	bool_t cacheable = TRUE;
	bool_t checkpv = TRUE;

	if (pmap == NULL)
		return;

	if (va > VM_MAX_KERNEL_ADDRESS) {
		panic("pmap_enter: toobig");
	}
	/* also, should not muck with PTD va! */

	/*
	 * Page Directory table entry not valid, we need a new PT page
	 */
	if (!pmap_pde_v(pmap_map_pde(pmap, va))) {
		printf("ptdi %x", pmap->pm_pdir[PTDPTDI]);
	}

	pte = pmap_pte(pmap, va);
	opa = pmap_pte_pa(pte);

	/*
	 * Mapping has not changed, must be protection or wiring change.
	 */
	if (opa == pa) {
		/*
		 * Wiring change, just update stats.
		 * We don't worry about wiring PT pages as they remain
		 * resident as long as there are valid mappings in them.
		 * Hence, if a user page is wired, the PT page will be also.
		 */
		if ((wired && !pmap_pte_w(pte)) || (!wired && pmap_pte_w(pte))) {
			if (wired) {
				pmap->pm_stats.wired_count++;
			} else {
				pmap->pm_stats.wired_count--;
			}
		}
		goto validate;
	}

	/*
	 * Mapping has changed, invalidate old range and fall through to
	 * handle validating new mapping.
	 */
	if (opa) {
		pmap_remove(pmap, va, va + PAGE_SIZE);
	}

	/*
	 * Enter on the PV list if part of our managed memory
	 * Note that we raise IPL while manipulating pv_table
	 * since pmap_enter can be called at interrupt time.
	 */
	pmap_hat_enter_pv(pmap, va, pa, PMAP_HAT_VM, vm_first_phys, vm_last_phys);

	/*
	 * Assumption: if it is not part of our managed memory
	 * then it must be device memory which may be volitile.
	 */
	if (pmap_initialized) {
		checkpv = cacheable = FALSE;
	}

	/*
	 * Increment counters
	 */
	pmap->pm_stats.resident_count++;
	if (wired) {
		pmap->pm_stats.wired_count++;
	}

validate:
	/*
	 * Now validate mapping with desired protection/wiring.
	 * Assume uniform modified and referenced status for all
	 * I386 pages in a MACH page.
	 */
	npte = (pa & PG_FRAME) | pte_prot(pmap, prot) | PG_V;
	npte |= (*(int*) pte & (PG_M | PG_U));
	if (wired) {
		npte |= PG_W;
	}
	if (va < UPT_MIN_ADDRESS) {
		npte |= PG_u;
	} else if (va < UPT_MAX_ADDRESS) {
		npte |= PG_u | PG_RW;
	}
	ix = 0;
	do {
		*(int*) pte++ = npte;
		npte += I386_PAGE_SIZE;
		va += I386_PAGE_SIZE;
	} while (++ix != 1);
	pte--;
	pmap_invalidate_page(pmap, va);
	tlbflush();
}

/*
 *      pmap_page_protect:
 *
 *      Lower the permission for all mappings to a given page.
 */
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

/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
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

	ptab = pmap_pde(pmap, va, lvl);
	if (pmap_valid_entry(pmap, ptab, va)) {
		return (ptab);
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

	pte = (pt_entry_t *)pmap_table(pmap, va, 1);
	if (pte != NULL) {
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

/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t		dst_pmap;
	pmap_t		src_pmap;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
{
	pt_entry_t *src_pte, *dst_pte, ptetemp;
	pd_entry_t srcptepaddr;
	vm_offset_t addr, end_addr, pdnxt;
	unsigned long ptepindex;
	int i;

	if (dst_addr != src_addr) {
		return;
	}

	end_addr = src_addr + len;
	if (dst_pmap < src_pmap) {
		pmap_lock(dst_pmap);
		pmap_lock(src_pmap);
	} else {
		pmap_lock(src_pmap);
		pmap_lock(dst_pmap);
	}

	for (i = PTP_LEVELS; i > 1; i--) {
		for (addr = src_addr; addr < end_addr; addr = pdnxt) {
			pdnxt = (addr + NBPDR) & ~NBPDR-1;
			if (pdnxt < addr) {
				pdnxt = end_addr;
			}
			ptepindex = PL_I(addr, i);

			srcptepaddr = src_pmap->pm_pdir[ptepindex];
			if (srcptepaddr == 0) {
				continue;
			}
			if (srcptepaddr & PG_PS) {
				if ((addr & NBPDR-1) != 0 || addr + NBPDR > end_addr) {
					continue;
				}
				if (dst_pmap->pm_pdir[ptepindex] == 0 && ((srcptepaddr & PG_MANAGED) == 0)) {
					dst_pmap->pm_pdir[ptepindex] = srcptepaddr & ~PG_W;
					dst_pmap->pm_stats.resident_count += NBPDR / PAGE_SIZE;
				}
				continue;
			}
			if (pdnxt > end_addr) {
				pdnxt = end_addr;
			}

			src_pte = pmap_pte(src_pmap, addr);
			while (addr < pdnxt) {
				ptetemp = *src_pte;

				if ((ptetemp & PG_MANAGED) != 0) {
					dst_pte = pmap_pte(dst_pmap, addr);
					if (*dst_pte == 0) {
						*dst_pte = ptetemp & ~(PG_W | PG_M | PG_A);
						dst_pmap->pm_stats.resident_count++;
					} else {
						goto out;
					}
				}
				addr += PAGE_SIZE;
				src_pte++;
			}
		}
	}

out:
	pmap_lock(src_pmap);
	pmap_lock(dst_pmap);
}

/*
 *	Require that all active physical maps contain no
 *	incorrect entries NOW.  [This update includes
 *	forcing updates of any address map caching.]
 *
 *	Generally used to insure that a thread about
 *	to run will see a semantically correct world.
 */
void
pmap_update(void)
{
	tlbflush();
}

/*
 *	Routine:	pmap_collect (Taken from 4.4BSD lite2 HP300)
 *	Function:
 *		Garbage collects the physical map system for
 *		pages which are no longer used.
 *		Success need not be guaranteed -- that is, there
 *		may well be pages which are not referenced, but
 *		others may be collected.
 *	Usage:
 *		Called by the pageout daemon when pages are scarce.
 * [ needs to be written -wfj ]
 */
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
		pte = (int *)(pv->pv_va + I386_PAGE_SIZE);
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

/* [ macro again?, should I force kstack into user map here? -wfj ] */
void
pmap_activate(pmap, pcbp)
	register pmap_t pmap;
	struct pcb *pcbp;
{
	int x;

	if(pmap != NULL && pmap->pm_pdchanged) {

#ifdef PMAP_PAE_COMP
		pcbp->pcb_cr3 = pmap_extract(kernel_pmap, (vm_offset_t)pmap->pm_pdpt);
#else
		pcbp->pcb_cr3 = pmap_extract(kernel_pmap, (vm_offset_t)pmap->pm_pdir);
#endif
		if(pmap == &curproc->p_vmspace->vm_pmap) {
			lcr3(pcbp->pcb_cr3);
		}
		pmap->pm_pdchanged = FALSE;
	}
}

/*
 * zero out physical memory
 * specified in relocation units (NBPG bytes)
 */
void
clearseg(n)
	int n;
{
	*(int*) CMAP2 = PG_V | PG_KW | ctob(n);
	lcr3(rcr3());
	bzero(CADDR2, NBPG);
	*(int*) CADDR2 = 0;
}

/*
 *	pmap_zero_page zeros the specified (machine independent)
 *	page by mapping the page into virtual memory and using
 *	bzero to clear its contents, one machine dependent page
 *	at a time.
 */
void
pmap_zero_page(phys)
	register vm_offset_t	phys;
{
	register int ix;

	phys >>= PGSHIFT;
	ix = 0;
	do {
		clearseg(phys++);
	} while (++ix != 1);
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
void
physcopyseg(frm, to)
	int frm, to;
{
	*(int*) CMAP1 = PG_V | PG_KW | ctob(frm);
	*(int*) CMAP2 = PG_V | PG_KW | ctob(to);
	lcr3(rcr3());
	bcopy(CADDR1, CADDR2, NBPG);
}

/*
 *	pmap_copy_page copies the specified (machine independent)
 *	page by mapping the page into virtual memory and using
 *	bcopy to copy the page, one machine dependent page at a
 *	time.
 */
void
pmap_copy_page(src, dst)
	register vm_offset_t	src, dst;
{
	register int ix;

	src >>= PGSHIFT;
	dst >>= PGSHIFT;
	ix = 0;
	do {
		physcopyseg(src++, dst++);
	} while (++ix != 1);
}

/*
 *	Routine:	pmap_pageable
 *	Function:
 *		Make the specified pages (by pmap, offset)
 *		pageable (or not) as requested.
 *
 *		A page which is not pageable may not take
 *		a fault; therefore, its page table entry
 *		must remain valid for the duration.
 *
 *		This routine is merely advisory; pmap_enter
 *		will specify that these pages are to be wired
 *		down (or not) as appropriate.
 */
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

/*
 *	Clear the modify bits on the specified physical page.
 */

void
pmap_clear_modify(pa)
	vm_offset_t	pa;
{
	pmap_changebit(pa, PG_M, FALSE);
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */

void
pmap_clear_reference(pa)
	vm_offset_t	pa;
{
	pmap_changebit(pa, PG_U, FALSE);
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */

bool_t
pmap_is_referenced(pa)
	vm_offset_t	pa;
{
	return (pmap_testbit(pa, PG_U));
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */

bool_t
pmap_is_modified(pa)
	vm_offset_t	pa;
{
	return (pmap_testbit(pa, PG_M));
}

vm_offset_t
pmap_phys_address(ppn)
	int ppn;
{
	return (i386_ptob(ppn));
}

/*
 * add a wired page to the kva
 * note that in order for the mapping to take effect -- you
 * should do a pmap_update after doing the pmap_kenter...
 */
void
pmap_kenter(va, pa)
	vm_offset_t va;
	register vm_offset_t pa;
{
	register pt_entry_t *pte;
	int wasvalid = 0;

	pte = vtopte(va);

	if (*pte)
		wasvalid++;

	*pte = (pt_entry_t) ((int) (pa | PG_RW | PG_V | PG_W));

	if (wasvalid) {
		pmap_update();
	}
}

/*
 * remove a page from the kernel pagetables
 */
void
pmap_kremove(va)
	vm_offset_t va;
{
	register pt_entry_t *pte;

	pte = vtopte(va);

	*pte = (pt_entry_t) 0;
	pmap_update();
}

/*
 * Miscellaneous support routines follow
 */
void
i386_protection_init(void)
{
	register int *kp, prot;

	kp = protection_codes;
	for (prot = 0; prot < 8; prot++) {
		switch (prot) {
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE:
			*kp++ = 0;
			break;
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE:
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE:
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE:
			*kp++ = PG_RO;
			break;
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE:
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE:
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
				va += I386_PAGE_SIZE;
				pte++;
			} while (++ix != 1);

			if (pv->pv_pmap == &p->p_vmspace->vm_pmap) {
				pmap_activate(pv->pv_pmap, (struct pcb*) &p->p_addr->u_pcb);
			}
		}
	}
	splx(s);
}

struct bios16_pmap_handle {
	pt_entry_t	*pte;
	pd_entry_t	*ptd;
	pt_entry_t	orig_ptd;
};

void *
pmap_bios16_enter(void)
{
	struct bios16_pmap_handle *h;
	extern pd_entry_t *IdlePTD;

	/*
	 * no page table, so create one and install it.
	 */
	h = (struct bios16_pmap_handle *)malloc(sizeof(struct bios16_pmap_handle), M_TEMP, M_WAITOK);
	h->pte = (pt_entry_t *) malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
	h->ptd = IdlePTD;
	*h->pte = vm86phystk | PG_RW | PG_V;
	h->orig_ptd = *h->ptd;
	*h->ptd = vtophys(h->pte) | PG_RW | PG_V;
	pmap_invalidate_all(kernel_pmap); /* XXX insurance for now */
	return (h);
}

void
pmap_bios16_leave(void *arg)
{
	struct bios16_pmap_handle *h;

	h = arg;
	*h->ptd = h->orig_ptd; /* remove page table */
	/*
	 * XXX only needs to be invlpg(0) but that doesn't work on the 386
	 */
	pmap_invalidate_all(kernel_pmap);
	free(h->pte, M_TEMP); /* ... and free it */
}

#ifdef DEBUG
void
pmap_pvdump(pa)
	vm_offset_t pa;
{
	register pv_entry_t pv;

	printf("pa %x", pa);
	for (pv = pa_to_pvh(pa); pv; pv = pv->pv_next) {
		printf(" -> pmap %x, va %x, flags %x", pv->pv_pmap, pv->pv_va, pv->pv_flags);
		pmap_pads(pv->pv_pmap);
	}
	printf(" ");
}

/* print address space of pmap*/
void
pmap_pads(pm)
	pmap_t pm;
{
	unsigned int va, i, j;
	register int *ptep;

	if (pm == kernel_pmap) {
		return;
	}
	for (i = 0; i < NBPTD; i++) {
		if (pmap_pde_v(pm->pm_pdir[i])) {
			for (j = 0; j < NBPTD; j++) {
				va = (i << L2_SHIFT) + (j << L1_SHIFT);
				if (pm == kernel_pmap && va < KERNBASE) {
					continue;
				}
				if (pm != kernel_pmap && va > UPT_MAX_ADDRESS) {
					continue;
				}
				ptep = pmap_pte(pm, va);
				if (pmap_pte_v(ptep)) {
					printf("%x:%x ", va, *(int*) ptep);
				}
			}
		}
	}
}
#endif
