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

/*
 * TODO:
 * Issue:
 * - With Overlay's enabled pmap will run both hats, when only one is likely going to be accessed at any one time.
 *
 * Current Solution:
 * - Pass the hat flags through to the machine dependent pmap, instead of embedding them in vm_hat & ovl_hat.
 * This would have the benefit of carrying back over into the vm and ovl.
 */

struct hat_list	ovlhat_list;
struct hat_list vmhat_list;

/*
 * pmap functions which use pa_to_pvh:
 * pmap_remove
 * pmap_enter
 * pmap_remove_all: N/A (overlay does not use)
 * pmap_collect: N/A (overlay does not use)
 * pmap_pageable: N/A (overlay does not use)
 */

pv_entry_t
pa_to_pvh(pa)
	vm_offset_t pa;
{
	return (vm_hat_to_pvh(&vmhat_list, pa));
}

void
pmap_hat_bootstrap(firstaddr)
	vm_offset_t firstaddr;
{
	LIST_INIT(&vmhat_list);
#ifdef OVERLAY
	LIST_INIT(&ovlhat_list);
#endif
}

void
pmap_hat_init(phys_start, phys_end)
	vm_offset_t phys_start, phys_end;
{
	vm_hat_init(&vmhat_list, phys_start, phys_end);

#ifdef OVERLAY
	ovl_hat_init(&ovlhat_list, phys_start, phys_end);
#endif
}

void
pmap_hat_pv_enter(pmap, va, pa)
	pmap_t pmap;
	vm_offset_t va;
	vm_offset_t pa;
{
	vm_hat_pv_enter(&vmhat_list, pmap, va, pa);
#ifdef OVERLAY
	ovl_hat_pv_enter(&ovlhat_list, pmap, va, pa);
#endif
}

void
pmap_hat_pv_remove(pmap, sva, eva)
	pmap_t pmap;
	vm_offset_t sva, eva;
{
	vm_hat_pv_remove(&vmhat_list, pmap, sva, eva);
#ifdef OVERLAY
	ovl_hat_pv_remove(&ovlhat_list, pmap, sva, eva);
#endif
}
