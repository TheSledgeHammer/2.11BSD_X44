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
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/msgbuf.h>
#include <sys/memrange.h>
#include <sys/sysctl.h>
#include <sys/cputopo.h>
#include <sys/queue.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_param.h>

#include <machine/param.h>
#include <machine/cpu.h>
#include <machine/cputypes.h>
#include <machine/cpuvar.h>
#include <machine/atomic.h>
#include <machine/specialreg.h>
#include <machine/vmparam.h>
#include <machine/pte.h>
#ifdef SMP
#include <machine/smp.h>
#endif
#include <machine/pmap_base.h>

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

/* TLB Shootdown job queue */
struct pmap_tlb_shootdown_job {
	TAILQ_ENTRY(pmap_tlb_shootdown_job) 	pj_list;
	vm_offset_t 							pj_va;			/* virtual address */
	vm_offset_t 							pj_sva;			/* virtual address start */
	vm_offset_t 							pj_eva;			/* virtual address end */
	pmap_t 									pj_pmap;		/* the pmap which maps the address */
	struct pmap_tlb_shootdown_job 			*pj_nextfree;
};

#define PMAP_TLB_SHOOTDOWN_JOB_ALIGN 32
union pmap_tlb_shootdown_job_al {
	struct pmap_tlb_shootdown_job 			pja_job;
	char 									pja_align[PMAP_TLB_SHOOTDOWN_JOB_ALIGN];
};

struct pmap_tlb_shootdown_q {
	TAILQ_HEAD(, pmap_tlb_shootdown_job) 	pq_head;
	int 									pq_count;		/* number of pending requests */
	struct lock_object						pq_slock;		/* spin lock on queue */
	int 									pq_flush;		/* pending flush */
} pmap_tlb_shootdown_q[NCPUS];

#define	PMAP_TLB_MAXJOBS					16

struct lock_object 				pmap_tlb_shootdown_job_lock;
union pmap_tlb_shootdown_job_al *pj_page, *pj_free;

/* System, pte's & apte's addresses */
#define SYSTEM 				0xFE000000					/* virtual address of system start */
#define SYSPDROFF 			0x3F8						/* Page dir index of System Base (i.e. KPTDI_FIRST) */

/*
 * PTmap is recursive pagemap at top of virtual address space.
 * Within PTmap, the page directory can be found (third indirection).
 */
#define PDRPDROFF   		0x3F7						/* Page dir index of Page dir */
#define PTmap       		((pt_entry_t *)0xFDC00000)
#define PTD         		((pd_entry_t *)0xFDFF7000)
#define PTDpde      		((pt_entry_t *)0xFDFF7000+4*PDRPDROFF)

/*
 * APTmap, APTD is the alternate recursive pagemap.
 * It's used when modifying another process's page tables.
 */
#define APDRPDROFF  		0x3FE						/* Page dir index of Page dir */
#define APTmap      		((pt_entry_t *)0xFF800000)
#define APTD        		((pd_entry_t *)0xFFBFE000)
#define APTDpde     		((pt_entry_t *)0xFDFF7000+4*APDRPDROFF)

/*
 * Get PDEs and PTEs for user/kernel address space
 */
#define	pmap_pde(m, v)		(&((m)->pm_pdir[((vm_offset_t)(v) >> PDRSHIFT)]))

#define pmap_pte_pa(pte)	((*(int *)pte & PG_FRAME) != 0)

#define pmap_pte_ci(pte)	((*(int *)pte & PG_CI) != 0)

#define pmap_pde_v(pte)		((*(int *)pte & PG_V) != 0)
#define pmap_pte_w(pte)		((*(int *)pte & PG_W) != 0)
#define pmap_pte_m(pte)		((*(int *)pte & PG_M) != 0)
#define pmap_pte_u(pte)		((*(int *)pte & PG_A) != 0)
#define pmap_pte_v(pte)		((*(int *)pte & PG_V) != 0)

#define pmap_pte_set_w(pte, v) 		\
	if (v) *(int *)(pte) |= PG_W; else *(int *)(pte) &= ~PG_W
#define pmap_pte_set_prot(pte, v) 	\
	if (v) *(int *)(pte) |= PG_PROT; else *(int *)(pte) &= ~PG_PROT

/*
 * Given a map and a machine independent protection code,
 * convert to a vax protection code.
 */
#define pte_prot(m, p)	(protection_codes[p])
int	protection_codes[8];

struct pmap	kernel_pmap_store;

static int pgeflag = 0;			/* PG_G or-in */
static int pseflag = 0;			/* PG_PS or-in */

static int nkpt = NKPT;

#ifdef PMAP_PAE_COMP
pt_entry_t pg_nx;
#else
#define	pg_nx	0
#endif

extern int pat_works;
extern int pg_ps_enabled;

extern int elf32_nxstack;

#define	PAT_INDEX_SIZE	8
static int pat_index[PAT_INDEX_SIZE];	/* cache mode to PAT index conversion */

extern char 		_end[];
extern u_long 		physfree;			/* phys addr of next free page */
extern u_long 		vm86phystk;			/* PA of vm86/bios stack */
extern u_long 		vm86paddr;			/* address of vm86 region */
extern int 			vm86pa;				/* phys addr of vm86 region */

vm_offset_t 		avail_start;		/* PA of first available physical page */
vm_offset_t			avail_end;			/* PA of last available physical page */
vm_size_t			mem_size;			/* memory size in bytes */
vm_offset_t			virtual_avail;  	/* VA of first avail page (after kernel bss)*/
vm_offset_t			virtual_end;		/* VA of last avail page (end of kernel AS) */
vm_offset_t			vm_first_phys;		/* PA of first managed page */
vm_offset_t			vm_last_phys;		/* PA just past last managed page */

int					i386pagesperpage;	/* PAGE_SIZE / I386_PAGE_SIZE */
boolean_t			pmap_initialized = FALSE;	/* Has pmap_init completed? */
char				*pmap_attributes;	/* reference and modify bits */

pd_entry_t 			*IdlePTD;			/* phys addr of kernel PTD */
pdpt_entry_t 		*IdlePDPT;			/* phys addr of kernel PDPT */
pt_entry_t 			*KPTmap;			/* address of kernel page tables */

extern u_long 		KPTphys;			/* phys addr of kernel page tables */
extern u_long 		tramp_idleptd;

boolean_t			pmap_testbit();
void				pmap_clear_modify();
void 				pmap_activate (pmap_t, struct pcb *);

/* linked list of all non-kernel pmaps */
static struct pmap_head pmaps;

/*
 * All those kernel PT submaps that BSD is so fond of
 */
static pd_entry_t 	*KPTD;
pt_entry_t			*CMAP1, *CMAP2, *CMAP3, *mmap;
caddr_t				CADDR1, CADDR2, CADDR3, vmmap;
pt_entry_t			*msgbufmap;
struct msgbuf		*msgbufp;

static pt_entry_t 	*PMAP1 = NULL, *PMAP2, *PMAP3;
static pt_entry_t 	*PADDR1 = NULL, *PADDR2, *PADDR3;

static u_long
allocpages(u_int cnt, u_long *physfree)
{
	u_long res;

	res = *physfree;
	*physfree += PAGE_SIZE * cnt;
	bzero((void *)res, PAGE_SIZE * cnt);
	return (res);
}

static void
pmap_cold_map(pa, va, cnt)
	u_long pa, va, cnt;
{
	pt_entry_t *pt;
	for (pt = (pt_entry_t *)KPTphys + atop(va); cnt > 0; cnt--, pt++, va += PAGE_SIZE, pa += PAGE_SIZE) {
		*pt = pa | PG_V | PG_RW | PG_A | PG_M;
	}
}

