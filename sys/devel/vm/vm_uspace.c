/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

/* Code is based on 2.11BSD's PDP-11 code */

#include <sys/malloc.h>
#include <vm_idspace.h>

#include <arch/i386/include/pmap.h>
#include <arch/i386/include/vmparam.h>

#define VM_USPACE 	105

#define USPACE_MIN	USRSTACK
#define USPACE_MAX	VM_MAXUSER_ADDRESS

/* uspace */
vm_object_t uspace_object;
char *uispace_min, *uispace_max; /* user i-space vm_map range */
char *udspace_min, *udspace_max; /* user d-space vm_map range */

void
vm_uispace_map_init(uspace, min, max, size)
	vm_uspace_t uspace;
	vm_offset_t *min, *max;
	vm_size_t size;
{
	uisd_map = kmem_suballoc(kernel_map, min, max, size, FALSE);
	uisa_map = kmem_suballoc(kernel_map, min, max, size, FALSE);
	uspace->desc_map = uisd_map;
	uspace->addr_map = uisa_map;
}

void
vm_udspace_map_init(uspace, min, max, size)
	vm_uspace_t uspace;
	vm_offset_t *min, *max;
	vm_size_t size;
{
	udsd_map = kmem_suballoc(kernel_map, min, max, size, FALSE);
	udsa_map = kmem_suballoc(kernel_map, min, max, size, FALSE);
	uspace->desc_map = udsd_map;
	uspace->addr_map = udsa_map;
}

void
vm_uspace_init(min, max, size)
	vm_offset_t min, max;
	vm_size_t size;
{
	vm_uspace_t uspace;
	vm_object_t object;

	MALLOC(uspace, struct vm_uspace *, sizeof(struct vm_uspace *), VM_USPACE, M_WAITOK);

	if (min < USPACE_MIN && max > USPACE_MAX) {
		FREE(uspace, VM_USPACE);
		return;
	}

	uispace_min = (char *)min;
	uispace_max = (char *)max;
	vm_uispace_map_init(uspace, &min, &max, size);

	udspace_min = (char *)min;
	udspace_max = (char *)max;
	vm_udspace_map_init(uspace, &min, &max, size);

	object = vm_uspace_object_create(size);
	uspace->object = object;
}

vm_object_t
vm_uspace_object_create(size)
	vm_size_t size;
{
	vm_object_t result;

	result = vm_object_allocate(size);
	if (result != NULL) {
		uspace_object = result;
		return (result);
	}
	return (NULL);
}
