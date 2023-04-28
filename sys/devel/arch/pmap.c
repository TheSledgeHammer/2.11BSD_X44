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

extern vm_offset_t		vm_first_phys;		/* PA of first managed page */
extern vm_offset_t		vm_last_phys;		/* PA just past last managed page */

extern vm_offset_t		ovl_first_phys;		/* PA of first managed page */
extern vm_offset_t		ovl_last_phys;		/* PA just past last managed page */

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

/* compatability */
vm_offset_t
pa_index(pa)
	vm_offset_t pa;
{
	return (pmap_hat_pa_index(pa, PMAP_HAT_VM));
}

pv_entry_t
pa_to_pvh(pa)
	vm_offset_t pa;
{
	return (pmap_hat_pa_to_pvh(pa, PMAP_HAT_VM));
}
