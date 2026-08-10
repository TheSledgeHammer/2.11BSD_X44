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
static struct vm_uspace kernel_uspace_store;
vm_uspace_t kernel_uspace;
vm_object_t uspace_object;
char *uispace_min, *uispace_max; /* user i-space vm_map range */
char *udspace_min, *udspace_max; /* user d-space vm_map range */

static void vm_uspace_alloc(vm_offset_t, vm_offset_t, vm_size_t, vm_uspace_t);
static void vm_uispace_map_init(vm_uspace_t, int, vm_object_t, vm_offset_t *, vm_offset_t *, vm_size_t, bool_t);
static void vm_udspace_map_init(vm_uspace_t, int, vm_object_t, vm_offset_t *, vm_offset_t *, vm_size_t, bool_t);

void
vm_uspace_init(void)
{
	kernel_uspace = &kernel_uspace_store;
	vm_uspace_alloc((USPACE_MAX - USPACE_MIN), USPACE_MIN, USPACE_MAX, kernel_uspace);
}

vm_uspace_t
vm_uspace_allocate(size)
	vm_size_t size;
{
	vm_uspace_t result;

	result = (vm_uspace_t)malloc(sizeof(*result), M_VMUSPACE, M_WAITOK);
	vm_uspace_alloc(result, USPACE_MIN, USPACE_MAX, size);
	return (result);
}

void
vm_uspace_deallocate(uspace)
	vm_uspace_t uspace;
{
	if (uspace != NULL) {
		if (uspace->idspace_i != NULL) {
			return;
		}
		if (uspace->idspace_d != NULL) {
			return;
		}
		free(uspace, M_VMUSPACE);
	}
}

static void
vm_uspace_alloc(min, max, size, uspace)
	vm_offset_t min, max;
	vm_size_t size;
	vm_uspace_t uspace;
{
	if (size > (max - min)) {
		vm_uspace_deallocate(uspace);
		panic("vm_uspace_allocate: unable to allocate uspace, size is too big");
		return;
	}

	/* Init I-Space */
	vm_uispace_map_init(uspace, M_VMUSPACE, uspace_object, &min, &max, size, TRUE);

	/* Init D-Space */
	vm_udspace_map_init(uspace, M_VMUSPACE, uspace_object, &min, &max, size, TRUE);
}

static void
vm_uispace_map_init(uspace, mtype, object, min, max, size, pageable)
	vm_uspace_t uspace;
	int mtype;
	vm_object_t object;
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
	error = vm_idspace_init(uspace->idspace_i, &uspace->uisa_space, mtype,
			uisa_map, imin, imax, object, size, pageable);
	if (error != 0) {
		return;
	}

	/* I-Space descriptor map */
	error = vm_idspace_init(uspace->idspace_d, &uspace->uisd_space, mtype,
			uisd_map, dmin, dmax, object, size, pageable);
	if (error != 0) {
		return;
	}
}

static void
vm_udspace_map_init(uspace, mtype, object, min, max, size, pageable)
	vm_uspace_t uspace;
	int mtype;
	vm_object_t object;
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
	error = vm_idspace_init(uspace->idspace_i, &uspace->udsa_space, mtype,
			udsa_map, imin, imax, object, size, pageable);
	if (error != 0) {
		return;
	}

	/* D-Space descriptor map */
	error = vm_idspace_init(uspace->idspace_d, &uspace->udsd_space, mtype,
			udsd_map, dmin, dmax, object, size, pageable);
	if (error != 0) {
		return;
	}
}