static void
pmap_cold_mapident(pa, cnt)
	u_long pa, cnt;
{
	pmap_cold_map(pa, pa, cnt);
}

static void
pmap_remap_lower(enable)
	boolean_t enable;
{
	int i;

	for (i = 0; i < LOWPTDI; i++)
		IdlePTD[i] = enable ? IdlePTD[LOWPTDI + i] : 0;
	lcr3(rcr3()); /* invalidate TLB */
}

void
pmap_cold(void)
{
	u_long *pt;
	u_long a;
	u_int cr3, ncr4;

	physfree = (u_long)&_end;
	physfree = roundup(physfree, NBPDR);
	KERNend = physfree;

	/* Allocate Kernel Page Tables */
	KPTphys = allocpages(NKPT, &physfree);
	KPTmap = (pt_entry_t *)KPTphys;

	/* Allocate Page Table Directory */
	IdlePTD = (pd_entry_t *)allocpages(NPGPTD, &physfree);

	allocpages(1, &physfree);
	proc0kstack = allocpages(KSTACK_PAGES, &physfree);

	/* vm86/bios stack */
	vm86phystk = allocpages(1, &physfree);

	/* pgtable + ext + IOPAGES */
	vm86paddr = vm86pa = allocpages(3, &physfree);

	/* Install page tables into PTD.  Page table page 1 is wasted. */
	for (a = 0; a < NKPT; a++)
		IdlePTD[a] = (KPTphys + ptoa(a)) | PG_V | PG_RW | PG_A | PG_M;

#ifdef PMAP_PAE_COMP
	/* PAE install PTD pointers into PDPT */
	for (a = 0; a < NPGPTD; a++)
		IdlePDPT[a] = ((u_int)IdlePTD + ptoa(a)) | PG_V;
#endif

	/*
	 * Install recursive mapping for kernel page tables into
	 * itself.
	 */
	for (a = 0; a < NPGPTD; a++)
		IdlePTD[PTDPTDI + a] = ((u_int)IdlePTD + ptoa(a)) | PG_V | PG_RW;

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
	*(pt_entry_t*) vm86pa = 0 | PG_RW | PG_U | PG_A | PG_M | PG_V;

	/* ...likewise for the ISA hole for vm86 */
	for (pt = (pt_entry_t*) vm86pa + atop(ISA_HOLE_START), a = 0; a < atop(ISA_HOLE_LENGTH); a++, pt++)
		*pt = (ISA_HOLE_START + ptoa(a)) | PG_RW | PG_U | PG_A | PG_M | PG_V;

	/* Enable PSE, PGE, VME, and PAE if configured. */
	ncr4 = 0;
	if ((cpu_feature & CPUID_PSE) != 0) {
		ncr4 |= CR4_PSE;
		pseflag = PG_PS;
		/*
		 * Superpage mapping of the kernel text.  Existing 4k
		 * page table pages are wasted.
		 */
		for (a = KERNBASE; a < KERNend; a += NBPDR)
			IdlePTD[a >> PDRSHIFT] = a | PG_PS | PG_A | PG_M | PG_RW | PG_V;
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
	i386_pmap_PDRSHIFT = PD_SHIFT;
}

static void
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
pmap_bootstrap(firstaddr, loadaddr)
	vm_offset_t firstaddr;
	vm_offset_t loadaddr;
{
	vm_offset_t va;
	pt_entry_t *pte, *unused;
	u_long res;
	int i;

	extern vm_offset_t 	maxmem, physmem;

	res = atop(firstaddr - (vm_offset_t)KERNLOAD);

	printf("ps %x pe %x ", firstaddr, maxmem << PGSHIFT);

	avail_start = firstaddr;
	avail_end = maxmem << PGSHIFT;

	/* XXX: allow for msgbuf */
	avail_end -= i386_round_page(sizeof(struct msgbuf));

	mem_size = physmem << PGSHIFT;

	virtual_avail = (vm_offset_t)firstaddr;
	virtual_end = VM_MAX_KERNEL_ADDRESS;
	i386pagesperpage = PAGE_SIZE / I386_PAGE_SIZE;

	/*
	 * Initialize protection array.
	 */
	i386_protection_init();

	/*
	 * Initialize the kernel pmap (which is statically allocated).
	 * Count bootstrap data as being resident in case any of this data is
	 * later unmapped (using pmap_remove()) and freed.
	 */
	kernel_pmap->pm_pdir = IdlePTD;

#ifdef PMAP_PAE_COMP
	kernel_pmap()->pm_pdir = IdlePDPT;
#endif
	simple_lock_init(&kernel_pmap()->pm_lock, "kernel_pmap_lock");
	LIST_INIT(&pmaps);
	kernel_pmap()->pm_count = 1;
	kernel_pmap()->pm_stats.resident_count = res;

#define	SYSMAP(c, p, v, n)	\
	v = (c)va; va += ((n) * I386_PAGE_SIZE); p = pte; pte += (n);

	va = virtual_avail;
	pte = vtopte(va);

	SYSMAP(caddr_t, CMAP1, CADDR1, 1)
	SYSMAP(caddr_t, CMAP2, CADDR2, 1)
	SYSMAP(caddr_t, CMAP3, CADDR3, 1)
	SYSMAP(caddr_t, mmap, vmmap, 1)
	SYSMAP(struct msgbuf *, msgbufmap, msgbufp, 1)

	for (i = 0; i < NKPT; i++)
		KPTD[i] = (KPTphys + ptoa(i)) | PG_RW | PG_V;

	/*
	 * PADDR1 and PADDR2 are used by pmap_pte_quick() and pmap_pte(),
	 * respectively.
	 */
	SYSMAP(pt_entry_t *, PMAP1, PADDR1, 1)
	SYSMAP(pt_entry_t *, PMAP2, PADDR2, 1)
	SYSMAP(pt_entry_t *, PMAP3, PADDR3, 1)

	virtual_avail = va;

	/*
	 * Initialize the TLB shootdown queues.
	 */
	pmap_tlb_init();

	pmap_init_pat();
}

int
pmap_isvalidphys(addr)
	int addr;
{
	if (addr < 0xa0000)
		return (1);
	if (addr >= 0x100000)
		return (1);
	return(0);
}

void *
pmap_bootstrap_alloc(size)
	u_long size;
{
	vm_offset_t val;
	int i;
	extern boolean_t vm_page_startup_initialized;

	if (vm_page_startup_initialized)
		panic("pmap_bootstrap_alloc: called after startup initialized");
	size = round_page(size);
	val = virtual_avail;

	/* deal with "hole incursion" */
	for (i = 0; i < size; i += PAGE_SIZE) {
		while (!pmap_isvalidphys(avail_start)) {
			avail_start += PAGE_SIZE;
		}
		virtual_avail = pmap_map(virtual_avail, avail_start, avail_start + PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE);
		avail_start += PAGE_SIZE;
	}

	bzero ((caddr_t) val, size);
	return ((void *) val);
}

/*
 * Setup the PAT MSR.
 */
static void
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

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(phys_start, phys_end)
	vm_offset_t	phys_start, phys_end;
{
	vm_offset_t addr;
	vm_size_t npg, s;
	int i;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_init(%x, %x)\n", phys_start, phys_end);
#endif
/*
  	vm_page_t mpte;
	for (i = 0; i < vm_page_array_size; i++) {
		mpte = PHYS_TO_VM_PAGE(KPTphys + ptoa(i));
		KASSERT(mpte >= vm_page_array && mpte < &vm_page_array[vm_page_array_size], ("pmap_init: page table page is out of range"));
		mpte->offset = i + KPTDI_FIRST;
		mpte->phys_addr = KPTphys + ptoa(i);
	}
*/
	addr = (vm_offset_t) SYSTEM + KPTphys;
	vm_object_reference(kernel_object);
	vm_map_find(kernel_map, kernel_object, addr, &addr, 2 * NBPG, FALSE);

	/*
	 * Allocate memory for random pmap data structures.  Includes the
	 * pv_head_table and pmap_attributes.
	 */
	npg = atop(phys_end - phys_start);
	s = (vm_size_t) (sizeof(struct pv_entry) * npg + npg);
	s = round_page(s);
	pv_table = (pv_entry_t *)kmem_alloc(kernel_map, s);
	addr = (vm_offset_t) pv_table;
	addr += sizeof(struct pv_entry) * npg;
	pmap_attributes = (char *) addr;

#ifdef DEBUG
	if (pmapdebug & PDB_INIT)
		printf("pmap_init: %x bytes (%x pgs): tbl %x attr %x\n", s, npg, pv_table, pmap_attributes);
#endif

	pj_page = (void *)kmem_alloc(kernel_map, PAGE_SIZE);
	if (pj_page == NULL) {
		panic("pmap_init: pj_page");
	}
	for (i = 0; i < (PAGE_SIZE / sizeof (union pmap_tlb_shootdown_job_al) - 1); i++) {
		pj_page[i].pja_job.pj_nextfree = &pj_page[i + 1].pja_job;
	}
	pj_page[i].pja_job.pj_nextfree = NULL;
	pj_free = &pj_page[0];

	/*
	 * Now it is safe to enable pv_table recording.
	 */
	vm_first_phys = phys_start;
	vm_last_phys = phys_end;

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
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_map(%x, %x, %x, %x)\n", virt, start, end, prot);
#endif
	vm_offset_t va, sva;

	va = sva = virt;
	while (start < end) {
		pmap_enter(kernel_pmap(), va, start, prot, FALSE);
		va += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	pmap_invalidate_range(kernel_pmap(), sva, va);
	*virt = va;
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

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create(%x)\n", size);
#endif
	/*
	 * Software use map does not need a pmap
	 */
	if (size)
		return(NULL);

	/* XXX: is it ok to wait here? */
	pmap = (pmap_t) malloc(sizeof *pmap, M_VMPMAP, M_WAITOK);
#ifdef notifwewait
	if (pmap == NULL)
		panic("pmap_create: cannot allocate a pmap");
#endif
	bzero(pmap, sizeof(*pmap));
	pmap_pinit(pmap);
	return (pmap);
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
void
pmap_pinit(pmap)
	register struct pmap *pmap;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		pg("pmap_pinit(%x)\n", pmap);
#endif

	/*
	 * No need to allocate page table space yet but we do need a
	 * valid page directory table.
	 */
	pmap->pm_pdir = (pd_entry_t *) kmem_alloc(kernel_map, NBPG);

	/* wire in kernel global address entries */
	bcopy(PTD + KPTDI_FIRST, pmap->pm_pdir + KPTDI_FIRST, (KPTDI_LAST - KPTDI_FIRST + 1)*4);

	/* install self-referential address mapping entry */
	*(int *)(pmap->pm_pdir + PTDPTDI) = (int)pmap_extract(kernel_pmap(), (vm_offset_t)pmap->pm_pdir) | PG_V | PG_URKW;

	pmap->pm_count = 1;
	simple_lock_init(&pmap->pm_lock, "pmap_lock");
	LIST_INSERT_HEAD(&pmaps, pmap, pm_list);
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

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_destroy(%x)\n", pmap);
#endif
	if (pmap == NULL) {
		return;
	}

	simple_lock(&pmap->pm_lock);
	LIST_REMOVE(pmap, pm_list);
	count = --pmap->pm_count;
	simple_unlock(&pmap->pm_lock);
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
	register struct pmap *pmap;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		pg("pmap_release(%x)\n", pmap);
#endif
#ifdef notdef /* DIAGNOSTIC */
	/* count would be 0 from pmap_destroy... */
	simple_lock(&pmap->pm_lock);
	if (pmap->pm_count != 1)
		panic("pmap_release count");
#endif
	kmem_free(kernel_map, (vm_offset_t)pmap->pm_pdir, NBPG);
}

/*
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap)
	pmap_t	pmap;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		pg("pmap_reference(%x)", pmap);
#endif
	if (pmap != NULL) {
		simple_lock(&pmap->pm_lock);
		pmap->pm_count++;
		simple_unlock(&pmap->pm_lock);
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
	register struct pmap *pmap;
	vm_offset_t sva, eva;
{
	register vm_offset_t pa, va;
	register pt_entry_t *pte;
	register pv_entry_t pv, npv;
	register int ix;
	pmap_t ptpmap;
	int *pde, s, bits;
	boolean_t firstpage = TRUE;
	boolean_t flushcache = FALSE;
#ifdef DEBUG
	pt_entry_t opte;

	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%x, %x, %x)", pmap, sva, eva);
	if (eva >= USRSTACK && eva <= UPT_MAX_ADDRESS)
		nullop();
#endif

	if (pmap == NULL)
		return;

#ifdef DEBUG
	remove_stats.calls++;
#endif
	for (va = sva; va < eva; va += PAGE_SIZE) {
		/*
		 * Weed out invalid mappings.
		 * Note: we assume that the page directory table is
	 	 * always allocated, and in kernel virtual.
		 */
		if (!pmap_pde_v(pmap_pde(pmap, va)))
			continue;

		pte = pmap_pte(pmap, va);
		if (pte == 0)
			continue;
		pa = pmap_pte_pa(pte);
		if (pa == 0)
			continue;
#ifdef DEBUG
		opte = *pte;
		remove_stats.removes++;
#endif
		/*
		 * Update statistics
		 */
		if (pmap_pte_w(pte))
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;

		/*
		 * Invalidate the PTEs.
		 * XXX: should cluster them up and invalidate as many
		 * as possible at once.
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_REMOVE)
			printf("remove: inv %x ptes at %x(%x) ", i386pagesperpage, pte, *(int *)pte);
#endif
		bits = ix = 0;
		do {
			bits |= *(int *)pte & (PG_U|PG_M);
			*(int *)pte++ = 0;
			/*TBIS(va + ix * I386_PAGE_SIZE);*/
		} while (++ix != i386pagesperpage);
		if (pmap == &curproc()->p_vmspace->vm_pmap)
			pmap_activate(pmap, (struct pcb *)curproc()->p_addr);
		/* are we current address space or kernel? */
		/*if (pmap->pm_pdir[PTDPTDI].pd_pfnum == PTDpde.pd_pfnum
			|| pmap == kernel_pmap)
		load_cr3(curpcb->pcb_ptd);*/
		tlbflush();

#ifdef needednotdone
reduce wiring count on page table pages as references drop
#endif

		/*
		 * Remove from the PV table (raise IPL since we
		 * may be called at interrupt time).
		 */
		if (pa < vm_first_phys || pa >= vm_last_phys)
			continue;
		pv = pa_to_pvh(pa);
		s = splimp();
		/*
		 * If it is the first entry on the list, it is actually
		 * in the header and we must copy the following entry up
		 * to the header.  Otherwise we must search the list for
		 * the entry.  In either case we free the now unused entry.
		 */
		if (pmap == pv->pv_pmap && va == pv->pv_va) {
			npv = pv->pv_next;
			if (npv) {
				*pv = *npv;
				free((caddr_t)npv, M_VMPVENT);
			} else
				pv->pv_pmap = NULL;
#ifdef DEBUG
			remove_stats.pvfirst++;
#endif
		} else {
			for (npv = pv->pv_next; npv; npv = npv->pv_next) {
#ifdef DEBUG
				remove_stats.pvsearch++;
#endif
				if (pmap == npv->pv_pmap && va == npv->pv_va)
					break;
				pv = npv;
			}
#ifdef DEBUG
			if (npv == NULL)
				panic("pmap_remove: PA not in pv_tab");
#endif
			pv->pv_next = npv->pv_next;
			free((caddr_t)npv, M_VMPVENT);
			pv = pa_to_pvh(pa);
		}

#ifdef notdef
[tally number of pagetable pages, if sharing of ptpages adjust here]
#endif
		/*
		 * Update saved attributes for managed page
		 */
		pmap_attributes[pa_index(pa)] |= bits;
		splx(s);
	}
