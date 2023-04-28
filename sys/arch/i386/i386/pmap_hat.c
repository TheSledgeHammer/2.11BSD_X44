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

#ifdef OVERLAY
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
static vm_offset_t     	vm_hat_pa_index(vm_offset_t);
static pv_entry_t		vm_hat_to_pvh(pmap_hat_list_t, vm_offset_t);

#ifdef OVERLAY
static vm_offset_t     	ovl_hat_pa_index(vm_offset_t);
static pv_entry_t	   	ovl_hat_to_pvh(pmap_hat_list_t, vm_offset_t);
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

void
pmap_hat_init(hatlist, phys_start, phys_end)
	pmap_hat_list_t hatlist;
	vm_offset_t phys_start, phys_end;
{
	vm_hat_init(hatlist, phys_start, phys_end);
#ifdef OVERLAY
	ovl_hat_init(hatlist, phys_start, phys_end);
#endif
}

static vm_offset_t
pmap_hat_to_pa_index(pa, first_phys)
	vm_offset_t pa;
	vm_offset_t first_phys;
{
	vm_offset_t index;
	index = atop(pa - first_phys);
	return (index);
}

static pv_entry_t
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

#ifdef OVERLAY
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

#ifdef OVERLAY
	case PMAP_HAT_OVL:
		pvh = ovl_hat_to_pvh(&ovlhat_list, pa);
		break;
#endif
	}

	return (pvh);
}

static void
pmap_remove_pv(pmap, va, pv, bits)
	register pmap_t pmap;
	vm_offset_t va;
	pv_entry_t pv;
	int bits;
{
	register pv_entry_t npv;
	int s;

	/*
	 * Remove from the PV table (raise IPL since we
	 * may be called at interrupt time).
	 */
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
			pv->pv_next = NULL;
		}
	} else {
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va) {
				break;
			}
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
			free((caddr_t)npv, M_VMPVENT);
		}
	}

	pv->pv_attr |= bits;
	splx(s);
}

static void
pmap_enter_pv(pmap, va, pv)
	pmap_t pmap;
	vm_offset_t va;
	pv_entry_t pv;
{
	register pv_entry_t npv;
	int s;

	s = splimp();
	if (pv->pv_pmap == NULL) {
		pv->pv_va = va;
		pv->pv_pmap = pmap;
		pv->pv_next = NULL;
		pv->pv_flags = 0;
	} else {
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
		npv = (pv_entry_t)malloc(sizeof(*npv), M_VMPVENT, M_NOWAIT);
	    npv->pv_pmap = pmap;
	    npv->pv_va = va;
	    npv->pv_next = pv->pv_next;
	    pv->pv_next = npv;
	}
	splx(s);
}

void
pmap_hat_remove_pv(pmap, va, pa, bits, flags, first_phys, last_phys)
	pmap_t pmap;
	vm_offset_t va, first_phys, last_phys;
	vm_offset_t pa;
	int bits, flags;
{
	register pv_entry_t pv;

	if (pa < first_phys || pa >= last_phys) {
		pv = pmap_hat_pa_to_pvh(pa, flags);
		pmap_remove_pv(pmap, va, pv, bits);
	}
}

void
pmap_hat_enter_pv(pmap, va, pa, wired, flags, first_phys, last_phys)
	pmap_t pmap;
	vm_offset_t va, first_phys, last_phys;
	vm_offset_t pa;
	bool_t wired;
	int flags;
{
	register pv_entry_t pv;
	bool_t cacheable = TRUE;
	bool_t checkpv = TRUE;

	if (pa >= first_phys && pa < last_phys) {
		pv = pmap_hat_pa_to_pvh(pa, flags);
		pmap_enter_pv(pmap, va, pv);
	}
}

/* VM HAT */
void
vm_hat_init(hatlist, phys_start, phys_end, addr)
	pmap_hat_list_t hatlist;
	vm_offset_t phys_start, phys_end, addr;
{
	pmap_hat_t hat;
	vm_size_t pvsize, size, npg;

    vm_object_reference(kernel_object);
    vm_map_find(kernel_map, kernel_object, addr, &addr, 2*NBPG, FALSE);

	pvsize = (vm_size_t)(sizeof(struct pv_entry));
	npg = atop(phys_end - phys_start);
	size = (pvsize * npg + npg);

	LIST_INIT(hatlist);
	hat = (struct pmap_hat *)kmem_alloc(kernel_map, sizeof(struct pmap_hat));
	hat->ph_pvtable = (struct pv_entry *)kmem_alloc(kernel_map, size);
	hat->ph_pvtable->pv_attr = (char)addr;
	pmap_hat_attach(hatlist, hat, kernel_map, kernel_object, PMAP_HAT_VM);
}

static vm_offset_t
vm_hat_pa_index(pa)
	vm_offset_t pa;
{
	return (pmap_hat_to_pa_index(pa, vm_first_phys));
}

static pv_entry_t
vm_hat_to_pvh(hatlist, pa)
	pmap_hat_list_t hatlist;
	vm_offset_t pa;
{
	return (pmap_hat_to_pvh(hatlist, kernel_map, kernel_object, pa, vm_first_phys, PMAP_HAT_VM));
}

#ifdef OVERLAY
/* OVERLAY HAT */
void
ovl_hat_init(hatlist, phys_start, phys_end, addr)
	pmap_hat_list_t hatlist;
	vm_offset_t phys_start, phys_end, addr;
{
	pmap_hat_t hat;
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

static vm_offset_t
ovl_hat_pa_index(pa)
	vm_offset_t pa;
{
	return (pmap_hat_to_pa_index(pa, ovl_first_phys));
}

static pv_entry_t
ovl_hat_to_pvh(hatlist, pa)
	pmap_hat_list_t hatlist;
	vm_offset_t pa;
{
	return (pmap_hat_to_pvh(hatlist, overlay_map, overlay_object, pa, ovl_first_phys, PMAP_HAT_OVL));
}
#endif
