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

#include <dev/i386/cpt.h>
#include <arch/i386/include/pmap.h>

/*
 * Get PDEs and PTEs for user/kernel address space
 */
#define	pmap_pde(m, v)				(&((m)->pm_pdir[((vm_offset_t)(v) >> PD_SHIFT)&1023]))

#define pmap_pte_pa(pte)			(*(int *)(pte) & PG_FRAME)

#define pmap_pde_v(pte)				((pte)->pd_v)
#define pmap_pte_w(pte)				((pte)->pg_w)
/* #define pmap_pte_ci(pte)			((pte)->pg_ci) */
#define pmap_pte_m(pte)				((pte)->pg_m)
#define pmap_pte_u(pte)				((pte)->pg_u)
#define pmap_pte_v(pte)				((pte)->pg_v)
#define pmap_pte_set_w(pte, v)		((pte)->pg_w = (v))
#define pmap_pte_set_prot(pte, v)	((pte)->pg_prot = (v))


struct pmap	kernel_pmap_store;

vm_offset_t avail_start;				/* PA of first available physical page */
vm_offset_t	avail_end;					/* PA of last available physical page */
vm_size_t	mem_size;					/* memory size in bytes */
vm_offset_t	virtual_avail;  			/* VA of first avail page (after kernel bss)*/
vm_offset_t	virtual_end;				/* VA of last avail page (end of kernel AS) */
vm_offset_t	vm_first_phys;				/* PA of first managed page */
vm_offset_t	vm_last_phys;				/* PA just past last managed page */
int			i386pagesperpage;			/* PAGE_SIZE / I386_PAGE_SIZE */
boolean_t	pmap_initialized = FALSE;	/* Has pmap_init completed? */
char		*pmap_attributes;			/* reference and modify bits */


extern u_long physfree;	/* phys addr of next free page */
extern u_long KPTphys;	/* phys addr of kernel page tables */

extern vm_offset_t	atdevbase;
extern vm_offset_t maxmem, physmem;

extern u_long IdlePTD;

#define NPGPTD		4		/* Num of pages for page directory */

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
pmap_cold_map(cpte, pa, va, cnt)
	cpte_entry_t *cpte;
	u_long pa, va, cnt;
{
	pt_entry_t *pt;
	for (pt = (pt_entry_t *)KPTphys + atop(va); cnt > 0; cnt--, pt++, va += PAGE_SIZE, pa += PAGE_SIZE) {
		*pt = pa | PG_V | PG_RW | PG_A | PG_M;
		cpte_add(cpte, pt, cnt);
	}
}

static void
pmap_cold_mapident(cpte, pa, cnt)
	cpte_entry_t *cpte;
	u_long pa, cnt;
{
	pmap_cold_map(cpte, pa, pa, cnt);
}

/* Setup clustered page table: with phys addr of region */
void
allocate_cpt(cpt, cpte)
	cpt_entry_t *cpt;
	cpte_entry_t *cpte;
{
	u_long a;

	KPTphys = allocpages(NKPT, &physfree);
	IdlePTD = allocpages(NPGPTD, &physfree);
	allocpages(1, &physfree);

	proc0addr = allocpages(TD0_KSTACK_PAGES, &physfree);
	vm86phystk = allocpages(1, &physfree);
	vm86paddr = vm86pa = allocpages(3, &physfree);

	cpt_add(cpt, cpte, KPTphys);
	cpt_add(cpt, cpte, IdlePTD);
	cpt_add(cpt, cpte, proc0addr);
	cpt_add(cpt, cpte, vm86phystk);
	cpt_add(cpt, cpte, vm86paddr);


	for (a = 0; a < NKPT; a++) {
		u_long b = (KPTphys + ptoa(a)) | PG_V | PG_RW | PG_A | PG_M;
		cpt_add(cpt, cpte, b);
	}
}

void
pmap_cold()
{
	cpt_entry_t *cpt;
	cpte_entry_t *cpte;
	pt_entry_t *pt;
	u_long a;
	u_int cr3, ncr4;

	RB_INIT(&cpt_root);
	RB_INIT(&cpte_root);

	cpt = &cpt_base[0];
	cpte = &cpte_base[0];

	/* Allocate Kernel Page Tables */
	KPTphys = allocpages(NKPT, &physfree);
	cpt_add(cpt, cpte, KPTphys);
	KPTmap = cpt_lookup(cpt, KPTphys);

	/* Allocate Page Table Directory */
	IdlePTD = allocpages(NPGPTD, &physfree);

	cpt_add(cpt, cpte, IdlePTD);


	pmap_cold_mapident(cpte, 0, atop(NBPDR) * LOWPTDI);
	pmap_cold_map(cpte, 0, NBPDR * LOWPTDI, atop(NBPDR) * LOWPTDI);
	pmap_cold_mapident(cpte, KERNBASE, atop(KERNend - KERNBASE));
}
