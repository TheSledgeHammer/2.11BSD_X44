/*
 * The 3-Clause BSD License:
 * Copyright (c) 2023 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

#ifdef OVERLAYS
#include <ovl/include/ovl.h>
#include <ovl/include/ovl_overlay.h>
#include <ovl/include/ovl_page.h>
#endif

#include <machine/cpufunc.h>
#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/pmap_hat.h>

/*
 * TODO:
 * Issue:
 * - With Overlay's enabled pmap will run both hats, when only one is likely going to be accessed at any one time.
 *
 * Current Solution:
 * - Pass the hat flags through to the machine dependent pmap, instead of embedding them in vm_hat & ovl_hat.
 * This would have the benefit of carrying back over into the vm and ovl.
 */
/*
 * pmap functions which use pa_to_pvh:
 * pmap_remove
 * pmap_enter
 * pmap_remove_all: N/A (overlay does not use)
 * pmap_collect: N/A (overlay does not use)
 * pmap_pageable: N/A (overlay does not use)
 */

#define pte_prot(m, p)	(protection_codes[p])
extern int	protection_codes[8];

extern int i386pagesperpage;
//extern char *pmap_attributes;

extern vm_offset_t		vm_first_phys;		/* PA of first managed page */
extern vm_offset_t		vm_last_phys;		/* PA just past last managed page */

extern vm_offset_t		ovl_first_phys;		/* PA of first managed page */
extern vm_offset_t		ovl_last_phys;		/* PA just past last managed page */

struct pmap_hat_list 	vmhat_list;

#ifdef OVERLAYS
struct pmap_hat_list	ovlhat_list;
#endif

/* PMAP HAT */
void
pmap_hat_attach(hatlist, hat, map, object, flags)
	pmap_hat_list_t hatlist;
	pmap_hat_t hat;
	pmap_hat_map_t map;
	pmap_hat_object_t object;
	int flags;
{
    hat->ph_map = map;
    hat->ph_object = object;
    hat->ph_flags = flags;
    LIST_INSERT_HEAD(hatlist, hat, ph_next);
}

void
pmap_hat_detach(hatlist, map, object, flags)
	pmap_hat_list_t hatlist;
	pmap_hat_map_t map;
	pmap_hat_object_t object;
	int flags;
{
	pmap_hat_t hat;
	LIST_FOREACH(hat, hatlist, ph_next) {
        if (map == hat->ph_map && object == hat->ph_object && flags == hat->ph_flags) {
        	LIST_REMOVE(hat, ph_next);
        }
    }
}

pmap_hat_t
pmap_hat_find(hatlist, map, object, flags)
	pmap_hat_list_t hatlist;
	pmap_hat_map_t map;
	pmap_hat_object_t object;
	int flags;
{
	pmap_hat_t hat;
	LIST_FOREACH(hat, hatlist, ph_next) {
        if (map == hat->ph_map && object == hat->ph_object && flags == hat->ph_flags) {
            return (hat);
        }
    }
    return (NULL);
}

vm_offset_t
pmap_hat_to_pa_index(pa, first_phys)
	vm_offset_t pa;
	vm_offset_t first_phys;
{
	vm_offset_t index;
	index = atop(pa - first_phys);
	return (index);
}

pv_entry_t
pmap_hat_to_pvh(hatlist, map, object, pa, first_phys, flags)
	pmap_hat_list_t hatlist;
	pmap_hat_map_t map;
	pmap_hat_object_t object;
	vm_offset_t pa;
	vm_offset_t first_phys;
	int flags;
{
	pmap_hat_t hat;

    hat = pmap_hat_find(hatlist, map, object, flags);
    return (&hat->ph_pvtable[pmap_hat_to_pa_index(pa, first_phys)]);
}

vm_offset_t
pmap_hat_pa_index(pa, flags)
	vm_offset_t pa;
	int flags;
{
	vm_offset_t index;

	switch (flags) {
	case PMAP_HAT_VM:
		index = vm_hat_pa_index(pa);
		break;

#ifdef OVERLAYS
	case PMAP_HAT_OVL:
		index = ovl_hat_pa_index(pa);
		break;
#endif
	}
	return (index);
}

pv_entry_t
pmap_hat_pa_to_pvh(pa, flags)
	vm_offset_t pa;
	int flags;
{
	pv_entry_t pvh;

	switch (flags) {
	case PMAP_HAT_VM:
		pvh = vm_hat_to_pvh(&vmhat_list, pa);
		break;

#ifdef OVERLAYS
	case PMAP_HAT_OVL:
		pvh = ovl_hat_to_pvh(&ovlhat_list, pa);
		break;
#endif
	}

	return (pvh);
}

