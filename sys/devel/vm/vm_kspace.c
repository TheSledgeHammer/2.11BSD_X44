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
static struct vm_kspace kernel_kspace_store;
vm_kspace_t kernel_kspace;
vm_object_t kspace_object;
char *kispace_min, *kispace_max; /* kernel i-space vm_map range */
char *kdspace_min, *kdspace_max; /* kernel d-space vm_map range */

vm_kspace_t vm_kspace_allocate(vm_size_t);
void vm_kspace_deallocate(vm_kspace_t);

static void vm_kspace_alloc(vm_offset_t, vm_offset_t, vm_size_t, vm_kspace_t);
static void vm_kispace_map_init(vm_kspace_t, int, vm_object_t, vm_offset_t *, vm_offset_t *, vm_size_t, bool_t);
static void vm_kdspace_map_init(vm_kspace_t, int, vm_object_t, vm_offset_t *, vm_offset_t *, vm_size_t, bool_t);
static int vm_kspace_map_alloc(vm_kspace_t, int, int);
static int vm_kspace_map_free(vm_kspace_t, int, int);
static int vm_kspace_save(vm_kspace_t, vm_offset_t, vm_size_t, int, int);
static int vm_kspace_restore(vm_kspace_t, vm_offset_t, vm_size_t, int, int);
static vm_offset_t *vm_kspace_map_offset(vm_kspace_t, vm_offset_t, bool_t, bool_t, int);

void
vm_kspace_init(void)
{
	kernel_kspace = &kernel_kspace_store;
	vm_kspace_alloc((KSPACE_MAX - KSPACE_MIN), KSPACE_MIN, KSPACE_MAX, kernel_kspace);
}

vm_kspace_t
vm_kspace_allocate(size)
	vm_size_t size;
{
	vm_kspace_t result;

	result = (vm_kspace_t)malloc(sizeof(*result), M_VMKSPACE, M_WAITOK);
	vm_kspace_alloc(result, KSPACE_MIN, KSPACE_MAX, size);
	return (result);
}

void
vm_kspace_deallocate(kspace)
	vm_kspace_t kspace;
{
	if (kspace != NULL) {
		if (kspace->idspace_i != NULL) {
			return;
		}
		if (kspace->idspace_d != NULL) {
			return;
		}
		free(kspace, M_VMKSPACE);
	}
}

static void
vm_kspace_alloc(min, max, size, kspace)
	vm_offset_t min, max;
	vm_size_t size;
	vm_kspace_t kspace;
{
	if (size > (max - min)) {
		vm_kspace_deallocate(kspace);
		panic("vm_kspace_allocate: unable to allocate kspace, size is too big");
		return;
	}

	/* Init I-Space */
	vm_kispace_map_init(kspace, M_VMKSPACE, kspace_object, &min, &max, size, TRUE);

	/* Init D-Space */
	vm_kdspace_map_init(kspace, M_VMKSPACE, kspace_object, &min, &max, size, TRUE);
}

static void
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

static void
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

