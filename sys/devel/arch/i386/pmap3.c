/*
 * pmap3.c
 *
 *  Created on: 22 Oct 2020
 *      Author: marti
 */
#include <sys/msgbuf.h>

#include <vm/include/vm.h>
#include <vm/include/vm_param.h>

#include <arch/i386/include/param.h>
#include <arch/i386/include/specialreg.h>

#include <pte3.h>
#include <pmap3.h>
#include <pmap_nopae3.h>

#define	pmap_pde(m, v)				(&((m)->pm_pdir[((vm_offset_t)(v) >> PD_SHIFT)&1023]))

#define pmap_pte_pa(pte)			(*(int *)(pte) & PG_FRAME)
#define pmap_pde_v(pte)				(*(int *)(pte) & PG_V)
#define pmap_pte_w(pte)				(*(int *)(pte)& PG_W)
/* #define pmap_pte_ci(pte)			((pte)->pg_ci) */
#define pmap_pte_m(pte)				(*(int *)(pte) & PG_M)
#define pmap_pte_u(pte)				(*(int *)(pte) & PG_A)
#define pmap_pte_v(pte)				(*(int *)(pte) & PG_V)
#define pmap_pte_set_w(pte, v)		((pte) & PG_W)
#define pmap_pte_set_prot(pte, v)	((pte) & PG_PROT)

static int pgeflag = 0;		/* PG_G or-in */
static int pseflag = 0;		/* PG_PS or-in */

static int nkpt = NKPT;

extern int pat_works;
extern int pg_ps_enabled;

extern int elf32_nxstack;

#define	PAT_INDEX_SIZE	8
static int pat_index[PAT_INDEX_SIZE];	/* cache mode to PAT index conversion */

extern int elf32_nxstack;

extern char 		_end[];
extern u_long 		physfree;		/* phys addr of next free page */
extern u_long 		vm86phystk;		/* PA of vm86/bios stack */
extern u_long 		vm86paddr;		/* address of vm86 region */
extern int 			vm86pa;			/* phys addr of vm86 region */

vm_offset_t 		avail_start;	/* PA of first available physical page */
vm_offset_t			avail_end;		/* PA of last available physical page */
vm_size_t			mem_size;		/* memory size in bytes */
vm_offset_t			virtual_avail;  /* VA of first avail page (after kernel bss)*/
vm_offset_t			virtual_end;	/* VA of last avail page (end of kernel AS) */
vm_offset_t			vm_first_phys;	/* PA of first managed page */
vm_offset_t			vm_last_phys;	/* PA just past last managed page */

#ifdef PMAP_PAE_COMP
pd_entry_t 			*IdlePTD_pae;	/* phys addr of kernel PTD */
pdpt_entry_t 		*IdlePDPT;		/* phys addr of kernel PDPT */
pt_entry_t 			*KPTmap_pae;	/* address of kernel page tables */
#define	IdlePTD		IdlePTD_pae
#define	KPTmap		KPTmap_pae
#else
pd_entry_t 			*IdlePTD_nopae;
pt_entry_t 			*KPTmap_nopae;
#define	IdlePTD		IdlePTD_nopae
#define	KPTmap		KPTmap_nopae
#endif
extern u_long 		KPTphys;		/* phys addr of kernel page tables */
extern u_long 		tramp_idleptd;

/*
 * All those kernel PT submaps that BSD is so fond of
 */
static pd_entry_t 	*KPTD;
pt_entry_t			*CMAP1, *CMAP2, *CMAP3, *mmap;
caddr_t				CADDR1, CADDR2, CADDR3, vmmap;
pt_entry_t			*msgbufmap;
struct msgbuf		*msgbufp;

static pt_entry_t *PMAP1 = NULL, *PMAP2, *PMAP3;
static pt_entry_t *PADDR1 = NULL, *PADDR2, *PADDR3;

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
pmap_remap_lower(bool enable)
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
	proc0kstack = allocpages(TD0_KSTACK_PAGES, &physfree);

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
	pmap_cold_mapident(proc0kstack, TD0_KSTACK_PAGES);
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
#ifdef PMAP_PAE_COMP
		i386_pmap_VM_NFREEORDER = VM_NFREEORDER_PAE;
		i386_pmap_VM_LEVEL_0_ORDER = VM_LEVEL_0_ORDER_PAE;
		i386_pmap_PDRSHIFT = PDRSHIFT_PAE;
#else
	i386_pmap_VM_NFREEORDER = VM_NFREEORDER_NOPAE;
	i386_pmap_VM_LEVEL_0_ORDER = VM_LEVEL_0_ORDER_NOPAE;
	i386_pmap_PDRSHIFT = PDRSHIFT_NOPAE;
#endif
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
	struct pcpu *pc;
	u_long res;
	int i;

	res = atop(firstaddr - (vm_offset_t)KERNLOAD);

	avail_start = firstaddr;
	avail_end = maxmem << PGSHIFT;

	/* XXX: allow for msgbuf */
	avail_end -= i386_round_page(sizeof(struct msgbuf));

	mem_size = physmem << PGSHIFT;

	virtual_avail = (vm_offset_t)firstaddr;
	virtual_end = VM_MAX_KERNEL_ADDRESS;

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
	kernel_pmap->pm_pdpt = IdlePDPT;
