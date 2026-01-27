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

#define VM_KSPACE 	104

#define KSPACE_MIN	VM_MIN_KERNEL_ADDRESS
#define KSPACE_MAX	VM_MAX_KERNEL_ADDRESS

 /* kspace */
vm_object_t kspace_object;
char *kispace_min, *kispace_max; /* kernel i-space vm_map range */
char *kdspace_min, *kdspace_max; /* kernel d-space vm_map range */

void
vm_kispace_map_init(kspace, min, max, size)
	vm_kspace_t kspace;
	vm_offset_t *min, *max;
	vm_size_t size;
{
	kisd_map = kmem_suballoc(kernel_map, min, max, size, FALSE);
	kisa_map = kmem_suballoc(kernel_map, min, max, size, FALSE);
	kspace->desc_map = kisd_map;
	kspace->addr_map = kisa_map;
}

void
vm_kdspace_map_init(kspace, min, max, size)
	vm_kspace_t kspace;
	vm_offset_t *min, *max;
	vm_size_t size;
{
	kdsd_map = kmem_suballoc(kernel_map, min, max, size, FALSE);
	kdsa_map = kmem_suballoc(kernel_map, min, max, size, FALSE);
	kspace->desc_map = kdsd_map;
	kspace->addr_map = kdsa_map;
}

void
vm_kspace_init(min, max, size)
	vm_offset_t min, max;
	vm_size_t size;
{
	vm_kspace_t kspace;
	vm_object_t object;

	MALLOC(kspace, struct vm_kspace *, sizeof(struct vm_kspace *), VM_KSPACE, M_WAITOK);

	if (min < KSPACE_MIN && max > KSPACE_MAX) {
		FREE(kspace, VM_KSPACE);
		return;
	}

	kispace_min = (char *)min;
	kispace_max = (char *)max;
	vm_kispace_map_init(kspace, &min, &max, size);

	kdspace_min = (char *)min;
	kdspace_max = (char *)max;
	vm_kdspace_map_init(kspace, &min, &max, size);

	object = vm_kspace_object_create(size);
	kspace->object = object;
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
