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

#include <devel/arch/pmap_hat.h>

struct hat_list	ovlhat_list;
struct hat_list vmhat_list;

void
pmap_init(phys_start, phys_end)
	vm_offset_t phys_start, phys_end;
{
	vm_hat_init(&vmhat_list, phys_start, phys_end);

#ifdef OVERLAY
	ovl_hat_init(&ovlhat_list, phys_start, phys_end);
#endif
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

	case PMAP_HAT_OVL:
		index = ovl_hat_pa_index(pa);
		break;
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

	case PMAP_HAT_OVL:
		pvh = ovl_hat_to_pvh(&ovlhat_list, pa);
		break;
	}
	return (pvh);
}

vm_offset_t
pmap_hat_map(virt, start, end, prot, flags)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int			prot, flags;
{
	vm_offset_t va, sva;
	va = sva = virt;

	while (start < end) {
		pmap_hat_enter(kernel_pmap, va, start, prot, FALSE, flags);
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
		pmap_attributes[pmap_hat_pa_to_pvh(pa, flags)] |= bits;
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

/* compatability */
vm_offset_t
pa_index(pa)
	vm_offset_t pa;
{
	return (vm_hat_pa_index(pa));
}

pv_entry_t
pa_to_pvh(pa)
	vm_offset_t pa;
{
	return (vm_hat_to_pvh(&vmhat_list, pa));
}

vm_offset_t
pmap_map(virt, start, end, prot)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int			prot;
{
	return (pmap_hat_map(virt, start, end, prot, PMAP_HAT_VM));
}

void
pmap_remove(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	pmap_hat_remove(pmap, sva, eva, PMAP_HAT_VM, vm_first_phys, vm_last_phys);
}

void
pmap_enter(pmap, va, pa, prot, wired)
	pmap_t pmap;
	vm_offset_t va;
	vm_offset_t pa;
	vm_prot_t prot;
	bool_t wired;
{
	pmap_hat_enter(pmap, va, pa, prot, wired, PMAP_HAT_VM, vm_first_phys, vm_last_phys);
}

void
pmap_overlay_bootstrap(firstaddr, res)
	vm_offset_t firstaddr;
	u_long res;
{
	overlay_avail = (vm_offset_t)firstaddr;
	overlay_end = OVL_MAX_ADDRESS;

	/*
	 * Set first available physical page to the end
	 * of the overlay address space.
	 */
	res = atop(overlay_end - (vm_offset_t)KERNLOAD);

	avail_start = overlay_end;

	virtual_avail = (vm_offset_t)overlay_end;
	virtual_end = VM_MAX_KERNEL_ADDRESS;
}

vm_offset_t
pmap_overlay_map(virt, start, end, prot)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int			prot;
{
	return (pmap_hat_map(virt, start, end, prot, PMAP_HAT_OVL));
}

void
pmap_overlay_remove(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	pmap_hat_remove(pmap, sva, eva, PMAP_HAT_OVL, ovl_first_phys, ovl_last_phys);
}

void
pmap_overlay_enter(pmap, va, pa, prot, wired)
	pmap_t pmap;
	vm_offset_t va;
	vm_offset_t pa;
	vm_prot_t prot;
	bool_t wired;
{
	pmap_hat_enter(pmap, va, pa, prot, wired, PMAP_HAT_OVL, ovl_first_phys, ovl_last_phys);
}
