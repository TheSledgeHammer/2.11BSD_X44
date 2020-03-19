/*
 * pmap1.c
 *
 *  Created on: 17 Mar 2020
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
#include <dev/i386/pmap2.h>

extern u_long 		physfree;			/* phys addr of next free page */
extern u_long 		KPTphys;			/* phys addr of kernel clustered page table */
extern cpt_entry_t 	*KPTmap;			/* Kernel Clustered Page Table Mapping */

extern u_long 		IdlePTD;

#define NPGPTD		4					/* Num of pages for page directory */

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

	for (pt = (pt_entry_t *)KCPTphys + atop(va); cnt > 0; cnt--, pt++, va += PAGE_SIZE, pa += PAGE_SIZE) {
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
pmap_cold()
{
    pt_entry_t *pt;
	u_long a;
    u_int cr3, ncr4;

    KPTphys = allocpages(NKPT, &physfree);
    KPTmap = (pt_entry_t *)KPTphys;

    IdlePTD = (pd_entry_t *)allocpages(NPGPTD, &physfree);

	allocpages(1, &physfree);
	proc0paddr_kstack = allocpages(P0_KSTACK_PAGES, &physfree);

    for (a = 0; a < NKPT; a++) {
        IdlePTD[a] = (KPTphys + ptoa(a)) | PG_V | PG_RW | PG_A | PG_M;
    }

    for (a = 0; a < NPGPTD; a++) {
        IdlePTD[PTDPTDI + a] = ((u_int)IdlePTD + ptoa(a)) | PG_V | PG_RW;
    }
}

extern vm_offset_t	atdevbase;
void
pmap_bootstrap(firstaddr)
	vm_offset_t firstaddr;
{
	vm_offset_t va;
	struct pte *pte;

	firstaddr=0x100000;
	extern vm_offset_t maxmem, physmem;

	avail_start = firstaddr;
	avail_end = maxmem << PG_SHIFT;

	/* XXX: allow for msgbuf */
	avail_end -= i386_round_page(sizeof(struct msgbuf));

	mem_size = physmem << PG_SHIFT;
	virtual_avail = atdevbase + 0x100000 - 0xa0000 + 10*NBPG;
	virtual_end = VM_MAX_KERNEL_ADDRESS;
	i386pagesperpage = PAGE_SIZE / I386_PAGE_SIZE;

	/*
	 * Initialize protection array.
	 */
	i386_protection_init();

	kernel_pmap->pm_pdir = (pd_entry_t *)(0xfe000000 + IdlePTD);

	simple_lock_init(&kernel_pmap->pm_lock);
	kernel_pmap->pm_count = 1;
}
