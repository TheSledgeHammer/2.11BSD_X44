/*
 * pmap2.c
 *
 *  Created on: 13 Mar 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

#include <arch/i386/include/param.h>

#include <sys/msgbuf.h>
#include "pmap/cpt.h"

/*
 * All those kernel PT submaps that BSD is so fond of
 */
struct pte			*CMAP1, *CMAP2, *mmap;
caddr_t				CADDR1, CADDR2, vmmap;
struct pte			*msgbufmap;
struct msgbuf		*msgbufp;

#define	pmap_pde(m, v)					(&((m)->pm_pdir[((vm_offset_t)(v) >> PD_SHIFT)&1023]))

#define	LOWPTDI		1 					/* No PAE */

extern u_long 		KERNend;			/* phys addr end of kernel (just after bss) */

/* New For PDE & PTE Mapping */
extern u_long 		physfree;			/* phys addr of next free page */
extern u_long 		KCPTphys;			/* phys addr of kernel clustered page table */

extern u_long 		IdleCPT;
extern cpt_entry_t 	*KCPTmap;			/* Kernel Clustered Page Table Mapping */
extern cpte_entry_t *KCPTEmap;			/* Kernel Clustered Page Table Entry Mapping */

#define NPGPTD		4					/* Num of pages for page directory */


/*
 * Get PDEs and PTEs for user/kernel address space
 */
#define	pmap_cpt(m, v)				(&((m)->pm_cpt[VPBN(((vm_offset_t)(v))])
#define	pmap_cpte_pa(cpte)			(*(int *)(cpte)->cpte_pte & PG_FRAME)

#define	pmap_cpt_v(cpte)			((cpte)->cpte_pte->pd_v)

#define pmap_pte_pa(pte)			(*(int *)(pte) & PG_FRAME)

#define pmap_pde_v(pte)				((pte)->pd_v)
#define pmap_pte_w(pte)				((pte)->pg_w)
/* #define pmap_pte_ci(pte)			((pte)->pg_ci) */
#define pmap_pte_m(pte)				((pte)->pg_m)
#define pmap_pte_u(pte)				((pte)->pg_u)
#define pmap_pte_v(pte)				((pte)->pg_v)
#define pmap_pte_set_w(pte, v)		((pte)->pg_w = (v))
#define pmap_pte_set_prot(pte, v)	((pte)->pg_prot = (v))

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
allocate_kern_cpt(cpt, cpte)
	cpt_entry_t *cpt;
	cpte_entry_t *cpte;
{
	u_long a;

	/* Allocate Kernel Page Tables */
	KCPTphys = allocpages(NKPT, &physfree);

	/* Allocate Page Table Directory */
	IdleCPT = allocpages(NPGPTD, &physfree);

	allocpages(1, &physfree);
	proc0paddr = allocpages(P0_KSTACK_PAGES, &physfree);
	cpte->cpte_pte
	/* Install page tables into PTD. */
	for (a = 0; a < NKPT; a++) {
		IdleCPT = (KCPTphys + ptoa(a)) | PG_V | PG_RW | PG_A | PG_M;
		cpt_add(cpt, cpte, IdleCPT);
	}
	KCPTmap = cpt_lookup(cpt, IdleCPT);
}

static void
allocate_kern_cpte(pte, entry, boff)
	pt_entry_t *pte;
    unsigned long entry;
    int boff;
{
    /* Install CPTE's */
    if(KCPTmap != NULL) {
    	KCPTEmap = cpt_lookup_cpte(KCPTmap, VPBN(entry));
        cpte_add(KCPTEmap, pte, boff);
    }
}

static void
pmap_cold_map(cpte, pa, va, cnt)
	cpte_entry_t *cpte;
	u_long pa, va, cnt;
{
	pt_entry_t *pt;

	for (pt = (pt_entry_t *)KCPTphys + atop(va); cnt > 0; cnt--, pt++, va += PAGE_SIZE, pa += PAGE_SIZE) {
		*pt = pa | PG_V | PG_RW | PG_A | PG_M;
		allocate_kern_cpte(cpte, pt, cnt);
	}
}

static void
pmap_cold_mapident(cpte, pa, cnt)
	cpte_entry_t *cpte;
	u_long pa, cnt;
{
	pmap_cold_map(cpte, pa, pa, cnt);
}

