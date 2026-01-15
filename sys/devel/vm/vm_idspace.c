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

/* vm_segment_map */
#define M_VMSEGMAP 103

void
vm_segment_map_insert(segment_register, segment_number)
	vm_segment_register_t segment_register;
	int segment_number;
{
	vm_segment_map_t segm;

	segm = (vm_segment_map_t)malloc((u_long)sizeof(struct vm_segment_map), M_VMSEGMAP, M_WAITOK);
	segm->segment_register = segment_register;
	segm->segment_number = segment_number;
	//lock
	LIST_INSERT_HEAD(&segmaplist, segm, segmlist);
	//unlock
}

vm_segment_map_t
vm_segment_map_lookup(segment_register, segment_number)
	vm_segment_register_t segment_register;
	int segment_number;
{
	vm_segment_map_t segm;

	//lock
	LIST_FOREACH(segm, &segmaplist, segmlist) {
		if ((segm->segment_register == segment_register) && (segm->segment_number == segment_number)) {
			//unlock
			return (segm);
		}
	}
	//unlock
	return (NULL);
}

void
vm_segment_map_remove(segment_register, segment_number)
	vm_segment_register_t segment_register;
	int segment_number;
{
	vm_segment_map_t segm;

	segm = vm_segment_map_lookup(segment_register, segment_number);
	if (segm != NULL) {
		LIST_REMOVE(segm, segmlist);
	}
}

/* vm_segment_register */
void
vm_segment_register_save(segment_register, segment_number, addr, desc)
	vm_segment_register_t segment_register;
	int segment_number;
	vm_offset_t addr, desc;
{
	segment_register[segment_number].desc = desc;
	segment_register[segment_number].addr = addr;
}

void
vm_segment_register_restore(segment_register, segment_number)
	vm_segment_register_t segment_register;
	int segment_number;
{
	vm_segment_register_t segr;

	segr = segment_register;
	if (segr != NULL) {
		segr[segment_number] = segment_register[segment_number];
		segr[segment_number].desc = segment_register[segment_number].desc;
		segr[segment_number].addr = segment_register[segment_number].addr;
	}
}