#endif
	simple_lock_init(&kernel_pmap->pm_lock);
	kernel_pmap->pm_count = 1;
	kernel_pmap->pm_stats.resident_count = res;

#define	SYSMAP(c, p, v, n)	\
	v = (c)va; va += ((n)*I386_PAGE_SIZE); p = pte; pte += (n);

	va = virtual_avail;
	pte = vtopte(va);

	SYSMAP(caddr_t, CMAP1, CADDR1, 1)
	SYSMAP(caddr_t, CMAP2, CADDR2, 1)
	SYSMAP(caddr_t, CMAP3, CADDR3, 1)
	SYSMAP(caddr_t, mmap, vmmap, 1)
	SYSMAP(struct msgbuf *, msgbufmap, msgbufp, 1)
	//SYSMAP(struct msgbuf *, unused, msgbufp, atop(round_page(msgbufsize)))

	/*
	 * KPTmap is used by pmap_kextract().
	 *
	 * KPTmap is first initialized by pmap_cold().  However, that initial
	 * KPTmap can only support NKPT page table pages.  Here, a larger
	 * KPTmap is created that can support KVA_PAGES page table pages.
	 */
	SYSMAP(pt_entry_t*, KPTD, KPTmap, KVA_PAGES)

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

		while (!pmap_isvalidphys(avail_start))
				avail_start += PAGE_SIZE;

		virtual_avail = pmap_map(virtual_avail, avail_start, avail_start + PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE);
		avail_start += PAGE_SIZE;
	}

	blkclr ((caddr_t) val, size);
	return ((void *) val);
}

void
pmap_virtual_space(startp, endp)
	vaddr_t *startp;
	vaddr_t *endp;
{
	*startp = virtual_avail;
	*endp = virtual_end;
}

/*
 * pmap_bootstrap_valloc: allocate a virtual address in the bootstrap area.
 * This function is to be used before any VM system has been set up.
 *
 * The va is taken from virtual_avail.
 */
static vaddr_t
pmap_bootstrap_valloc(size_t npages)
{
	vaddr_t va = virtual_avail;
	virtual_avail += npages * PAGE_SIZE;
	return (va);
}

/*
 * pmap_bootstrap_palloc: allocate a physical address in the bootstrap area.
 * This function is to be used before any VM system has been set up.
 *
 * The pa is taken from avail_start.
 */
static caddr_t
pmap_bootstrap_palloc(size_t npages)
{
	caddr_t pa = avail_start;
	avail_start += npages * PAGE_SIZE;
	return (pa);
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
	if (cpu_vendor_id == CPU_VENDOR_INTEL &&
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

/* TODO: fix vm_page_array_size & KPTDI
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(phys_start, phys_end)
	vm_offset_t	phys_start, phys_end;
{
	vm_offset_t addr;
	vm_page_t mpte;
	vm_size_t npg, s;

	int i;

	for (i = 0; i < NKPT; i++) {
		mpte = PHYS_TO_VM_PAGE(KPTphys + ptoa(i));
		KASSERT(mpte >= vm_page_array && mpte < &vm_page_array[i], ("pmap_init: page table page is out of range"));
		mpte->offset = i + KPTDI;
		mpte->phys_addr = KPTphys + ptoa(i);
	}

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_init(%x, %x)\n", phys_start, phys_end);
#endif

	/*
	 * Allocate memory for random pmap data structures.  Includes the
	 * pv_head_table and pmap_attributes.
	 */
	npg = atop(phys_end - phys_start);
	s = (vm_size_t) (sizeof(struct pv_entry) * npg + npg);
	s = round_page(s);
	pv_table = (pv_entry_t) kmem_alloc(kernel_map, s);
	addr = (vm_offset_t) pv_table;
	addr += sizeof(struct pv_entry) * npg;
	pmap_attributes = (char *) addr;
#ifdef DEBUG
	if (pmapdebug & PDB_INIT)
		printf("pmap_init: %x bytes (%x pgs): tbl %x attr %x\n", s, npg, pv_table, pmap_attributes);
#endif

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
	while (start < end) {
		pmap_enter(kernel_pmap, virt, start, prot, FALSE);
		virt += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	return(virt);
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
	bcopy(PTD+KPTDI_FIRST, pmap->pm_pdir+KPTDI_FIRST, (KPTDI_LAST-KPTDI_FIRST+1)*4);

	/* install self-referential address mapping entry */
	*(int *)(pmap->pm_pdir+PTDPTDI) = (int)pmap_extract(kernel_pmap, (vm_offset_t)pmap->pm_pdir) | PG_V | PG_URKW;

	pmap->pm_count = 1;
	simple_lock_init(&pmap->pm_lock);
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
	if (pmap == NULL)
		return;

	simple_lock(&pmap->pm_lock);
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