void
pmap_cold()
{
	cpt_entry_t *cpt;
	cpte_entry_t *cpte;
	pd_entry_t *pd;
	pt_entry_t *pt;
	u_int cr3, ncr4;

	RB_INIT(&cpt_root);
	RB_INIT(&cpte_root);

	cpt = &cpt_base[0];
	cpte = &cpte_base[0];

	/* Kernel Page Table */
	pd = cpt_to_pde(cpt, 0);
	pt = cpte_to_pte(cpte, 0);

	allocate_kern_cpt(cpt, cpte);

	pmap_cold_mapident(cpte, 0, atop(NBPDR) * LOWPTDI);
	pmap_cold_map(cpte, 0, NBPDR * LOWPTDI, atop(NBPDR) * LOWPTDI);
	pmap_cold_mapident(KERNBASE, atop(KERNend - KERNBASE));

	pmap_cold_mapident(cpte, IdleCPT, NPGPTD);

	/* Map early KPTmap.  It is really pmap_cold_mapident. */
	pmap_cold_map(cpte, KCPTphys, KCPTmap->cpt_pa_addr, KCPTmap->cpt_va_addr, NKPT);

	/* Map proc0addr */
	pmap_cold_mapident(cpte, proc0paddr, P0_KSTACK_PAGES);
}


extern vm_offset_t	atdevbase;
void
pmap_bootstrap(firstaddr, loadaddr)
	vm_offset_t firstaddr;
	vm_offset_t loadaddr;
{
	vm_offset_t va;
	struct pte *pte;
	extern vm_offset_t maxmem, physmem;

	avail_start = firstaddr;
	avail_end = maxmem << PG_SHIFT;

	/* XXX: allow for msgbuf */
	avail_end -= i386_round_page(sizeof(struct msgbuf));

	mem_size = physmem << PG_SHIFT;
	virtual_avail = atdevbase + 0x100000 - 0xa0000 + 10 * NBPG;
	virtual_end = VM_MAX_KERNEL_ADDRESS;
	i386pagesperpage = PAGE_SIZE / I386_PAGE_SIZE;

	/*
	 * Initialize protection array.
	 */
	i386_protection_init();

	kernel_pmap.pm_cpt = cpt_lookup(cpt_base, (0xfe000000 + IdleCPT));

	simple_lock_init(&kernel_pmap->pm_lock);
	kernel_pmap->pm_count = 1;
/*
 * Allocate all the submaps we need
 */
#define	SYSMAP(c, p, v, n)	\
		v = (c)va; va += ((n)*I386_PAGE_SIZE); p = pte; pte += (n);

	va = virtual_avail;
	pte = pmap_pte(kernel_pmap, va);

	SYSMAP(caddr_t, CMAP1, CADDR1, 1);
	SYSMAP(caddr_t, CMAP2, CADDR2, 1);
	SYSMAP(caddr_t, mmap, vmmap, 1);
	SYSMAP(struct msgbuf *, msgbufmap, msgbufp, 1);

	virtual_avail = va;
}


void
pmap_pinit(pmap)
	register struct pmap *pmap;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		pg("pmap_pinit(%x)\n", pmap);
#endif
	pmap->pm_cpt = (cpt_entry_t *)kmem_alloc(kernel_map, NBPG);

	pmap->pm_count = 1;
	simple_lock_init(&pmap->pm_lock);
}

struct cpte *
pmap_cpte(pmap, va)
	register pmap_t	pmap;
	vm_offset_t va;
{
	for(int i = 0; i < NCPTE; i++) {
		if (cpte_to_pte(cpt_lookup_cpte(pmap->pm_cpt, va), i)->pg_pfnum == PTDpde.pd_pfnum || pmap == kernel_pmap) {
			return (pte_to_cpte((struct pte *) vtopte(va), i));
		} else {
			if (cpte_to_pte(cpt_lookup_cpte(pmap->pm_cpt, va), i)->pg_pfnum != APTDpde.pd_pfnum) {
				APTDpde = cpte_to_pte(cpt_lookup_cpte(pmap->pm_cpt, va), i);
				tlbflush();
			}
			return (pte_to_cpte((struct pte *) vtopte(va), i));
		}
	}
	return (0);
}
