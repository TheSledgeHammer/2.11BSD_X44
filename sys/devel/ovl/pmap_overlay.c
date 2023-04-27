/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/map.h>

#include <vm/include/vm.h>

#include <devel/ovl/include/ovl.h>
#include <devel/ovl/include/pmap.h>
#include <arch/i386/include/pmap.h>

struct pv_overlay {
    struct pv_overlay       *ov_next;
    struct pmap             *ov_pmap;
    vm_offset_t             ov_va;
    int 					ov_flags;
};
typedef struct pv_overlay   *pv_overlay_t;

pv_overlay_t			ovl_table;

vm_offset_t				overlay_avail;
vm_offset_t 			overlay_end;
vm_offset_t				ovl_first_phys;
vm_offset_t				ovl_last_phys;

static vm_offset_t
pa_ov_index(pa)
	vm_offset_t pa;
{
	vm_offset_t index;
	index = atop(pa - ovl_first_phys);
	return (index);
}

pv_overlay_t
pa_to_ovh(pa)
    vm_offset_t pa;
{
    pv_overlay_t ovh;
    ovh = &ovl_table[pa_ov_index[pa]];
    return (ovh);
}

/* Example of pmap_bootstrap with overlays */
void
pmap_bootstrap(firstaddr)
	vm_offset_t firstaddr;
{
	vm_offset_t va;
	pt_entry_t *pte;
	u_long res;
	int i;

#ifdef OVERLAYS
	pmap_overlay_bootstrap(firstaddr, res);
#else
	res = atop(firstaddr - (vm_offset_t)KERNLOAD);

	avail_start = firstaddr;

	virtual_avail = (vm_offset_t)firstaddr;
	virtual_end = VM_MAX_KERNEL_ADDRESS;
#endif
}

void
pmap_overlay_bootstrap(firstaddr, res)
	vm_offset_t firstaddr;
	u_long res;
{
	overlay_avail = (vm_offset_t)firstaddr;
	overlay_end = 	OVL_MAX_ADDRESS;

	/*
	 * Set first available physical page to the end
	 * of the overlay address space.
	 */
	res = atop(overlay_end - (vm_offset_t)KERNLOAD);

	avail_start = overlay_end;

	virtual_avail = (vm_offset_t)overlay_end;
	virtual_end = 	VM_MAX_KERNEL_ADDRESS;
}

void *
pmap_overlay_bootstrap_allocate(va, size)
	vm_offset_t va;
	u_long 		size;
{
    vm_offset_t val;
	int i;

	size = round_page(size);
	val = va;
    for (i = 0; i < size; i += PAGE_SIZE) {
        while (!pmap_isvalidphys(avail_start)) {
			avail_start += PAGE_SIZE;
		}
        va = pmap_overlay_map(va, avail_start, avail_start + PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE);
        avail_start += PAGE_SIZE;
    }
    bzero((caddr_t) val, size);
	return ((void *) val);
}

void *
pmap_overlay_bootstrap_alloc(size)
	u_long size;
{
	return (pmap_overlay_bootstrap_allocate(overlay_avail, size));
}

vm_offset_t
pmap_overlay_map(virt, start, end, prot)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int			prot;
{
    vm_offset_t va, sva;

	va = sva = virt;
    while (start < end) {
        pmap_overlay_enter(kernel_pmap, va, start, prot, FALSE);
        va += PAGE_SIZE;
		start += PAGE_SIZE;
    }
    pmap_invalidate_range(kernel_pmap, sva, va);
    virt = va;
    return (sva);
}

void
pmap_overlay_remove(pmap, sva, eva)
	register pmap_t pmap;
	vm_offset_t sva, eva;
{
	register vm_offset_t pa, va;
	register pv_overlay_t ov, nov;
    int s;

    if (pmap == NULL) {
        return;
    }
    for (va = sva; va < eva; va += PAGE_SIZE) {
        if (pa < ovl_first_phys || pa >= ovl_last_phys) {
            continue;
        }
        ov = pa_to_ovh(pa);
        s = splimp();
        if (pmap == ov->ov_pmap && va == ov->ov_va) {
            nov = ov->ov_next;
            if (nov) {
                *ov = *nov;
			    free((caddr_t)nov, M_OVLPVENT);
            } else {
                ov->ov_pmap = NULL;
            }
        } else {
            for (nov = ov->ov_next; nov; nov = nov->ov_next) {
                if (pmap == nov->ov_pmap && va == nov->ov_va) {
                    break;
                }
                ov = nov;
            }
            ov->ov_next = nov->ov_next;
		    free((caddr_t)nov, M_OVLPVENT);
		    ov = pa_to_ovh(pa);
        }
        splx(s);
    }
}

void
pmap_overlay_enter(pmap, va, pa, prot, wired)
	register pmap_t pmap;
	vm_offset_t va;
	register vm_offset_t pa;
	vm_prot_t prot;
	bool_t wired;
{
    if (pa >= ovl_first_phys || pa < ovl_last_phys) {
        register pv_overlay_t ov, nov;
        int s;

        ov = pa_to_ovh(pa);
        s = splimp();
        if (ov->ov_pmap == NULL) {
            ov->ov_va = va;
			ov->ov_pmap = pmap;
			ov->ov_next = NULL;
			ov->ov_flags = 0;
        } else {
            nov = (pv_overlay_t) malloc(sizeof *nov, M_OVLPVENT, M_NOWAIT);
			nov->ov_va = va;
			nov->ov_pmap = pmap;
			nov->ov_next = ov->ov_next;
			ov->ov_next = nov;
            splx(s);
        }
    }
}

void
pmap_overlay_init(phys_start, phys_end)
	vm_offset_t	phys_start, phys_end;
{
    vm_offset_t addr;
    vm_size_t npg, s;

    addr = (vm_offset_t) KERNBASE + KPTphys;
    ovl_object_reference(overlay_object);
    ovl_map_find(overlay_map, overlay_object, addr, &addr, 2 * NBPG, FALSE);
    overlay_object->vm_object = kernel_object;

    npg = atop(phys_end - phys_start);
    s = (vm_size_t) (sizeof(struct pv_overlay) * npg + npg);
	s = round_page(s);

    ovl_table = (pv_overlay_t)omem_alloc(overlay_map, s);
    addr = (vm_offset_t) ovl_table;
	addr += sizeof(struct pv_overlay) * npg;
	pmap_attributes = (char *) addr;

    ovl_first_phys = phys_start;
    ovl_last_phys = phys_end;
}