vm_offset_t
pmap_hat_map(virt, start, end, prot, flags, first_phys, last_phys)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end, first_phys, last_phys;
	int		prot, flags;
{
	vm_offset_t va, sva;
	va = sva = virt;

	while (start < end) {
		pmap_hat_enter(kernel_pmap, va, start, prot, FALSE, flags, first_phys, last_phys);
		va += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	pmap_invalidate_range(kernel_pmap, sva, va);
	virt = va;
	return (sva);
}

void
pmap_hat_remove(pmap, sva, eva, flags, first_phys, last_phys)
	register pmap_t pmap;
	vm_offset_t sva, eva, first_phys, last_phys;
	int flags;
{
	register vm_offset_t pa, va;
	register pt_entry_t *pte;
	register pv_entry_t pv, npv;
	register int ix;
	int *pde, s, bits;

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
			bits |= *(int *)pte & (PG_U|PG_M);
			*(int *)pte++ = 0;
		} while (++ix != i386pagesperpage);
		if (pmap == &curproc->p_vmspace->vm_pmap) {
			pmap_activate(pmap, (struct pcb *)curproc->p_addr);
		}
		tlbflush();

		/*
		 * Remove from the PV table (raise IPL since we
		 * may be called at interrupt time).
		 */
		if (pa < first_phys || pa >= last_phys) {
			continue;
		}
		pv = pmap_hat_pa_to_pvh(pa, flags);
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
			pv->pv_next = npv->pv_next;
			free((caddr_t)npv, M_VMPVENT);
			pv = pmap_hat_pa_to_pvh(pa, flags);
		}

		/*
		 * Update saved attributes for managed page
		 */
		//pmap_attributes[pmap_hat_pa_to_pvh(pa, flags)] |= bits;
		splx(s);
	}
}

void
pmap_hat_enter(pmap, va, pa, prot, wired, flags, first_phys, last_phys)
	register pmap_t pmap;
	vm_offset_t va, first_phys, last_phys;
	register vm_offset_t pa;
	vm_prot_t prot;
	bool_t wired;
	int flags;
{
	register pt_entry_t *pte;
	register int npte, ix;
	vm_offset_t opa;
	bool_t cacheable = TRUE;
	bool_t checkpv = TRUE;

	if (pmap == NULL) {
		return;
	}

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
		pmap_hat_remove(pmap, va, va + PAGE_SIZE, flags, first_phys, last_phys);
	}

	/*
	 * Enter on the PV list if part of our managed memory
	 * Note that we raise IPL while manipulating pv_table
	 * since pmap_enter can be called at interrupt time.
	 */
	if (pa >= first_phys && pa < last_phys) {
		register pv_entry_t pv, npv;
		int s;

		pv = pmap_hat_pa_to_pvh(pa, flags);
		s = splimp();
		/*
		 * No entries yet, use header as the first entry
		 */
		if (pv->pv_pmap == NULL) {
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
			npv = (pv_entry_t) malloc(sizeof *npv, M_VMPVENT, M_NOWAIT);
			npv->pv_va = va;
			npv->pv_pmap = pmap;
			npv->pv_next = pv->pv_next;
			pv->pv_next = npv;
			splx(s);
		}
	}
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
	} while (++ix != i386pagesperpage);
	pte--;
	pmap_invalidate_page(pmap, va);
	tlbflush();
}

/* VM HAT */
void
vm_hat_init(hatlist, phys_start, phys_end)
	pmap_hat_list_t hatlist;
	vm_offset_t phys_start, phys_end;
{
	pmap_hat_t hat;
	vm_offset_t addr;
	vm_size_t pvsize, size, npg;

    vm_object_reference(kernel_object);
    vm_map_find(kernel_map, kernel_object, addr, &addr, 2*NBPG, FALSE);

	pvsize = (vm_size_t)(sizeof(struct pv_entry));
	npg = atop(phys_end - phys_start);
	size = (pvsize * npg + npg);

	LIST_INIT(hatlist);
	hat = (struct pmap_hat *)kmem_alloc(kernel_map, sizeof(struct pmap_hat));
	hat->ph_pvtable = (struct pv_entry *)kmem_alloc(kernel_map, size);
	hat->ph_pvtable->pv_attr = (char *)addr;
	pmap_hat_attach(hatlist, hat, kernel_map, kernel_object, PMAP_HAT_VM);
}

vm_offset_t
vm_hat_pa_index(pa)
	vm_offset_t pa;
{
	return (pmap_hat_to_pa_index(pa, vm_first_phys));
}

pv_entry_t
vm_hat_to_pvh(hatlist, pa)
	pmap_hat_list_t hatlist;
	vm_offset_t pa;
{
	return (pmap_hat_to_pvh(hatlist, kernel_map, kernel_object, pa, vm_first_phys, PMAP_HAT_VM));
}

#ifdef OVERLAYS
/* OVERLAY HAT */
void
ovl_hat_init(hatlist, phys_start, phys_end)
	pmap_hat_list_t hatlist;
	vm_offset_t phys_start, phys_end;
{
	pmap_hat_t hat;
	vm_offset_t addr;
	vm_size_t pvsize, size, npg;

	ovl_object_reference(overlay_object);
	ovl_map_find(overlay_map, overlay_object, addr, &addr, 2*NBPG, FALSE);

	pvsize = (vm_size_t)(sizeof(struct pv_entry));
	npg = atop(phys_end - phys_start);
	size = (pvsize * npg + npg);

	LIST_INIT(hatlist);
	hat = (struct pmap_hat *)omem_alloc(overlay_map, sizeof(struct pmap_hat));
	hat->ph_pvtable = (struct pv_entry *)omem_alloc(overlay_map, size);
	pmap_hat_attach(hatlist, hat, overlay_map, overlay_object, PMAP_HAT_OVL);
}

vm_offset_t
ovl_hat_pa_index(pa)
	vm_offset_t pa;
{
	return (pmap_hat_to_pa_index(pa, ovl_first_phys));
}

pv_entry_t
ovl_hat_to_pvh(hatlist, pa)
	pmap_hat_list_t hatlist;
	vm_offset_t pa;
{
	return (pmap_hat_to_pvh(hatlist, overlay_map, overlay_object, pa, ovl_first_phys, PMAP_HAT_OVL));
}
#endif
