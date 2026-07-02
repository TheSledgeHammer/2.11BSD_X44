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
#include <vm_uspace.h>

#include <arch/i386/include/pmap.h>
#include <arch/i386/include/vmparam.h>

#define M_VMUSPACE 	105

#define USPACE_MIN	USRSTACK
#define USPACE_MAX	VM_MAXUSER_ADDRESS

/* uspace */
vm_object_t uspace_object;
char *uispace_min, *uispace_max; /* user i-space vm_map range */
char *udspace_min, *udspace_max; /* user d-space vm_map range */

void
vm_uispace_map_init(uspace, mtype, object, min, max, size, pageable)
	vm_uspace_t uspace;
	int mtype;
	vm_object_t object;
	//vm_offset_t offset;
	vm_offset_t *min, *max;
	vm_size_t size;
	bool_t pageable;
{
	vm_offset_t *imin, *dmin;
	vm_offset_t *imax, *dmax;
	int error;

	uispace_min = (char *)min;
	uispace_max = (char *)max;

	/*
	 * These may need to be adjusted to account for the map/s needing more or less space,
	 * instead of an even split.
	 */
	imin = min; 			/* instruction map min */
	imax = ((max-min)/2); 	/* instruction map max */
	dmin = imax + 1; 		/* descriptor map min */
	dmax = max; 			/* descriptor map max */

	/* I-Space instruction map */
	error = vm_idspace_init(uspace->idspace, &uspace->uisa_space, mtype, uisa_map, imin, imax, object, size, pageable);
	if (error != 0) {
		return;
	}
#ifdef deprecated
	uisa_map = vm_idspace_map_allocate(object, offset, imin, imax, size, pageable);
	if (uisa_map != NULL) {
		uspace->addr_map = uisa_map;
		uspace->i_start = &imin;
		uspace->i_end = &imax;
	}
#endif

	/* I-Space descriptor map */
	error = vm_idspace_init(uspace->idspace, &uspace->uisd_space, mtype, uisd_map, dmin, dmax, object, size, pageable);
	if (error != 0) {
		return;
	}
#ifdef deprecated
	uisd_map = vm_idspace_map_allocate(object, offset, dmin, dmax, size, pageable);
	if (uisd_map != NULL) {
		uspace->desc_map = uisd_map;
		uspace->d_start = &dmin;
		uspace->d_end = &dmax;
	}
#endif
}

void
vm_udspace_map_init(uspace, mtype, object, offset, min, max, size, pageable)
	vm_uspace_t uspace;
	int mtype;
	vm_object_t object;
	vm_offset_t offset;
	vm_offset_t *min, *max;
	vm_size_t size;
	bool_t pageable;
{
	vm_offset_t *imin, *dmin;
	vm_offset_t *imax, *dmax;
	int error;

	udspace_min = (char *)min;
	udspace_max = (char *)max;

	/*
	 * These may need to be adjusted to account for the map/s needing more or less space,
	 * instead of an even split.
	 */
	imin = min; 			/* instruction map min */
	imax = ((max-min)/2); 	/* instruction map max */
	dmin = imax + 1; 		/* descriptor map min */
	dmax = max; 			/* descriptor map max */

	/* D-Space instruction map */
	error = vm_idspace_init(uspace->idspace, &uspace->udsa_space, mtype, udsa_map, imin, imax, object, size, pageable);
	if (error != 0) {
		return;
	}
#ifdef deprecated
	udsa_map = vm_idspace_map_allocate(object, offset, imin, imax, size, pageable);
	if (udsa_map != NULL) {
		uspace->addr_map = udsa_map;
		uspace->i_start = &imin;
		uspace->i_end = &imax;
	}
#endif

	/* D-Space descriptor map */
	error = vm_idspace_init(uspace->idspace, &uspace->udsd_space, mtype, udsd_map, dmin, dmax, object, size, pageable);
	if (error != 0) {
		return;
	}
#ifdef deprecated
	udsd_map = vm_idspace_map_allocate(object, offset, dmin, dmax, size, pageable);
	if (udsd_map != NULL) {
		uspace->desc_map = udsd_map;
		uspace->d_start = &dmin;
		uspace->d_end = &dmax;
	}
#endif
}

void
vm_uspace_init(min, max)
	vm_offset_t min, max;
{
	vm_uspace_t uspace;
	vm_size_t size;

	/* Allocate Uspace */
	MALLOC(uspace, struct vm_uspace *, sizeof(struct vm_uspace *), M_VMUSPACE, M_WAITOK);
	if (min < USPACE_MIN && max > USPACE_MAX) {
		FREE(uspace, M_VMUSPACE);
		return;
	}

	/* Set Object Size */
	size = (max - min);
#ifdef deprecated
	/* Allocate Object */
	vm_uspace_object_init(uspace, size, uspace_object);
#endif
	/* Init I-Space */
	vm_uispace_map_init(uspace, M_VMUSPACE, uspace_object, min, &min, &max, size, TRUE);

	/* Init D-Space */
	vm_udspace_map_init(uspace, M_VMUSPACE, uspace_object, min, &min, &max, size, TRUE);
#ifdef deprecated
	/* Init idspace */
	vm_idspace_init(uspace->idspace, uspace->object, min, M_VMUSPACE);
#endif
}

#ifdef deprecated
void
vm_uspace_object_init(uspace, size, object)
	vm_uspace_t uspace;
	vm_size_t size;
	vm_object_t object;
{
	object = vm_object_allocate(size);
	if (object != NULL) {
		uspace->object = object;
	}
}
#endif

/* uspace regions */
void
vm_uspace_region_insert(uspace, segnum)
	vm_uspace_t uspace;
	int segnum;
{
	vm_idspace_t idspace;
	vm_segment_region_t region;

	idspace = uspace->idspace;
	if (idspace == NULL) {
		return;
	}
	region = vm_segment_region_lookup(idspace, segnum);
	if (region == NULL) {
		region = vm_segment_region_alloc(M_VMUSPACE);
	}
	vm_segment_region_insert(idspace, region, segnum, M_VMUSPACE);
}

void
vm_uspace_region_remove(uspace, segnum)
	vm_uspace_t uspace;
	int segnum;
{
	vm_idspace_t idspace;

	idspace = uspace->idspace;
	if (idspace == NULL) {
		return;
	}
	vm_segment_region_remove(idspace, segnum);
}

vm_segment_region_t
vm_uspace_region_lookup(uspace, segnum)
	vm_uspace_t uspace;
	int segnum;
{
	vm_idspace_t idspace;
	vm_segment_region_t region;

	idspace = uspace->idspace;
	if (idspace == NULL) {
		return (NULL);
	}
	region = vm_segment_region_lookup(idspace, segnum);
	if (region == NULL) {
		return (NULL);
	}
	return (region);
}
