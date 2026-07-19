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
vm_kispace_map_init(kspace, mtype, object, min, max, size, pageable)
	vm_kspace_t kspace;
	int mtype;
	vm_object_t object;
	vm_offset_t *min, *max;
	vm_size_t size;
	bool_t pageable;
{
	vm_offset_t *imin, *dmin;
	vm_offset_t *imax, *dmax;
	int error;

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
	error = vm_idspace_init(kspace->idspace_i, &kspace->kisa_space, mtype,
			kisa_map, imin, imax, object, size, pageable);
	if (error != 0) {
		return;
	}

	/* I-Space descriptor map */
	error = vm_idspace_init(kspace->idspace_d, &kspace->kisd_space, mtype,
			kisd_map, dmin, dmax, object, size, pageable);
	if (error != 0) {
		return;
	}
}

void
vm_kdspace_map_init(kspace, mtype, object, min, max, size, pageable)
	vm_kspace_t kspace;
	int mtype;
	vm_object_t object;
	vm_offset_t *min, *max;
	vm_size_t size;
	bool_t pageable;
{
	vm_offset_t *imin, *dmin;
	vm_offset_t *imax, *dmax;
	int error;

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
	error = vm_idspace_init(kspace->idspace_i, &kspace->kdsa_space, mtype,
			kdsa_map, imin, imax, object, size, pageable);
	if (error != 0) {
		return;
	}

	/* D-Space descriptor map */
	error = vm_idspace_init(kspace->idspace_d, &kspace->kdsd_space, mtype,
			kdsa_map, dmin, dmax, object, size, pageable);
	if (error != 0) {
		return;
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

	/* Init I-Space */
	vm_kispace_map_init(kspace, M_VMKSPACE, kspace_object, min, &min, &max,
			size, TRUE);

	/* Init D-Space */
	vm_kdspace_map_init(kspace, M_VMKSPACE, kspace_object, min, &min, &max,
			size, TRUE);
}

/* kspace maps */
vm_kspace_map()
{

}

/* kspace regions */
void
vm_kspace_region_insert(kspace, segnum, sepid)
	vm_kspace_t kspace;
	int segnum, sepid;
{
	vm_idspace_t idspace;

	vm_segment_region_t region;

	if (sepid == 0) {
		idspace = kspace->idspace_i;
	} else {
		idspace = kspace->idspace_d;
	}
	if (idspace != NULL) {
		region = vm_segment_region_alloc(M_VMKSPACE);
		if (region != NULL) {
			idspace->region = region;
		}
	}


	if (idspace != NULL) {
		vm_segment_region_insert(idspace, region, segnum);
	}
}

set_idspace(kspace, sepid)
	vm_kspace_t kspace;
	int sepid;
{
	vm_idspace_t idspace;

	switch (sepid) {
	case 0:
		idspace = kspace->idspace_i;
		break;
	case 1:
		idspace = kspace->idspace_d;
		break;
	}

}

void
vm_kspace_region_remove(kspace, segnum)
	vm_kspace_t kspace;
	int segnum;
{
	vm_idspace_t idspace;

	idspace = kspace->idspace;
	if (idspace == NULL) {
		return;
	}
	vm_segment_region_remove(idspace, segnum);
}

vm_segment_region_t
vm_kspace_region_lookup(kspace, segnum)
	vm_kspace_t kspace;
	int segnum;
{
	vm_idspace_t idspace;
	vm_segment_region_t region;

	idspace = kspace->idspace;
	if (idspace == NULL) {
		return (NULL);
	}
	region = vm_segment_region_lookup(idspace, segnum);
	if (region == NULL) {
		return (NULL);
	}
	return (region);
}
