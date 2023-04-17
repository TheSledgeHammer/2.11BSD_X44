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
 * pmap code that still needs work
 * or is new and not yet implemented
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
uint64_t 	nbpds[] = NBPD_INITIALIZER;
pd_entry_t 	*pdes[] = PDES_INITIALIZER;
pd_entry_t 	*apdes[] = APDES_INITIALIZER;


/* PL_I does the same thing as pmap_index */
static vm_offset_t
pmap_index(va, lvl)
	vm_offset_t va;
	int lvl;
{
	vm_offset_t index;

	KASSERT(lvl >= 1);

	index = (va >> (((lvl * PTP_SHIFT) + PGSHIFT) - PTP_SHIFT) & ((1ULL << PTP_SHIFT) - 1));
	return (index);
}

vm_offset_t
pmap_pte_index(vm_offset_t va)
{
	return (pmap_index(va, 1));
}

vm_offset_t
pmap_pt_index(vm_offset_t va)
{
	return (pmap_index(va, 2));
}

vm_offset_t
pmap_pd_index(vm_offset_t va)
{
	return (pmap_index(va, 3));
}

vm_offset_t
pmap_pdp_index(vm_offset_t va)
{
	return (pmap_index(va, 4));
}

static pd_entry_t *
pmap_pdei(va, lvl)
	vm_offset_t va;
	int lvl;
{
    pd_entry_t *pdei;
	unsigned long index;

	KASSERT(lvl > 1);

	index = PL_I(va, lvl);
    pdei = &pdes[lvl - 2][index];
    if (pdei && pmap_pde_v(pdei)) {
        return (pdei);
    }
	return (NULL);
}

static pd_entry_t *
pmap_apdei(va, lvl)
	vm_offset_t va;
	int lvl;
{
    pd_entry_t *apdei;
	unsigned long index;

	KASSERT(lvl > 1);

	index = PL_I(va, lvl);
    apdei = &apdes[lvl - 2][index];
    if (apdei && pmap_pde_v(apdei)) {
        return (apdei);
    }
	return (NULL);
}

pd_entry_t *
pmap_find_pde(vm_offset_t va)
{
    pd_entry_t *pde;
    unsigned long index;
    int i;

    for (i = PTP_LEVELS; i > 1; i--) {
        index = PL_I(va, i);
        pde = pde_search(va, i);
        /* check if valid */
        if (pde && pmap_pde_v(pde)) {
        	return (pde);
        }
    }
    return (NULL);
}

pmap_pdp(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pd_entry_t *pde, *apde;
    int i;

    for (i = PTP_LEVELS; i > 1; i--) {
    	 if (pmap && pmap_pde_v(pmap_pde(pmap, va, i))) {
    		 pde = pde_search(va, i);
    		 apde = apde_search(va, i);
    		 if (pde <= PDP_PDE && apde <= APDP_PDE) {
    			 if (pmap->pm_pdir == pde) {
    				 return (pde);
    			 } else {
    				 if (pmap->pm_pdir != apde) {
    					 tlbflush();
    				 }
    				 return (apde);
    			 }
    		 }
    	 }
    }
    return (NULL);
}

pd_entry_t *
pmap_find_apde(vm_offset_t va)
{
    pd_entry_t *apde;
    unsigned long index;
    int i;

    for (i = PTP_LEVELS; i > 1; i--) {
        index = PL_I(va, i);
        apde = apde_search(va, i);
        /* check if valid */
        if (apde && pmap_pde_v(apde)) {
        	return (apde);
        }
    }
    return (NULL);
}

static pt_entry_t *
pmap_pte(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pd_entry_t newpf;
	pd_entry_t *pde, *apde;

	pde = pmap_find_pde(va);
	if (pde != NULL) {
		if (pmap->pm_pdir == pde || pmap == kernel_pmap) {
			return (pde);
		}
	}

	apde = pmap_find_apde(va);
	if (apde != NULL) {
		if (pmap->pm_pdir != apde) {
			tlbflush();
		}
		newpf = *pde & PG_FRAME;
		if (newpf) {
			//pmap_invalidate_page(kernel_pmap, PADDR2);
		}
		return (apde);
	}
	return (0);
}

struct pmap {
	pt_entry_t 		*pm_ptab;
	pd_entry_t 		*pm_pdir;
	pdp_entry_t 	*pm_pdp;
	pml4_entry_t 	*pm_pml4;
};

pml4_entry_t *
pmap_pdp(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	pml4_entry_t *pml4;
	pml4 = pde_search(va, 4);
	return (&pmap->pm_pml4[pmap_pdp_index(va)]);
}

pdp_entry_t *
pmap_pdp_to_pd(pdp_pte, va)
	pml4_entry_t pdp_pte;
	vm_offset_t va;
{
	pdp_entry_t *pd;

	pd = (pdp_entry_t *)PHYS_TO_DMAP(pdp_pte & PG_FRAME);
	return (&pd[pmap_pd_index(va)]);
}

pdp_entry_t *
pmap_pd(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	pml4_entry_t *pdp;

	pdp = pmap_pdp(pmap, va);
	if (pdp && pmap_pde_v(pdp)) {
		return (pmap_pdp_to_pd(pdp, va));
	}
	return (NULL);
}

pml4_entry_t *
pmap_pml4e(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	pml4_entry_t *pml4e;

	pml4e = pmap_pde(pmap, va, 4);
	if (pml4e && pmap_pde_v(pml4e)) {
		return (&pml4e[pmap_pml4_index(va)]);
	}
	return (NULL);
}

static pt_entry_t *
pmap_ptetov(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
    pd_entry_t *pde;

    pde = pmap_find_pde(va);
    if (pde != NULL) {
    	if (pde == PDP_PDE) {
    		return (amd64_ptob(pde - PTE_BASE));
    	}
    }
	return (0);
}

static pt_entry_t *
pmap_ptetoav(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pd_entry_t *apde;

	apde = pmap_find_apde(va);
	if (apde != NULL) {
		if (apde == APDP_PDE) {
			return (amd64_ptob(apde - APTE_BASE));
		}
	}
	return (0);
}