#ifdef notdef
[cache and tlb flushing, if needed]
#endif
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

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove_all(%x)", pa);
	/*pmap_pvdump(pa);*/
#endif
	/*
	 * Not one of ours
	 */
	if (pa < vm_first_phys || pa >= vm_last_phys)
		return;

	pv = pa_to_pvh(pa);
	s = splimp();
	/*
	 * Do it the easy way for now
	 */
	while (pv->pv_pmap != NULL) {
#ifdef DEBUG
		if (!pmap_pde_v(pmap_pde(pv->pv_pmap, pv->pv_va)) ||
		    pmap_pte_pa(pmap_pte(pv->pv_pmap, pv->pv_va)) != pa)
			panic("pmap_remove_all: bad mapping");
#endif
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
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_copy_on_write(%x)", pa);
#endif
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
	boolean_t firstpage = TRUE;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%x, %x, %x, %x)", pmap, sva, eva, prot);
#endif
	if (pmap == NULL)
		return;

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE)
		return;

	for (va = sva; va < eva; va += PAGE_SIZE) {
		/*
		 * Page table page is not allocated.
		 * Skip it, we don't want to force allocation
		 * of unnecessary PTE pages just to set the protection.
		 */
		if (!pmap_pde_v(pmap_pde(pmap, va))) {
			/* XXX: avoid address wrap around */
			if (va >= i386_trunc_pdr((vm_offset_t )-1))
				break;
			va = i386_round_pdr(va + PAGE_SIZE) - PAGE_SIZE;
			continue;
		} else
			pte = pmap_pte(pmap, va);

		/*
		 * Page not valid.  Again, skip it.
		 * Should we do this?  Or set protection anyway?
		 */
		if (!pmap_pte_v(pte))
			continue;

		ix = 0;
		i386prot = pte_prot(pmap, prot);
		if (va < UPT_MAX_ADDRESS)
			i386prot |= 2 /*PG_u*/;
		do {
			/* clear VAC here if PG_RO? */
			pmap_pte_set_prot(pte++, i386prot);
			/*TBIS(va + ix * I386_PAGE_SIZE);*/
		} while (++ix != i386pagesperpage);
	}
