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

#include <devel/vm/include/vm.h>

#include <devel/ovl/include/ovl.h>

vm_offset_t				overlay_avail;
vm_offset_t 			overlay_end;

void
pmap_overlay(firstaddr)
{
	overlay_avail = (vm_offset_t)firstaddr;
	overlay_end = 	OVL_MAX_ADDRESS;
	virtual_avail = (vm_offset_t)overlay_end;
	virtual_end = 	VM_MAX_KERNEL_ADDRESS;
}

void *
pmap_bootstrap_alloc(va, size)
	vm_offset_t va;
	u_long 		size;
{
	vm_offset_t val;
	int i;
	extern bool_t vm_page_startup_initialized;

	if (vm_page_startup_initialized) {
		panic("pmap_bootstrap_alloc: called after startup initialized");
	}
	size = round_page(size);
	val = va;

	/* deal with "hole incursion" */
	for (i = 0; i < size; i += PAGE_SIZE) {
		while (!pmap_isvalidphys(avail_start)) {
			avail_start += PAGE_SIZE;
		}
		va = pmap_map(va, avail_start, avail_start + PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE);
		avail_start += PAGE_SIZE;
	}

	bzero((caddr_t) val, size);
	return ((void *) val);
}

void *
pmap_bootstrap_overlay_alloc(size)
	u_long size;
{
	return (pmap_bootstrap_alloc(overlay_avail, size));
}