/* uspace maps */
int
vm_uspace_map_alloc(uspace, segno, maptype)
	vm_uspace_t uspace;
	int segno, maptype;
{
	vm_idspace_t idspace_i, idspace_d;
	int error;

	idspace_i = uspace->idspace_i;
	if (idspace_i != NULL) {
		switch (maptype) {
		case UISA:
			error = vm_idspace_map(idspace_i, uisa_space, segno);
			break;
		case UISD:
			error = vm_idspace_map(idspace_i, uisd_space, segno);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	idspace_d = uspace->idspace_d;
	if (idspace_d != NULL) {
		switch (maptype) {
		case UDSA:
			error = vm_idspace_map(idspace_d, udsa_space, segno);
			break;
		case UDSD:
			error = vm_idspace_map(idspace_d, udsd_space, segno);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}
	return (error);
}

int
vm_uspace_map_free(uspace, segno, maptype)
	vm_uspace_t uspace;
	int segno, maptype;
{
	vm_idspace_t idspace_i, idspace_d;
	int error;

	idspace_i = uspace->idspace_i;
	if (idspace_i != NULL) {
		switch (maptype) {
		case UISA:
			error = vm_idspace_unmap(idspace_i, uisa_space, segno);
			break;
		case UISD:
			error = vm_idspace_unmap(idspace_i, uisd_space, segno);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	idspace_d = uspace->idspace_d;
	if (idspace_d != NULL) {
		switch (maptype) {
		case UDSA:
			error = vm_idspace_unmap(idspace_d, udsa_space, segno);
			break;
		case UDSD:
			error = vm_idspace_unmap(idspace_d, udsd_space, segno);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}
	return (error);
}

int
vm_uspace_write(uspace, size, segno, maptype, is_txt, is_ext)
	vm_uspace_t uspace;
	vm_size_t size;
	int segno, maptype;
	bool_t is_txt, is_ext;
{
	vm_idspace_t idspace_i, idspace_d;
	vm_offset_t addr, desc;
	int error;

	error = vm_uspace_map_alloc(uspace, segno, maptype);
	if (error != 0) {
		(void)vm_uspace_map_free(uspace, segno, maptype);
		return (error);
	}

	desc = (vm_offset_t)u.u_uisd[segno];
	addr = (vm_offset_t)u.u_uisa[segno];

	idspace_i = uspace->idspace_i;
	if (idspace_i != NULL) {
		switch (maptype) {
		case UISA:
			error = vm_idspace_write(idspace_i, uisa_space, addr, desc, size, segno, is_txt, is_ext);
			break;
		case UISD:
			error = vm_idspace_write(idspace_i, uisd_space, addr, desc, size, segno, is_txt, is_ext);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	idspace_d = uspace->idspace_d;
	if (idspace_d != NULL) {
		switch (maptype) {
		case UDSA:
			error = vm_idspace_write(idspace_d, udsa_space, addr, desc, size, segno, is_txt, is_ext);
			break;
		case UDSD:
			error = vm_idspace_write(idspace_d, udsd_space, addr, desc, size, segno, is_txt, is_ext);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}
	return (error);
}

int
vm_uspace_read(uspace, size, segno, maptype, is_txt, is_ext)
	vm_uspace_t uspace;
	vm_size_t size;
	int segno, maptype;
	bool_t is_txt, is_ext;
{
	vm_idspace_t idspace_i, idspace_d;
	vm_offset_t addr, desc;
	int error;

	desc = (vm_offset_t)u.u_uisd[segno];
	addr = (vm_offset_t)u.u_uisa[segno];

	idspace_i = uspace->idspace_i;
	if (idspace_i != NULL) {
		switch (maptype) {
		case UISA:
			error = vm_idspace_read(idspace_i, uisa_space, addr, desc, size, segno, is_txt, is_ext);
			break;
		case UISD:
			error = vm_idspace_read(idspace_i, uisd_space, addr, desc, size, segno, is_txt, is_ext);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	idspace_d = uspace->idspace_d;
	if (idspace_d != NULL) {
		switch (maptype) {
		case UDSA:
			error = vm_idspace_read(idspace_d, udsa_space, addr, desc, size, segno, is_txt, is_ext);
			break;
		case UDSD:
			error = vm_idspace_read(idspace_d, udsd_space, addr, desc, size, segno, is_txt, is_ext);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}
	return (error);
}

vm_offset_t *
vm_uspace_offset(uspace, offset, use_min, use_max, maptype)
	vm_uspace_t uspace;
	vm_offset_t offset;
	bool_t use_min, use_max;
	int maptype;
{
	vm_idspace_t idspace_i, idspace_d;
	vm_offset_t *val;

	idspace_i = uspace->idspace_i;
	if (idspace_i != NULL) {
		switch (maptype) {
		case UISA:
			val = vm_idspace_map_offset(idspace_i, uisa_space, offset, use_min, use_max);
			break;
		case UISD:
			val = vm_idspace_map_offset(idspace_i, uisd_space, offset, use_min, use_max);
			break;
		default:
			*val = 0;
			break;
		}
	}

	idspace_d = uspace->idspace_d;
	if (idspace_d != NULL) {
		switch (maptype) {
		case UDSA:
			val = vm_idspace_map_offset(idspace_d, udsa_space, offset, use_min, use_max);
			break;
		case UDSD:
			val = vm_idspace_map_offset(idspace_d, udsd_space, offset, use_min, use_max);
			break;
		default:
			*val = 0;
			break;
		}
	}
	return (val);
}