out:
	if (pmap == &curproc()->p_vmspace->vm_pmap)
		pmap_activate(pmap, (struct pcb*) curproc()->p_addr);
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
	boolean_t wired;
{
	register pt_entry_t *pte;
	register int npte, ix;
	vm_offset_t opa;
	boolean_t cacheable = TRUE;
	boolean_t checkpv = TRUE;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%x, %x, %x, %x, %x)", pmap, va, pa, prot, wired);
	if(!pmap_isvalidphys(pa)) panic("invalid phys");
#endif
	if (pmap == NULL)
		return;

	if (va > VM_MAX_KERNEL_ADDRESS)
		panic("pmap_enter: toobig");
	/* also, should not muck with PTD va! */

#ifdef DEBUG
	if (pmap == kernel_pmap())
		enter_stats.kernel++;
	else
		enter_stats.user++;
#endif

	/*
	 * Page Directory table entry not valid, we need a new PT page
	 */
	if (!pmap_pde_v(pmap_pde(pmap, va))) {
		pg("ptdi %x", pmap->pm_pdir[PTDPTDI]);
	}

	pte = pmap_pte(pmap, va);
	opa = pmap_pte_pa(pte);
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("enter: pte %x, *pte %x ", pte, *(int *)pte);
#endif

	/*
	 * Mapping has not changed, must be protection or wiring change.
	 */
	if (opa == pa) {
#ifdef DEBUG
		enter_stats.pwchange++;
#endif
		/*
		 * Wiring change, just update stats.
		 * We don't worry about wiring PT pages as they remain
		 * resident as long as there are valid mappings in them.
		 * Hence, if a user page is wired, the PT page will be also.
		 */
		if ((wired && !pmap_pte_w(pte)) || (!wired && pmap_pte_w(pte))) {
#ifdef DEBUG
			if (pmapdebug & PDB_ENTER)
				pg("enter: wiring change -> %x ", wired);
#endif
			if (wired)
				pmap->pm_stats.wired_count++;
			else
				pmap->pm_stats.wired_count--;
#ifdef DEBUG
			enter_stats.wchange++;
#endif
		}
		goto validate;
	}

	/*
	 * Mapping has changed, invalidate old range and fall through to
	 * handle validating new mapping.
	 */
	if (opa) {
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("enter: removing old mapping %x pa %x ", va, opa);
#endif
		pmap_remove(pmap, va, va + PAGE_SIZE);
#ifdef DEBUG
		enter_stats.mchange++;
#endif
	}

	/*
	 * Enter on the PV list if part of our managed memory
	 * Note that we raise IPL while manipulating pv_table
	 * since pmap_enter can be called at interrupt time.
	 */
	if (pa >= vm_first_phys && pa < vm_last_phys) {
		register pv_entry_t pv, npv;
		int s;

#ifdef DEBUG
		enter_stats.managed++;
#endif
		pv = pa_to_pvh(pa);
		s = splimp();
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("enter: pv at %x: %x/%x/%x ", pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
		/*
		 * No entries yet, use header as the first entry
		 */
		if (pv->pv_pmap == NULL) {
#ifdef DEBUG
			enter_stats.firstpv++;
#endif
			pv->pv_va = va;
			pv->pv_pmap = pmap;
			pv->pv_next = NULL;
			pv->pv_flags = 0;
		}
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
		else {
			/*printf("second time: ");*/
#ifdef DEBUG
			for (npv = pv; npv; npv = npv->pv_next)
				if (pmap == npv->pv_pmap && va == npv->pv_va)
					panic("pmap_enter: already in pv_tab");
#endif
			npv = (pv_entry_t) malloc(sizeof *npv, M_VMPVENT, M_NOWAIT);
			npv->pv_va = va;
			npv->pv_pmap = pmap;
			npv->pv_next = pv->pv_next;
			pv->pv_next = npv;
#ifdef DEBUG
			if (!npv->pv_next)
				enter_stats.secondpv++;
#endif
			splx(s);
		}
	}
	/*
	 * Assumption: if it is not part of our managed memory
	 * then it must be device memory which may be volitile.
	 */
	if (pmap_initialized) {
		checkpv = cacheable = FALSE;
#ifdef DEBUG
		enter_stats.unmanaged++;
#endif
	}

	/*
	 * Increment counters
	 */
	pmap->pm_stats.resident_count++;
	if (wired)
		pmap->pm_stats.wired_count++;

validate:
	/*
	 * Now validate mapping with desired protection/wiring.
	 * Assume uniform modified and referenced status for all
	 * I386 pages in a MACH page.
	 */
	npte = (pa & PG_FRAME) | pte_prot(pmap, prot) | PG_V;
	npte |= (*(int*) pte & (PG_M | PG_U));
	if (wired)
		npte |= PG_W;
	if (va < UPT_MIN_ADDRESS)
		npte |= PG_u;
	else if (va < UPT_MAX_ADDRESS)
		npte |= PG_u | PG_RW;
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("enter: new pte value %x ", npte);
#endif
	ix = 0;
	do {
		*(int*) pte++ = npte;
		/*TBIS(va);*/
		npte += I386_PAGE_SIZE;
		va += I386_PAGE_SIZE;
	} while (++ix != i386pagesperpage);
	pte--;
	pmap_invalidate_page(pmap, va);
#ifdef DEBUGx
cache, tlb flushes
#endif
	/*pads(pmap);*/
	/*load_cr3(((struct pcb *)curproc->p_addr)->pcb_ptd);*/
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
	boolean_t	wired;
{
	register pt_entry_t *pte;
	register int ix;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_change_wiring(%x, %x, %x)", pmap, va, wired);
#endif
	if (pmap == NULL)
		return;

	pte = pmap_pte(pmap, va);
#ifdef DEBUG
	/*
	 * Page table page is not allocated.
	 * Should this ever happen?  Ignore it for now,
	 * we don't want to force allocation of unnecessary PTE pages.
	 */
	if (!pmap_pde_v(pmap_pde(pmap, va))) {
		if (pmapdebug & PDB_PARANOIA)
			pg("pmap_change_wiring: invalid PDE for %x ", va);
		return;
	}
	/*
	 * Page not valid.  Should this ever happen?
	 * Just continue and change wiring anyway.
	 */
	if (!pmap_pte_v(pte)) {
		if (pmapdebug & PDB_PARANOIA)
			pg("pmap_change_wiring: invalid PTE for %x ", va);
	}
#endif
	if ((wired && !pmap_pte_w(pte)) || (!wired && pmap_pte_w(pte))) {
		if (wired)
			pmap->pm_stats.wired_count++;
		else
			pmap->pm_stats.wired_count--;
	}
	/*
	 * Wiring is not a hardware characteristic so there is no need
	 * to invalidate TLB.
	 */
	ix = 0;
	do {
		pmap_pte_set_w(pte++, wired);
	} while (++ix != i386pagesperpage);
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
	register pmap_t	pmap;
	vm_offset_t va;
{

#ifdef DEBUGx
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_pte(%x, %x) ->\n", pmap, va);
#endif
	pd_entry_t newpf;
	pd_entry_t *pde;

	pde = pmap_pde(pmap, va);

	if (pmap && pmap_pde_v(pmap_pde(pmap, va))) {

		/* are we current address space or kernel? */
		if (pmap->pm_pdir[PTDPTDI] == PTDpde || pmap == kernel_pmap()) {
			return ((pt_entry_t *) vtopte(va));
		} else {
			/* otherwise, we are alternate address space */
			if (pmap->pm_pdir[PTDPTDI] != APTDpde) {
				APTDpde = pmap->pm_pdir[PTDPTDI];
				tlbflush();
			}
			newpf = *pde & PG_FRAME;
			if ((*PMAP2 & PG_FRAME) != newpf) {
				*PMAP2 = newpf | PG_RW | PG_V | PG_A | PG_M;
				pmap_invalidate_page(kernel_pmap(), (vm_offset_t)PADDR2);
			}
			return ((pt_entry_t *)(avtopte(va)));
		}
	}
	return(0);
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */

vm_offset_t
pmap_extract(pmap, va)
	register pmap_t	pmap;
	vm_offset_t va;
{
	register vm_offset_t pa;

#ifdef DEBUGx
	if (pmapdebug & PDB_FOLLOW)
		pg("pmap_extract(%x, %x) -> ", pmap, va);
#endif
	pa = 0;
	if (pmap && pmap_pde_v(pmap_pde(pmap, va))) {
		pa = *(int *) pmap_pte(pmap, va);
	}
	if (pa)
		pa = (pa & PG_FRAME) | (va & ~PG_FRAME);
#ifdef DEBUGx
	if (pmapdebug & PDB_FOLLOW)
		printf("%x\n", pa);
#endif
	return(pa);
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
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy(%x, %x, %x, %x, %x)", dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif
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
pmap_update()
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_update()");
#endif
	tlbflush();
}

/*
 *	Routine:	pmap_collect
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
	pmap_t		pmap;
{
	register vm_offset_t pa;
	register pv_entry_t pv;
	register int *pte;
	vm_offset_t kpa;
	int s;

#ifdef DEBUG
	int *pde;
	int opmapdebug;
	printf("pmap_collect(%x) ", pmap);
#endif
	if (pmap != kernel_pmap())
		return;
}

/* [ macro again?, should I force kstack into user map here? -wfj ] */
void
pmap_activate(pmap, pcbp)
	register pmap_t pmap;
	struct pcb *pcbp;
{
int x;
#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PDRTAB))
		pg("pmap_activate(%x, %x) ", pmap, pcbp);
#endif
	PMAP_ACTIVATE(pmap, pcbp);
/*printf("pde ");
for(x=0x3f6; x < 0x3fA; x++)
	printf("%x ", pmap->pm_pdir[x]);*/
/*pads(pmap);*/
/*pg(" pcb_cr3 %x", pcbp->pcb_cr3);*/
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

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_zero_page(%x)", phys);
#endif
	phys >>= PG_SHIFT;
	ix = 0;
	do {
		clearseg(phys++);
	} while (++ix != i386pagesperpage);
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

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%x, %x)", src, dst);
#endif
	src >>= PG_SHIFT;
	dst >>= PG_SHIFT;
	ix = 0;
	do {
		physcopyseg(src++, dst++);
	} while (++ix != i386pagesperpage);
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
	boolean_t	pageable;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_pageable(%x, %x, %x, %x)", pmap, sva, eva, pageable);
#endif
	/*
	 * If we are making a PT page pageable then all valid
	 * mappings must be gone from that page.  Hence it should
	 * be all zeros and there is no need to clean it.
	 * Assumptions:
	 *	- we are called with only one page at a time
	 *	- PT pages have only one pv_table entry
	 */
	if (pmap == kernel_pmap() && pageable && sva + PAGE_SIZE == eva) {
		register pv_entry_t pv;
		register vm_offset_t pa;

#ifdef DEBUG
		if ((pmapdebug & (PDB_FOLLOW|PDB_PTPAGE)) == PDB_PTPAGE)
			printf("pmap_pageable(%x, %x, %x, %x)", pmap, sva, eva, pageable);
#endif
		/*if (!pmap_pde_v(pmap_pde(pmap, sva)))
			return;*/
		if(pmap_pte(pmap, sva) == 0)
			return;
		pa = pmap_pte_pa(pmap_pte(pmap, sva));
		if (pa < vm_first_phys || pa >= vm_last_phys)
			return;
		pv = pa_to_pvh(pa);
		/*if (!ispt(pv->pv_va))
			return;*/
#ifdef DEBUG
		if (pv->pv_va != sva || pv->pv_next) {
			pg("pmap_pageable: bad PT page va %x next %x\n", pv->pv_va, pv->pv_next);
			return;
		}
#endif
		/*
		 * Mark it unmodified to avoid pageout
		 */
		pmap_clear_modify(pa);
#ifdef needsomethinglikethis
		if (pmapdebug & PDB_PTPAGE)
			pg("pmap_pageable: PT page %x(%x) unmodified\n", sva, *(int *)pmap_pte(pmap, sva));
		if (pmapdebug & PDB_WIRING)
			pmap_check_wiring("pageable", sva);
#endif
	}
}

/*
 *	Clear the modify bits on the specified physical page.
 */

void
pmap_clear_modify(pa)
	vm_offset_t	pa;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%x)", pa);
#endif
	pmap_changebit(pa, PG_M, FALSE);
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */

void pmap_clear_reference(pa)
	vm_offset_t	pa;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%x)", pa);
