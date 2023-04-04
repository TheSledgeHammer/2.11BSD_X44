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

vm_offset_t				overlay_avail;
vm_offset_t 			overlay_end;

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
pmap_overlay_bootstrap_alloc(size)
	u_long size;
{
	return (pmap_bootstrap_allocate(overlay_avail, size));
}

/* Example of pmap_bootstrap with overlays */
void
pmap_bootstrap(firstaddr)
	vm_offset_t firstaddr;
{
	vm_offset_t va;
	pt_entry_t *pte, *unused;
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
