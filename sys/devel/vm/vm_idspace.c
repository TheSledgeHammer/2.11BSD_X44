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

#include <vm_idspace.h>

 /* kspace */
vm_object_t kspace_object;

void
vm_kispace_map_init(kspace, size)
	vm_kspace_t kspace;
	vm_size_t size;
{
    kisd_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kispace_min, (vm_offset_t *)&kispace_max, size, FALSE);
    kisa_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kispace_min, (vm_offset_t *)&kispace_max, size, FALSE);
    kspace->desc_map = kisd_map;
    kspace->addr_map = kisa_map;
}

void
vm_kdspace_map_init(kspace, size)
	vm_kspace_t kspace;
	vm_size_t size;
{
    kdsd_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kdspace_min, (vm_offset_t *)&kdspace_max, size, FALSE);
    kdsa_map = kmem_suballoc(kernel_map, (vm_offset_t *)&kdspace_min, (vm_offset_t *)&kdspace_max, size, FALSE);
    kspace->desc_map = kdsd_map;
    kspace->addr_map = kdsa_map;
}

void
vm_kspace_init(size)
	vm_size_t size;
{
	vm_kspace_t kspace;

	vm_kispace_map_init(kspace, size);
	vm_kdspace_map_init(kspace, size);

	kspace->object = vm_kspace_object_create(size);
}

vm_object_t
vm_kspace_object_create(size)
	vm_size_t size;
{
	vm_object_t result;

	result = vm_object_allocate(size);
	if (result != NULL) {
		kspace_object = result;
		return (result);
	}
	return (NULL);
}

/* uspace */
vm_object_t uspace_object;

void
vm_uispace_map_init(uspace, size)
	vm_uspace_t uspace;
	vm_size_t size;
{
    uisd_map = kmem_suballoc(kernel_map, (vm_offset_t *)&uispace_min, (vm_offset_t *)&uispace_max, size, FALSE);
    uisa_map = kmem_suballoc(kernel_map, (vm_offset_t *)&uispace_min, (vm_offset_t *)&uispace_max, size, FALSE);
    uspace->desc_map = uisd_map;
    uspace->addr_map = uisa_map;
}

void
vm_udspace_map_init(uspace, size)
	vm_uspace_t uspace;
	vm_size_t size;
{
    udsd_map = kmem_suballoc(kernel_map, (vm_offset_t *)&udspace_min, (vm_offset_t *)&udspace_max, size, FALSE);
    udsa_map = kmem_suballoc(kernel_map, (vm_offset_t *)&udspace_min, (vm_offset_t *)&udspace_max, size, FALSE);
    uspace->desc_map = udsd_map;
    uspace->addr_map = udsa_map;
}

void
vm_uspace_init(size)
	vm_size_t size;
{
	vm_uspace_t uspace;

	vm_uispace_map_init(uspace, size);
	vm_udspace_map_init(uspace, size);

	uspace->object = vm_uspace_object_create(size);
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
