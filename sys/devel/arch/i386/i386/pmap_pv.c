/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 * Copyright (c) 1994 John S. Dyson
 * All rights reserved.
 * Copyright (c) 1994 David Greenman
 * All rights reserved.
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
 *	from:	@(#)pmap.c	7.7 (Berkeley)	5/12/91
 *	$Id: pmap.c,v 1.57 1995/05/11 19:26:11 rgrimes Exp $
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
#include <sys/tree.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_param.h>

#include <arch/i386/include/param.h>
#include <arch/i386/include/cpu.h>
#include <arch/i386/include/cputypes.h>
#include <arch/i386/include/cpuvar.h>
#include <arch/i386/include/atomic.h>
#include <arch/i386/include/specialreg.h>
#include <arch/i386/include/vmparam.h>
#include <arch/i386/include/pte.h>

#include <arch/i386/include/pmap.h>

#define PV_FREELIST_MIN ((NBPG / sizeof (struct pv_entry)) / 2)

/*
 * Data for the pv entry allocation mechanism
 */
int pv_freelistcnt;
pv_entry_t pv_freelist;
vm_offset_t pvva;
int npvvapg;

/*
 * free the pv_entry back to the free list
 */
inline static void
free_pv_entry(pv)
	pv_entry_t pv;
{
	if (!pv)
		return;
	++pv_freelistcnt;
	pv->pv_next = pv_freelist;
	pv_freelist = pv;
}

/*
 * get a new pv_entry, allocating a block from the system
 * when needed.
 * the memory allocation is performed bypassing the malloc code
 * because of the possibility of allocations at interrupt time.
 */
static inline pv_entry_t
get_pv_entry()
{
	pv_entry_t tmp;

	/*
	 * get more pv_entry pages if needed
	 */
	if (pv_freelistcnt < PV_FREELIST_MIN || pv_freelist == 0) {
		pmap_alloc_pv_entry();
	}
	/*
	 * get a pv_entry off of the free list
	 */
	--pv_freelistcnt;
	tmp = pv_freelist;
	pv_freelist = tmp->pv_next;
	return tmp;
}

/*
 * this *strange* allocation routine *statistically* eliminates the
 * *possibility* of a malloc failure (*FATAL*) for a pv_entry_t data structure.
 * also -- this code is MUCH MUCH faster than the malloc equiv...
 */
static void
pmap_alloc_pv_entry()
{
	/*
	 * do we have any pre-allocated map-pages left?
	 */
	if (npvvapg) {
		vm_page_t m;

		/*
		 * we do this to keep recursion away
		 */
		pv_freelistcnt += PV_FREELIST_MIN;
		/*
		 * allocate a physical page out of the vm system
		 */
		m = vm_page_alloc(kernel_object, pvva - vm_map_min(kernel_map));
		if (m) {
			int newentries;
			int i;
			pv_entry_t entry;

			newentries = (NBPG / sizeof(struct pv_entry));
			/*
			 * wire the page
			 */
			vm_page_wire(m);
			m->flags &= ~PG_BUSY;
			/*
			 * let the kernel see it
			 */
			pmap_kenter(pvva, VM_PAGE_TO_PHYS(m));

			entry = (pv_entry_t) pvva;
			/*
			 * update the allocation pointers
			 */
			pvva += NBPG;
			--npvvapg;

			/*
			 * free the entries into the free list
			 */
			for (i = 0; i < newentries; i++) {
				free_pv_entry(entry);
				entry++;
			}
		}
		pv_freelistcnt -= PV_FREELIST_MIN;
	}
	if (!pv_freelist)
		panic("get_pv_entry: cannot get a pv_entry_t");
}

/*
 * init the pv_entry allocation system
 */
#define PVSPERPAGE 64
void
init_pv_entries(npg)
	int npg;
{
	/*
	 * allocate enough kvm space for PVSPERPAGE entries per page (lots)
	 * kvm space is fairly cheap, be generous!!!  (the system can panic if
	 * this is too small.)
	 */
	npvvapg = ((npg * PVSPERPAGE) * sizeof(struct pv_entry) + NBPG - 1) / NBPG;
	pvva = kmem_alloc_pageable(kernel_map, npvvapg * NBPG);
	/*
	 * get the first batch of entries
	 */
	free_pv_entry(get_pv_entry());
}

static pt_entry_t *
get_pt_entry(pmap)
	pmap_t pmap;
{
	vm_offset_t frame = (int) pmap->pm_pdir[PTDPTDI] & PG_FRAME;

	/* are we current address space or kernel? */
	if (pmap == kernel_pmap() || frame == ((int) PTDpde & PG_FRAME)) {
		return PTmap;
	}
	/* otherwise, we are alternate address space */
	if (frame != ((int) APTDpde & PG_FRAME)) {
		APTDpde = pmap->pm_pdir[PTDPTDI];
		pmap_update();
	}
	return (APTmap);
}

/*
 * If it is the first entry on the list, it is actually
 * in the header and we must copy the following entry up
 * to the header.  Otherwise we must search the list for
 * the entry.  In either case we free the now unused entry.
 */
void
pmap_remove_entry(pmap, pv, va)
	struct pmap *pmap;
	pv_entry_t pv;
	vm_offset_t va;
{
	pv_entry_t npv;
	int s;

	s = splhigh();
	if (pmap == pv->pv_pmap && va == pv->pv_va) {
		npv = pv->pv_next;
		if (npv) {
			*pv = *npv;
			free_pv_entry(npv);
		} else {
			pv->pv_pmap = NULL;
		}
	} else {
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va) {
				break;
			}
			pv = npv;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
			free_pv_entry(npv);
		}
	}
	splx(s);
}

/*
 * Map a set of physical memory pages into the kernel virtual
 * address space. Return a pointer to where it is mapped. This
 * routine is intended to be used for mapping device memory,
 * NOT real memory. The non-cacheable bits are set on each
 * mapped page.
 */
void *
pmap_mapdev(pa, size)
	vm_offset_t pa;
	vm_size_t size;
{
	vm_offset_t va, tmpva;
	pt_entry_t *pte;

	pa = trunc_page(pa);
	size = roundup(size, PAGE_SIZE);

	va = kmem_alloc_pageable(kernel_map, size);
	if (!va)
		panic("pmap_mapdev: Couldn't alloc kernel virtual memory");

	for (tmpva = va; size > 0;) {
		pte = vtopte(tmpva);
		*pte = (pt_entry_t) ((int) (pa | PG_RW | PG_V | PG_N));
		size -= PAGE_SIZE;
		tmpva += PAGE_SIZE;
		pa += PAGE_SIZE;
	}
	pmap_update();

	return ((void *) va);
}