#endif
	pmap_changebit(pa, PG_U, FALSE);
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */

boolean_t
pmap_is_referenced(pa)
	vm_offset_t	pa;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		boolean_t rv = pmap_testbit(pa, PG_U);
		printf("pmap_is_referenced(%x) -> %c", pa, "FT"[rv]);
		return(rv);
	}
#endif
	return(pmap_testbit(pa, PG_U));
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */

boolean_t
pmap_is_modified(pa)
	vm_offset_t	pa;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		boolean_t rv = pmap_testbit(pa, PG_M);
		printf("pmap_is_modified(%x) -> %c", pa, "FT"[rv]);
		return(rv);
	}
#endif
	return(pmap_testbit(pa, PG_M));
}

vm_offset_t
pmap_phys_address(ppn)
	int ppn;
{
	return(i386_ptob(ppn));
}

/*
 * Miscellaneous support routines follow
 */
void
i386_protection_init()
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

static
boolean_t
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
	if (pmap_attributes[pa_index(pa)] & bit) {
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
			} while (++ix != i386pagesperpage);
		}
	}
	splx(s);
	return (FALSE);
}

void
pmap_changebit(pa, bit, setem)
	register vm_offset_t pa;
	int bit;
	boolean_t setem;
{
	register pv_entry_t pv;
	register int *pte, npte, ix;
	vm_offset_t va;
	int s;
	boolean_t firstpage = TRUE;

#ifdef DEBUG
	if (pmapdebug & PDB_BITS)
		printf("pmap_changebit(%x, %x, %s)", pa, bit, setem ? "set" : "clear");
#endif
	if (pa < vm_first_phys || pa >= vm_last_phys)
		return;

	pv = pa_to_pvh(pa);
	s = splimp();
	/*
	 * Clear saved attributes (modify, reference)
	 */
	if (!setem)
		pmap_attributes[pa_index(pa)] &= ~bit;
	/*
	 * Loop over all current mappings setting/clearing as appropos
	 * If setting RO do we need to clear the VAC?
	 */
	if (pv->pv_pmap != NULL) {
#ifdef DEBUG
		int toflush = 0;
#endif
		for (; pv; pv = pv->pv_next) {
#ifdef DEBUG
			toflush |= (pv->pv_pmap == kernel_pmap()) ? 2 : 1;
#endif
			va = pv->pv_va;

			/*
			 * XXX don't write protect pager mappings
			 */
			if (bit == PG_RO) {
				extern vm_offset_t pager_sva, pager_eva;

				if (va >= pager_sva && va < pager_eva)
					continue;
			}

			pte = (int*) pmap_pte(pv->pv_pmap, va);
			ix = 0;
			do {
				if (setem)
					npte = *pte | bit;
				else
					npte = *pte & ~bit;
				if (*pte != npte) {
					*pte = npte;
					/*TBIS(va);*/
				}
				va += I386_PAGE_SIZE;
				pte++;
			} while (++ix != i386pagesperpage);

			if (pv->pv_pmap == &curproc->p_vmspace->vm_pmap)
				pmap_activate(pv->pv_pmap, (struct pcb*) curproc->p_addr);
		}
#ifdef somethinglikethis
		if (setem && bit == PG_RO && (pmapvacflush & PVF_PROTECT)) {
			if ((pmapvacflush & PVF_TOTAL) || toflush == 3)
				DCIA();
			else if (toflush == 2)
				DCIS();
			else
				DCIU();
		}
#endif
	}
	splx(s);
}