/* kspace maps */
static int
vm_kspace_map_alloc(kspace, segno, maptype)
	vm_kspace_t kspace;
	int segno, maptype;
{
	int error;

	if (kspace->idspace_i != NULL) {
		switch (maptype) {
		case KISA:
			error = vm_idspace_map(kspace->idspace_i, &kspace->kisa_space,
					segno);
			break;
		case KISD:
			error = vm_idspace_map(kspace->idspace_i, &kspace->kisd_space,
					segno);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	if (kspace->idspace_d != NULL) {
		switch (maptype) {
		case KDSA:
			error = vm_idspace_map(kspace->idspace_d, &kspace->kdsa_space,
					segno);
			break;
		case KDSD:
			error = vm_idspace_map(kspace->idspace_d, &kspace->kdsd_space,
					segno);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}
	return (error);
}

static int
vm_kspace_map_free(kspace, segno, maptype)
	vm_kspace_t kspace;
	int segno, maptype;
{
	int error;

	if (kspace->idspace_i != NULL) {
		switch (maptype) {
		case KISA:
			error = vm_idspace_unmap(kspace->idspace_i, &kspace->kisa_space,
					segno);
			break;
		case KISD:
			error = vm_idspace_unmap(kspace->idspace_i, &kspace->kisd_space,
					segno);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	if (kspace->idspace_d != NULL) {
		switch (maptype) {
		case KDSA:
			error = vm_idspace_unmap(kspace->idspace_d, &kspace->kdsa_space,
					segno);
			break;
		case KDSD:
			error = vm_idspace_unmap(kspace->idspace_d, &kspace->kdsd_space,
					segno);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}
	return (error);
}

static int
vm_kspace_save(kspace, addr, size, maptype, flags)
	vm_kspace_t kspace;
	vm_offset_t addr;
	vm_size_t size;
	int maptype, flags;
{
	int error, segno;

	segno = (NOVL + 1);
	error = vm_kspace_map_alloc(kspace, segno, maptype);
	if (error != 0) {
		goto free;
	}

	if (kspace->idspace_i != NULL) {
		switch (maptype) {
		case KISA:
			error = vm_idspace_save(kspace->idspace_i, &kspace->kisa_space,
					addr, size, flags);
			break;
		case KISD:
			error = vm_idspace_save(kspace->idspace_i, &kspace->kisd_space,
					addr, size, flags);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	if (kspace->idspace_d != NULL) {
		switch (maptype) {
		case KDSA:
			error = vm_idspace_save(kspace->idspace_d, &kspace->kdsa_space,
					addr, size, flags);
			break;
		case KDSD:
			error = vm_idspace_save(kspace->idspace_d, &kspace->kdsd_space,
					addr, size, flags);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	return (error);

free:
	error = vm_kspace_map_free(kspace, segno, maptype);
	if (error != 0) {
		return (error);
	}
	return (0);
}

static int
vm_kspace_restore(kspace, addr, size, maptype, flags)
	vm_kspace_t kspace;
	vm_offset_t addr;
	vm_size_t size;
	int maptype, flags;
{
	int error, segno;

	segno = (NOVL + 1);
	if (kspace->idspace_i != NULL) {
		switch (maptype) {
		case KISA:
			error = vm_idspace_save(kspace->idspace_i, &kspace->kisa_space,
					addr, size, flags);
			break;
		case KISD:
			error = vm_idspace_save(kspace->idspace_i, &kspace->kisd_space,
					addr, size, flags);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}

	if (kspace->idspace_d != NULL) {
		switch (maptype) {
		case KDSA:
			error = vm_idspace_restore(kspace->idspace_d, &kspace->kdsa_space,
					addr, size, flags);
			break;
		case KDSD:
			error = vm_idspace_restore(kspace->idspace_d, kspace->kdsd_space,
					addr, size, flags);
			break;
		default:
			error = ENOMEM;
			break;
		}
	}
	return (error);
}

static vm_offset_t *
vm_kspace_map_offset(kspace, offset, use_min, use_max, maptype)
	vm_kspace_t kspace;
	vm_offset_t offset;
	bool_t use_min, use_max;
	int maptype;
{
	vm_offset_t *val;

	if (kspace->idspace_i != NULL) {
		switch (maptype) {
		case KISA:
			val = vm_idspace_map_offset(kspace->idspace_i, &kspace->kisa_space,
					offset, use_min, use_max);
			break;
		case KISD:
			val = vm_idspace_map_offset(kspace->idspace_i, &kspace->kisd_space,
					offset, use_min, use_max);
			break;
		default:
			*val = 0;
			break;
		}
	}

	if (kspace->idspace_d != NULL) {
		switch (maptype) {
		case KDSA:
			val = vm_idspace_map_offset(kspace->idspace_d, &kspace->kdsa_space,
					offset, use_min, use_max);
			break;
		case KDSD:
			val = vm_idspace_map_offset(kspace->idspace_d, &kspace->kdsd_space,
					offset, use_min, use_max);
			break;
		default:
			*val = 0;
			break;
		}
	}
	return (val);
}

int
vm_kspace_saveseg5(addr, size, maptype)
	vm_offset_t addr;
	vm_size_t size;
	int maptype;
{
	return (vm_kspace_save(kernel_kspace, addr, size, maptype, SEGM_SEG5));
}

int
vm_kspace_restoreseg5(addr, size, maptype)
	vm_offset_t addr;
	vm_size_t size;
	int maptype;
{
	return (vm_kspace_restore(kernel_kspace, addr, size, maptype, SEGM_SEG5));
}

int
vm_kspace_saveseg6(addr, size, maptype)
	vm_offset_t addr;
	vm_size_t size;
	int maptype;
{
	return (vm_kspace_save(kernel_kspace, addr, size, maptype, SEGM_SEG6));
}

int
vm_kspace_restoreseg6(addr, size, maptype)
	vm_offset_t addr;
	vm_size_t size;
	int maptype;
{
	return (vm_kspace_restore(kernel_kspace, addr, size, maptype, SEGM_SEG6));
}

int
vm_kspace_saveseg56(addr, size, maptype)
	vm_offset_t addr;
	vm_size_t size;
	int maptype;
{
	return (vm_kspace_save(kernel_kspace, addr, size, maptype, SEGM_SEG56));
}

int
vm_kspace_restoreseg56(addr, size, maptype)
	vm_offset_t addr;
	vm_size_t size;
	int maptype;
{
	return (vm_kspace_restore(kernel_kspace, addr, size, maptype, SEGM_SEG56));
}

vm_offset_t *
vm_kspace_offset(addr, maptype)
	vm_offset_t addr;
	int maptype;
{
	return (vm_kspace_map_offset(kernel_kspace, addr, FALSE, FALSE, maptype));
}

vm_offset_t *
vm_kspace_min(maptype)
	int maptype;
{
	return (vm_kspace_map_offset(kernel_kspace, 0, TRUE, FALSE, maptype));
}

vm_offset_t *
vm_kspace_max(maptype)
	int maptype;
{
	return (vm_kspace_map_offset(kernel_kspace, 0, FALSE, TRUE, maptype));
}
