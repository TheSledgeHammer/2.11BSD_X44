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
#include <vm_kspace.h>

#include <arch/i386/include/pmap.h>
#include <arch/i386/include/vmparam.h>

#define M_VMKSPACE 	104

#define KSPACE_MIN	VM_MIN_KERNEL_ADDRESS
#define KSPACE_MAX	VM_MAX_KERNEL_ADDRESS

 /* kspace */
vm_object_t kspace_object;
char *kispace_min, *kispace_max; /* kernel i-space vm_map range */
char *kdspace_min, *kdspace_max; /* kernel d-space vm_map range */

void
vm_kispace_map_init(kspace, object, offset, min, max, size, pageable)
	vm_kspace_t kspace;
	vm_object_t object;
	vm_offset_t offset;
	vm_offset_t *min, *max;
	vm_size_t size;
	bool_t pageable;
{
	vm_offset_t *imin, *dmin;
	vm_offset_t *imax, *dmax;

	kispace_min = (char *)min;
	kispace_max = (char *)max;

	/*
	 * These may need to be adjusted to account for the map/s needing more or less space,
	 * instead of an even split.
	 */
	imin = min; 			/* instruction map min */
	imax = ((max-min)/2); 	/* instruction map max */
	dmin = imax + 1; 		/* descriptor map min */
	dmax = max; 			/* descriptor map max */

	/* I-Space instruction map */
	kisa_map = vm_idspace_map_allocate(object, offset, imin, imax, size, pageable);
	if (kisa_map != NULL) {
		kspace->addr_map = kisa_map;
		kspace->i_start = &imin;
		kspace->i_end = &imax;
	}

	/* I-Space descriptor map */
	kisd_map = vm_idspace_map_allocate(object, offset, dmin, dmax, size, pageable);
	if (kisd_map != NULL) {
		kspace->desc_map = kisd_map;
		kspace->d_start = &dmin;
		kspace->d_end = &dmax;
	}
}

void
vm_kdspace_map_init(kspace, object, offset, min, max, size, pageable)
	vm_kspace_t kspace;
	vm_object_t object;
	vm_offset_t offset;
	vm_offset_t *min, *max;
	vm_size_t size;
	bool_t pageable;
{
	vm_offset_t *imin, *dmin;
	vm_offset_t *imax, *dmax;

	kdspace_min = (char *)min;
	kdspace_max = (char *)max;

	/*
	 * These may need to be adjusted to account for the map/s needing more or less space,
	 * instead of an even split.
	 */
	imin = min; 			/* instruction map min */
	imax = ((max-min)/2); 	/* instruction map max */
	dmin = imax + 1; 		/* descriptor map min */
	dmax = max; 			/* descriptor map max */

	/* D-Space instruction map */
	kdsa_map = vm_idspace_map_allocate(object, offset, imin, imax, size, pageable);
	if (kdsa_map != NULL) {
		kspace->addr_map = kisa_map;
		kspace->i_start = &imin;
		kspace->i_end = &imax;
	}

	/* D-Space descriptor map */
	kdsd_map = vm_idspace_map_allocate(object, offset, dmin, dmax, size, pageable);
	if (kdsd_map != NULL) {
		kspace->desc_map = kdsd_map;
		kspace->d_start = &dmin;
		kspace->d_end = &dmax;
	}
}

void
vm_kspace_init(min, max)
	vm_offset_t min, max;
{
	vm_kspace_t kspace;
	vm_size_t size;

	/* Allocate Kspace */
	MALLOC(kspace, struct vm_kspace *, sizeof(struct vm_kspace *), M_VMKSPACE, M_WAITOK);
	if (min < KSPACE_MIN && max > KSPACE_MAX) {
		FREE(kspace, M_VMKSPACE);
		return;
	}

	/* Set Object Size */
	size = (max - min);

	/* Allocate Object */
	vm_kspace_object_init(kspace, size, kspace_object);

	/* Init I-Space */
	vm_kispace_map_init(kspace, kspace_object, min, &min, &max, size, TRUE);

	/* Init D-Space */
	vm_kdspace_map_init(kspace, kspace_object, min, &min, &max, size, TRUE);

	/* Init idspace */
	vm_idspace_init(kspace->idspace, kspace->object, min, M_VMKSPACE);
}

void
vm_kspace_object_init(kspace, size, object)
	vm_kspace_t kspace;
	vm_size_t size;
	vm_object_t object;
{
	object = vm_object_allocate(size);
	if (object != NULL) {
		kspace->object = object;
	}
}
