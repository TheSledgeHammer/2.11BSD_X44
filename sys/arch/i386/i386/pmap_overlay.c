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

#ifdef OVERLAY
#include <ovl/include/ovl.h>
#include <ovl/include/ovl_overlay.h>
#include <ovl/include/ovl_page.h>
#endif

#include <machine/pmap.h>
#include <machine/pmap_hat.h>

#ifdef OVERLAY

void
pmap_overlay_bootstrap(firstaddr, res)
	vm_offset_t firstaddr;
	u_long res;
{
      extern vm_offset_t avail_start, virtual_avail, virtual_end;
      
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

void *
pmap_overlay_bootstrap_allocate(va, size)
	vm_offset_t va;
	u_long 		size;
{
    vm_offset_t val;
	int i;
	extern vm_offset_t avail_start;

	size = round_page(size);
	val = va;
    for (i = 0; i < size; i += PAGE_SIZE) {
        va = pmap_overlay_map(va, avail_start, avail_start + PAGE_SIZE);
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
pmap_overlay_map(virt, start, end)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
{
	vm_offset_t va, sva;
	va = sva = virt;

	while (start < end) {
		pmap_overlay_enter(kernel_pmap, va, start);
		va += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	virt = va;
	return (sva);
}

void
pmap_overlay_pinit(pmap)
	register pmap_t pmap;
{

}

void
pmap_overlay_destroy(pmap)
	register pmap_t pmap;
{

}

void
pmap_overlay_release(pmap)
	pmap_t pmap;
{
	//omem_free(overlay_map, (vm_offset_t)pmap->pm_pdir, size);
}

void
pmap_overlay_reference(pmap)
	pmap_t	pmap;
{

}

void
pmap_overlay_remove(pmap, sva, eva)
	register pmap_t pmap;
	vm_offset_t sva, eva;
{
	register vm_offset_t pa, va;
	int bits;

	if (pmap == NULL) {
		return;
	}

	for (va = sva; va < eva; va += PAGE_SIZE) {
		bits = 0;
		pmap_hat_remove_pv(pmap, va, pa, bits, PMAP_HAT_OVL, ovl_first_phys, ovl_last_phys);
	}
}

void
pmap_overlay_enter(pmap, va, pa)
	register pmap_t pmap;
	vm_offset_t va;
	register vm_offset_t pa;
{
	if (pmap == NULL) {
		return;
	}
	if (va > OVL_MAX_ADDRESS) {
		panic("pmap_overlay_enter: toobig");
	}
	pmap_hat_enter_pv(pmap, va, pa, PMAP_HAT_OVL, ovl_first_phys, ovl_last_phys);
}
#endif