#ifdef DEBUG
void
pmap_pvdump(pa)
	vm_offset_t pa;
{
	register pv_entry_t pv;

	printf("pa %x", pa);
	for (pv = pa_to_pvh(pa); pv; pv = pv->pv_next) {
		printf(" -> pmap %x, va %x, flags %x", pv->pv_pmap, pv->pv_va,
				pv->pv_flags);
		pmap_pads(pv->pv_pmap);
	}
	printf(" ");
}

#ifdef notyet
void
pmap_check_wiring(str, va)
	char *str;
	vm_offset_t va;
{
	vm_map_entry_t entry;
	register int count, *pte;

	va = trunc_page(va);
	if (!pmap_pde_v(pmap_pde(kernel_pmap(), va)) ||
	!pmap_pte_v(pmap_pte(kernel_pmap(), va)))
		return;

	if (!vm_map_lookup_entry(pt_map, va, &entry)) {
		pg("wired_check: entry for %x not found\n", va);
		return;
	}
	count = 0;
	for (pte = (int*) va; pte < (int*) (va + PAGE_SIZE); pte++)
		if (*pte)
			count++;
	if (entry->wired_count != count)
		pg("*%s*: %x: w%d/a%d\n", str, va, entry->wired_count, count);
}
#endif

/* print address space of pmap*/
void
pmap_pads(pm)
	pmap_t pm;
{
	unsigned va, i, j;
	register int *ptep;

	if (pm == kernel_pmap())
		return;
	for (i = 0; i < 1024; i++) {
		if (pm->pm_pdir[i].pd_v) {
			for (j = 0; j < 1024; j++) {
				va = (i << 22) + (j << 12);
				if (pm == kernel_pmap() && va < 0xfe000000)
					continue;
				if (pm != kernel_pmap() && va > UPT_MAX_ADDRESS)
					continue;
				ptep = pmap_pte(pm, va);
				if (pmap_pte_v(ptep))
					printf("%x:%x ", va, *(int*) ptep);
			};
		}
	}
}
#endif

static u_int
pmap_get_kcr3(void)
{
#ifdef PMAP_PAE_COMP
	return ((u_int)IdlePDPT);
#else
	return ((u_int)IdlePTD);
#endif
}

static u_int
pmap_get_cr3(pmap)
	pmap_t pmap;
{
#ifdef PMAP_PAE_COMP
	return ((u_int)vtophys(pmap->pm_pdir));
#else
	return ((u_int)vtophys(pmap->pm_pdir));
#endif
}

static caddr_t
pmap_cmap3(pa, pte_bits)
	caddr_t pa;
	u_int pte_bits;
{
	pt_entry_t *pte;

	pte = CMAP3;
	*pte = pa | pte_bits;
	//invltlb();
	return (CADDR3);
}

struct bios16_pmap_handle {
	pt_entry_t	*pte;
	pd_entry_t	*ptd;
	pt_entry_t	orig_ptd;
};

static void *
pmap_bios16_enter(void)
{
	struct bios16_pmap_handle *h;
	extern int IdlePTD;

	/*
	 * no page table, so create one and install it.
	 */
	h = malloc(sizeof(struct bios16_pmap_handle), M_TEMP, M_WAITOK);
	h->pte = (pt_entry_t *) malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
	h->ptd = IdlePTD;
	*h->pte = vm86phystk | PG_RW | PG_V;
	h->orig_ptd = *h->ptd;
	*h->ptd = vtophys(h->pte) | PG_RW | PG_V;
	pmap_invalidate_all(kernel_pmap()); /* XXX insurance for now */
	return (h);
}

static void
pmap_bios16_leave(void *arg)
{
	struct bios16_pmap_handle *h;

	h = arg;
	*h->ptd = h->orig_ptd; /* remove page table */
	/*
	 * XXX only needs to be invlpg(0) but that doesn't work on the 386
	 */
	pmap_invalidate_all(kernel_pmap());
	free(h->pte, M_TEMP); /* ... and free it */
}

#ifdef SMP
/*
 * For SMP, these functions have to use the IPI mechanism for coherence.
 */
__inline void
pmap_invalidate_page(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	u_int cpumask;

	if (smp_started) {
		if (!(read_eflags() & PSL_I)) {
			panic("%s: interrupts disabled", __func__);
		}
	}

	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap() || pmap->pm_active == all_cpus) {
		invlpg(va);
	} else {
		cpumask = __PERCPU_GET(cpumask);
		if (pmap->pm_active & cpumask) {
			invlpg(va);
		} else {
			smp_masked_invlpg(cpumask, va, pmap);
		}
	}
}

__inline void
pmap_invalidate_range(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	u_int cpumask;
	vm_offset_t addr;

	if (smp_started) {
		if (!(read_eflags() & PSL_I)) {
			panic("%s: interrupts disabled", __func__);
		}
	}

	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap() || pmap->pm_active == all_cpus) {
		for (addr = sva; addr < eva; addr += PAGE_SIZE) {
			invlpg(addr);
		}
	} else {
		cpumask = __PERCPU_GET(cpumask);
		if (pmap->pm_active & cpumask) {
			for (addr = sva; addr < eva; addr += PAGE_SIZE) {
				invlpg(addr);
			}
		}
		smp_masked_invlpg_range(cpumask, sva, eva, pmap);
	}
}

__inline void
pmap_invalidate_all(pmap)
	pmap_t pmap;
{
	u_int cpumask;

	if (smp_started) {
		if (!(read_eflags() & PSL_I)) {
			panic("%s: interrupts disabled", __func__);
		}
	}

	/*
	 * We need to disable interrupt preemption but MUST NOT have
	 * interrupts disabled here.
	 * XXX we may need to hold schedlock to get a coherent pm_active
	 * XXX critical sections disable interrupts again
	 */
	if (pmap == kernel_pmap() || pmap->pm_active == all_cpus) {
		invltlb();
	} else {
		cpumask = __PERCPU_GET(cpumask);
		if (pmap->pm_active & cpumask) {
			invltlb();
		}
		smp_masked_invltlb(cpumask, pmap);
	}
}
#else /* !SMP */

/*
 * Normal, non-SMP, 486+ invalidation functions.
 * We inline these within pmap.c for speed.
 */
__inline void
pmap_invalidate_page(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	if (pmap == kernel_pmap() || pmap->pm_active) {
		invlpg(va);
	}
}

__inline void
pmap_invalidate_range(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	vm_offset_t addr;

	if (pmap == kernel_pmap() || pmap->pm_active) {
		for (addr = sva; addr < eva; addr += PAGE_SIZE) {
			invlpg(addr);
		}
	}
}

__inline void
pmap_invalidate_all(pmap)
	pmap_t pmap;
{
	if (pmap == kernel_pmap() || pmap->pm_active) {
		invltlb();
	}
}
#endif /* !SMP */

/******************** TLB shootdown code ********************/

/*
 * TLB Shootdown:
 *
 * When a mapping is changed in a pmap, the TLB entry corresponding to
 * the virtual address must be invalidated on all processors.  In order
 * to accomplish this on systems with multiple processors, messages are
 * sent from the processor which performs the mapping change to all
 * processors on which the pmap is active.  For other processors, the
 * ASN generation numbers for that processor is invalidated, so that
 * the next time the pmap is activated on that processor, a new ASN
 * will be allocated (which implicitly invalidates all TLB entries).
 *
 * Shootdown job queue entries are allocated using a simple special-
 * purpose allocator for speed.
 */

/*
 * Initialize the TLB shootdown queues.
 */
void
pmap_tlb_init()
{
	int i;
	simple_lock_init(&pmap_tlb_shootdown_job_lock, "pmap_tlb_shootdown_job_lock");
	for (i = 0; i < NCPUS; i++) {
		TAILQ_INIT(&pmap_tlb_shootdown_q[i].pq_head);
		simple_lock_init(&pmap_tlb_shootdown_q[i].pq_slock, "pmap_tlb_shootdown_lock");
	}

	/*
	 * ensure the TLB is sync'd with reality by flushing it...
	 */
	tlbflush();
}

void
pmap_tlb_shootnow(pmap, cpumask)
	pmap_t 	pmap;
	int32_t cpumask;
{
	struct cpu_info *self;
#ifdef SMP
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	int s;
#endif
#ifdef DIAGNOSTIC
	int count = 0;
#endif

	if (cpumask == 0) {
		return;
	}

	self = curcpu();
#ifdef SMP
	s = splipi();
	self->cpu_tlb_ipi_mask = cpumask;
#endif
	pmap_do_tlb_shootdown(pmap, self);

#ifdef SMP
	splx(s);

	/*
	 * Send the TLB IPI to other CPUs pending shootdowns.
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci == self)
			continue;
		if (cpumask & (1U << ci->cpu_cpuid)) {
			if (i386_send_ipi(ci, I386_IPI_TLB) != 0) {
				i386_atomic_clearbits_l(&self->cpu_tlb_ipi_mask, (1U << ci->cpu_cpuid));
			}
		}
	}
	while (self->cpu_tlb_ipi_mask != 0) {
#ifdef DIAGNOSTIC
		if (count++ > 10000000) {
			panic("TLB IPI rendezvous failed (mask %x)", self->cpu_tlb_ipi_mask);
		}
#endif
		ia32_pause();
	}
#endif
}

/*
 * pmap_tlb_shootdown:
 *
 *	Cause the TLB entry for pmap/va to be shot down.
 */
void
pmap_tlb_shootdown(pmap, addr1, addr2, mask)
	pmap_t 		pmap;
	vm_offset_t addr1, addr2;
	int32_t 	*mask;
{
	struct cpu_info *ci, *self;
	struct pmap_tlb_shootdown_q *pq;
	struct pmap_tlb_shootdown_job *pj;
	CPU_INFO_ITERATOR cii;
	int s;


	if (pmap_initialized == FALSE /*|| cpus_attached == 0*/) {
		invlpg(addr1);
		return;
	}

	self = curcpu();

	s = splipi();

#if 0
	printf("dshootdown %lx\n", addr1);
#endif

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (pmap == kernel_pmap() || pmap->pm_active) {
			continue;
		}
		if (ci != self && !(ci->cpu_flags & CPUF_RUNNING)) {
			continue;
		}

		pq = &pmap_tlb_shootdown_q[ci->cpu_cpuid];
		simple_lock(&pq->pq_slock);

		/*
		 * If there's a global flush already queued, or a
		 * non-global flush, and this pte doesn't have the G
		 * bit set, don't bother.
		 */
		if (pq->pq_flush > 0) {
			simple_unlock(&pq->pq_slock);
			continue;
		}
#ifdef I386_CPU
		if (cpu_class == CPUCLASS_386) {
			pq->pq_flush++;
			*mask |= 1U << ci->ci_cpuid;
			simple_unlock(&pq->pq_slock);
			continue;
		}
#endif
		pj = pmap_tlb_shootdown_job_get(pq);
		//pq->pq_pte |= pte;
		if (pj == NULL) {
			/*
			* Couldn't allocate a job entry.
 			* Kill it now for this CPU, unless the failure
 			* was due to too many pending flushes; otherwise,
 			* tell other cpus to kill everything..
 			*/
			if (ci == self && pq->pq_count < PMAP_TLB_MAXJOBS) {
				invlpg(addr1);
				simple_unlock(&pq->pq_slock);
				continue;
			} else {
				pmap_tlb_shootdown_q_drain(pq);
				*mask |= 1U << ci->cpu_cpuid;
			}
		} else {
			pj->pj_pmap = pmap;
			pj->pj_sva = addr1;
			pj->pj_eva = addr2;
			//pj->pj_pte = pte;
			TAILQ_INSERT_TAIL(&pq->pq_head, pj, pj_list);
			*mask |= 1U << ci->cpu_cpuid;
		}
		simple_unlock(&pq->pq_slock);
	}
	splx(s);
}

/*
 * pmap_do_tlb_shootdown:
 *
 *	Process pending TLB shootdown operations for this processor.
 */
void
pmap_do_tlb_shootdown(pmap, self)
	pmap_t pmap;
	struct cpu_info *self;
{
	u_long cpu_id = self->cpu_cpuid;
	struct pmap_tlb_shootdown_q *pq = &pmap_tlb_shootdown_q[cpu_id];
	struct pmap_tlb_shootdown_job *pj;
	int s;
#ifdef SMP
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
#endif

	KASSERT(self == curcpu());

	s = splipi();

	simple_lock(&pq->pq_slock);
	if (pq->pq_flush) {
		tlbflush();
		pq->pq_flush = 0;
		pmap_tlb_shootdown_q_drain(pq);
	} else {
		if (pq->pq_flush) {
			tlbflush();
		}
		while ((pj = TAILQ_FIRST(&pq->pq_head)) != NULL) {
			TAILQ_REMOVE(&pq->pq_head, pj, pj_list);
			if (/*(pj->pj_pte & pmap_pg_g) || */ pj->pj_pmap == kernel_pmap() || pj->pj_pmap == pmap) {
				invlpg(pj->pj_va);
			}
			pmap_tlb_shootdown_job_put(pq, pj);
		}
		pq->pq_flush /*= pq->pq_pte*/ = 0;
	}
#ifdef SMP
	for (CPU_INFO_FOREACH(cii, ci)) {
		i386_atomic_clearbits_l(&ci->cpu_tlb_ipi_mask, (1U << cpu_id));
	}
#endif
	simple_unlock(&pq->pq_slock);

	splx(s);
}

/*s
 * pmap_tlb_shootdown_q_drain:
 *
 *	Drain a processor's TLB shootdown queue.  We do not perform
 *	the shootdown operations.  This is merely a convenience
 *	function.
 *
 *	Note: We expect the queue to be locked.
 */
void
pmap_tlb_shootdown_q_drain(pq)
	struct pmap_tlb_shootdown_q *pq;
{
	struct pmap_tlb_shootdown_job *pj;

	while ((pj = TAILQ_FIRST(&pq->pq_head)) != NULL) {
		TAILQ_REMOVE(&pq->pq_head, pj, pj_list);
		pmap_tlb_shootdown_job_put(pq, pj);
	}
}

/*
 * pmap_tlb_shootdown_job_get:
 *
 *	Get a TLB shootdown job queue entry.  This places a limit on
 *	the number of outstanding jobs a processor may have.
 *
 *	Note: We expect the queue to be locked.
 */
struct pmap_tlb_shootdown_job *
pmap_tlb_shootdown_job_get(pq)
	struct pmap_tlb_shootdown_q *pq;
{
	struct pmap_tlb_shootdown_job *pj;

	if (pq->pq_count >= PMAP_TLB_MAXJOBS) {
		return (NULL);
	}

	simple_lock(&pmap_tlb_shootdown_job_lock);
	if (pj_free == NULL) {
		simple_unlock(&pmap_tlb_shootdown_job_lock);
		return (NULL);
	}
	pj = &pj_free->pja_job;
	pj_free = (union pmap_tlb_shootdown_job_al*) pj_free->pja_job.pj_nextfree;
	simple_unlock(&pmap_tlb_shootdown_job_lock);

	pq->pq_count++;
	return (pj);
}

/*
 * pmap_tlb_shootdown_job_put:
 *
 *	Put a TLB shootdown job queue entry onto the free list.
 *
 *	Note: We expect the queue to be locked.
 */
void
pmap_tlb_shootdown_job_put(pq, pj)
	struct pmap_tlb_shootdown_q 	*pq;
	struct pmap_tlb_shootdown_job 	*pj;
{
#ifdef DIAGNOSTIC
	if (pq->pq_count == 0) {
		panic("pmap_tlb_shootdown_job_put: queue length inconsistency");
	}
#endif
	simple_lock(&pmap_tlb_shootdown_job_lock);
	pj->pj_nextfree = &pj_free->pja_job;
	pj_free = (union pmap_tlb_shootdown_job_al*) pj;
	simple_unlock(&pmap_tlb_shootdown_job_lock);

	pq->pq_count--;
}

/* Pmap Arguments */
struct pmap_args pmap_arg = {
		.pmap_cold_map 				= &pmap_cold_map,
		.pmap_cold_mapident 		= &pmap_cold_mapident,
		.pmap_remap_lower 			= &pmap_remap_lower,
		.pmap_cold 					= &pmap_cold,
		.pmap_set_nx 				= &pmap_set_nx,
		.pmap_bootstrap 			= &pmap_bootstrap,
		.pmap_isvalidphys 			= &pmap_isvalidphys,
		.pmap_bootstrap_alloc 		= &pmap_bootstrap_alloc,
		.pmap_init 					= &pmap_init,
		.pmap_map 					= &pmap_map,
		.pmap_create 				= &pmap_create,
		.pmap_pinit 				= &pmap_pinit,
		.pmap_destroy 				= &pmap_destroy,
		.pmap_release 				= &pmap_release,
		.pmap_reference 			= &pmap_reference,
		.pmap_remove 				= &pmap_remove,
		.pmap_remove_all 			= &pmap_remove_all,
		.pmap_copy_on_write 		= &pmap_copy_on_write,
		.pmap_protect 				= &pmap_protect,
		.pmap_enter 				= &pmap_enter,
		.pmap_page_protect 			= &pmap_page_protect,
		.pmap_change_wiring 		= &pmap_change_wiring,
		.pmap_pte 					= &pmap_pte,
		.pmap_extract 				= &pmap_extract,
		.pmap_copy 					= &pmap_copy,
		.pmap_update 				= &pmap_update,
		.pmap_collect 				= &pmap_collect,
		.pmap_activate 				= &pmap_activate,
		.pmap_zero_page 			= &pmap_zero_page,
		.pmap_copy_page 			= &pmap_copy_page,
		.pmap_pageable 				= &pmap_pageable,
		.pmap_clear_modify 			= &pmap_clear_modify,
		.pmap_clear_reference 		= &pmap_clear_reference,
		.pmap_is_referenced 		= &pmap_is_referenced,
		.pmap_is_modified 			= &pmap_is_modified,
		.pmap_phys_address 			= &pmap_phys_address,
		.i386_protection_init 		= &i386_protection_init,
		.pmap_testbit 				= &pmap_testbit,
		.pmap_changebit 			= &pmap_changebit,
		.pmap_pvdump 				= &pmap_pvdump,
		.pmap_check_wiring 			= &pmap_check_wiring,
		.pmap_pads 					= &pmap_pads,
		.pmap_get_kcr3 				= &pmap_get_kcr3,
		.pmap_get_cr3 				= &pmap_get_cr3,
		.pmap_cmap3 				= &pmap_cmap3,
		.pmap_bios16_enter 			= &pmap_bios16_enter,
		.pmap_bios16_leave 			= &pmap_bios16_leave,
		.pmap_invalidate_page 		= &pmap_invalidate_page,
		.pmap_invalidate_range 		= &pmap_invalidate_range,
		.pmap_invalidate_all 		= &pmap_invalidate_all,
		.pmap_tlb_init 				= &pmap_tlb_init,
		.pmap_tlb_shootnow 			= &pmap_tlb_shootnow,
		.pmap_tlb_shootdown 		= &pmap_tlb_shootdown,
		.pmap_do_tlb_shootdown 		= &pmap_do_tlb_shootdown,
		.pmap_tlb_shootdown_q_drain = &pmap_tlb_shootdown_q_drain,
		.pmap_tlb_shootdown_job_get = &pmap_tlb_shootdown_job_get,
		.pmap_tlb_shootdown_job_put = &pmap_tlb_shootdown_job_put
};
