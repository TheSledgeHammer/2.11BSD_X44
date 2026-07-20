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

#include <vm/include/vm.h>
#include <vm_idspace.h>

/*
 * idspace layout:
 * - An idspace is a vm_segment sub-divided into 16 regions.
 * - Each region contains 64 pages or is 262144 in size.
 * - Each region can contain a register.
 * - A register is similar to pmap (but different purpose),
 * with an address and descriptor.
 */

/* generic registers */
struct vm_segment_register segregs[NOVL];

simple_lock_data_t vm_idspace_lock;

static void vm_idspace_map_alloc(vm_idspace_map_t, vm_map_t, vm_offset_t,
		vm_offset_t, vm_size_t);
static int vm_idspace_map_init(vm_idspace_map_t, vm_map_t, vm_offset_t *,
		vm_offset_t *, vm_size_t, bool_t);
static int vm_idspace_object_init(vm_idspace_t, vm_object_t, vm_size_t);
static int vm_idspace_segment_alloc(vm_idspace_t, int);
static int vm_idspace_page_alloc(vm_idspace_t, int);

static int vm_segment_region_check_segment(vm_segment_region_t, vm_object_t, int);
static int vm_segment_region_check_page(vm_segment_region_t, int);
static void vm_segment_region_saveseg5(vm_segment_region_t, vm_offset_t *,
		vm_offset_t *);
static void vm_segment_region_saveseg6(vm_segment_region_t, vm_offset_t *,
		vm_offset_t *);
//static void vm_idspace_setup(vm_idspace_t, vm_object_t, vm_offset_t);
static void vm_genmap_get(int, vm_offset_t *, vm_offset_t *);
static void vm_genmap_put(int, vm_offset_t *, vm_offset_t *);
static void vm_savemap_get(int, vm_offset_t *, vm_offset_t *);
static void vm_savemap_put(int, vm_offset_t *, vm_offset_t *);

/*
 * vm_idspace
 */
int
vm_idspace_init(idspace, idspacemap, mtype, map, min, max, object, size, pageable)
	vm_idspace_t idspace;
	vm_idspace_map_t idspacemap;
	int mtype;
	vm_map_t map;
	vm_offset_t *min, *max;
	vm_object_t object;
	vm_size_t size;
	bool_t pageable;
{
	int error;

	idspace = vm_idspace_allocate(mtype);
	if (idspace != NULL) {
		error = vm_idspace_map_init(idspacemap, map, min, max, size, pageable);
		if (error != 0) {
			vm_idspace_deallocate(idspace, mtype);
			return (error);
		}
		error = vm_idspace_object_init(idspace, object, size);
		if (error != 0) {
			vm_idspace_deallocate(idspace, mtype);
			return (error);
		}
	}
	return (0);
}

static void
vm_idspace_alloc(idspace, mtype)
	vm_idspace_t idspace;
	int mtype;
{
	TAILQ_INIT(&idspace->header);
	simple_lock_init(&vm_idspace_lock, "vm_idspace_lock");
	idspace->mtype = mtype;
}

vm_idspace_t
vm_idspace_allocate(mtype)
	int mtype;
{
	register vm_idspace_t result;

	MALLOC(result, struct vm_idspace *, sizeof(struct vm_idspace *), mtype, M_WAITOK);
	if (result != NULL) {
		vm_idspace_alloc(result, mtype);
	}
	return (result);
}

void
vm_idspace_deallocate(idspace, mtype)
	vm_idspace_t idspace;
	int mtype;
{
	if (idspace != NULL) {
		if (!TAILQ_EMPTY(&idspace->header)) {
			return;
		}
		FREE(idspace, mtype);
	}
}

static void
vm_idspace_map_alloc(idspacemap, map, start, end, size)
	vm_idspace_map_t idspacemap;
	vm_map_t map;
	vm_offset_t start, end;
	vm_size_t size;
{
	idspacemap->map = map;
	idspacemap->start = start;
	idspacemap->end = end;
	idspacemap->size = size;
	idspacemap->addr = 0;
	idspacemap->desc = 0;
}

static int
vm_idspace_map_init(idspacemap, map, min, max, size, pageable)
	vm_idspace_map_t idspacemap;
	vm_map_t map;
	vm_offset_t *min, *max;
	vm_size_t size;
	bool_t pageable;
{
	if (size < *min) {
		size = *min;
	}
	if (size > *max) {
		size = *max;
	}

	map = kmem_suballoc(kernel_map, min, max, size, pageable);
	if (map != NULL) {
		if (size > (*max - *min)) {
			size = round_page(*max - *min);
		}
		vm_idspace_map_alloc(idspacemap, map, *min, *max, size);
		return (0);
	}
	return (1);
}

static int
vm_idspace_object_init(idspace, object, size)
	vm_idspace_t idspace;
	vm_object_t object;
	vm_size_t size;
{
	object = vm_object_allocate(size);
	if (object != NULL) {
		idspace->object = object;
		return (0);
	}
	return (1);
}

static int
vm_idspace_segment_alloc(idspace, segnum)
	vm_idspace_t idspace;
	int segnum;
{
	vm_segment_t segment;
	vm_offset_t offset;

	if (idspace->object == NULL) {
		return (1);
	}

	offset = segnum_to_segment_offset(segnum);
	segment = vm_segment_alloc(idspace->object, offset);
	if (segment != NULL) {
		idspace->segment = segment;
		return (0);
	}
	return (1);
}

static int
vm_idspace_page_alloc(idspace, segnum)
	vm_idspace_t idspace;
	int segnum;
{
	vm_page_t page;
	vm_offset_t offset;

	if (idspace->segment == NULL) {
		return (1);
	}

	offset = segnum_to_page_offset(segnum);
	page = vm_page_alloc(idspace->segment, offset);
	if (page != NULL) {
		idspace->page = page;
		return (0);
	}
	return (1);
}

/*
 * vm_segment_region
 */
static int
vm_segment_region_check_segment(region, object, segnum)
	vm_segment_region_t region;
	vm_object_t object;
	int segnum;
{
	vm_segment_t segment;
	vm_offset_t offset;

	offset = segnum_to_segment_offset(segnum);
	segment = vm_segment_lookup(object, offset);
	if (region->segment == segment) {
		return (0);
	}
	return (1);
}

static int
vm_segment_region_check_page(region, segnum)
	vm_segment_region_t region;
	int segnum;
{
	vm_page_t page;
	vm_offset_t offset;

	offset = segnum_to_page_offset(segnum);
	page = vm_page_lookup(region->segment, offset);
	if (region->page == page) {
		return (0);
	}
	return (1);
}

vm_segment_region_t
vm_segment_region_alloc(mtype)
	int mtype;
{
	vm_segment_region_t region;

	region = (vm_segment_region_t)malloc(
			(u_long)sizeof(struct vm_segment_region), mtype, M_WAITOK);
	if (region == NULL) {
		return (NULL);
	}
	return (region);
}

void
vm_segment_region_free(region, mtype)
	vm_segment_region_t region;
	int mtype;
{
	if (region != NULL) {
		free(region, mtype);
	}
}

void
vm_segment_region_insert(idspace, region, segnum)
	vm_idspace_t idspace;
	vm_segment_region_t region;
	int segnum;
{
	if ((idspace == NULL) ||
			(region == NULL) ||
			(vm_idspace_segment_alloc(idspace, segnum) != 0) ||
			(vm_idspace_page_alloc(idspace, segnum) != 0)) {
		return;
	}

	region->segment = idspace->segment;
	region->page = idspace->page;
	region->segreg = &segregs[segnum];
	region->segnum = segnum;
	region->flags = 0;
	region->protect = VM_PROT_ALL;
	region->is_text = FALSE;
	region->is_extension = FALSE;
	region->is_abs = FALSE;

	simple_lock(&vm_idspace_lock);
	TAILQ_INSERT_TAIL(&idspace->header, region, segm);
	simple_unlock(&vm_idspace_lock);
}

void
vm_segment_region_remove(idspace, segnum)
	vm_idspace_t idspace;
	int segnum;
{
	vm_segment_region_t region;

	simple_lock(&vm_idspace_lock);
	TAILQ_FOREACH(region, &idspace->header, segm) {
		if (region->segnum == segnum) {
			if (vm_segment_region_check_segment(region, idspace->object, segnum)
					&& vm_segment_region_check_page(region, segnum)) {
				TAILQ_REMOVE(&idspace->header, region, segm);
				simple_unlock(&vm_idspace_lock);
			}
		}
	}
}

vm_segment_region_t
vm_segment_region_lookup(idspace, segnum)
	vm_idspace_t idspace;
	int segnum;
{
	vm_segment_region_t region;

	simple_lock(&vm_idspace_lock);
	TAILQ_FOREACH(region, &idspace->header, segm) {
		if (region->segnum == segnum) {
			if (vm_segment_region_check_segment(region, idspace->object, segnum)
					&& vm_segment_region_check_page(region, segnum)) {
				simple_unlock(&vm_idspace_lock);
				return (NULL);
			}
		}
	}
	simple_unlock(&vm_idspace_lock);
	return (NULL);
}

/*
 * vm_segment_register
 */
/*
 * Write to a segment register.
 * segreg will not be null if successful.
 */
int
vm_segment_register_write(region, segnum, addr, desc)
	vm_segment_region_t region;
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (region == NULL) {
		return (1);
	}
	if (region->protect & VM_PROT_WRITE) {
		if (region->flags & SEGM_SAVE) {
			switch (region->flags) {
			case SEGM_SEG5:
				vm_segment_region_saveseg5(region, addr, desc);
				vm_segmap_put((NOVL - 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				break;
			case SEGM_SEG6:
				vm_segment_region_saveseg6(region, addr, desc);
				vm_segmap_put(NOVL, &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			case SEGM_SEG56:
				vm_segment_region_saveseg5(region, addr, desc);
				vm_savemap_put((NOVL - 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				vm_segment_region_saveseg6(region, addr, desc);
				vm_savemap_put(NOVL, &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			default:
				panic("vm_segment_register_write: no valid save register specified");
				break;
			}
			goto out;
		}
		vm_genmap_put(segnum, addr, desc);
	}

out:
	if (region->segreg == &segregs[segnum]) {
		if ((region->segreg->addr == addr) && (region->segreg->desc == desc)) {
			return (0);
		}
	}
	return (1);
}

/*
 * Reads from a segment register.
 * segreg will not be null if successful.
 */
int
vm_segment_register_read(region, segnum, addr, desc)
	vm_segment_region_t region;
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (region == NULL) {
		return (1);
	}
	if (region->protect & VM_PROT_READ) {
		if (region->flags & SEGM_RESTORE) {
			switch (region->flags) {
			case SEGM_SEG5:
				vm_savemap_get((NOVL - 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				break;
			case SEGM_SEG6:
				vm_savemap_get(NOVL, &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			case SEGM_SEG56:
				vm_savemap_get((NOVL - 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				vm_savemap_get(NOVL, &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			default:
				panic("vm_segment_register_read: no valid save register specified");
				break;
			}
			goto out;
		}
		vm_genmap_get(segnum, addr, desc);
	}

out:
	if (region->segreg == &segregs[segnum]) {
		if ((region->segreg->addr == addr) && (region->segreg->desc == desc)) {
			return (0);
		}
	}
	return (1);
}

/* vm_segment_register: infomap */
static void
vm_genmap_get(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (&segregs[segnum] != NULL) {
		if ((segnum >= 0) && (segnum <= (NOVL - 2))) {
			*addr = segregs[segnum].addr;
			*desc = segregs[segnum].desc;
		}
	}
}

static void
vm_genmap_put(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if ((addr != NULL) && (desc != NULL)) {
		if ((segnum >= 0) && (segnum <= (NOVL - 2))) {
			segregs[segnum].addr = *addr;
			segregs[segnum].desc = *desc;
		}
	}
}

/* vm_segment_register savemap */
static void
vm_savemap_get(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (&segregs[segnum] != NULL) {
		if ((segnum >= (NOVL - 1)) && (segnum <= NOVL)) {
			*addr = segregs[segnum].addr;
			*desc = segregs[segnum].desc;
		}
	}
}

static void
vm_savemap_put(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if ((addr != NULL) && (desc != NULL)) {
		if ((segnum >= (NOVL - 1)) && (segnum <= NOVL)) {
			segregs[segnum].addr = *addr;
			segregs[segnum].desc = *desc;
		}
	}
}

/*
 * vm_segment_region_saveseg5:
 * copy contents of the address and the descriptor to
 * seg5 mapstore.
 */
static void
vm_segment_region_saveseg5(region, addr, desc)
	vm_segment_region_t region;
	vm_offset_t *addr, *desc;
{
	bcopy(addr, &region->mapstore.kdsa5, sizeof(*addr));
	bcopy(desc, &region->mapstore.kdsd5, sizeof(*desc));
}

/*
 * vm_segment_region_saveseg6:
 * copy contents of the address and the descriptor to
 * seg6 mapstore.
 */
static void
vm_segment_region_saveseg6(region, addr, desc)
	vm_segment_region_t region;
	vm_offset_t *addr, *desc;
{
	bcopy(addr, &region->mapstore.kdsa6, sizeof(*addr));
	bcopy(desc, &region->mapstore.kdsd6, sizeof(*desc));
}


#ifdef deprecated
void
vm_idspace_init(idspace, object, offset, mtype)
	vm_idspace_t idspace;
	vm_object_t object;
	vm_offset_t offset;
	int mtype;
{
	idspace = vm_idspace_allocate(mtype);
	if (idspace != NULL) {
		vm_idspace_setup(idspace, object, offset);
		/* initialize first segment region */
		vm_region_insert(idspace, 0, mtype);
	}
}

static void
vm_idspace_setup(idspace, object, offset)
	vm_idspace_t idspace;
	vm_object_t object;
	vm_offset_t offset;
{
	vm_segment_t segment;
	int i;

	TAILQ_INIT(&idspace->header);
	simple_lock_init(&vm_idspace_lock, "vm_idspace_lock");

	/* setup segment */
	segment = vm_segment_alloc(object, offset);
	idspace->segment = segment;

	/* setup pages */
	for (i = 0; i < NOVL; i++) {
		idspace->pagemap[i] = vm_idspace_pagemap_allocate(segment, i);
	}
}


/* allocate and insert map */
vm_map_t
vm_idspace_map_allocate(object, offset, min, max, size, pageable)
	vm_object_t object;
	vm_offset_t offset, *min, *max;
	vm_size_t size;
	bool_t pageable;
{
	vm_map_t map;
	vm_offset_t start, end;
	int error;

	map = kmem_suballoc(kernel_map, min, max, size, pageable);
	if (map != NULL) {
		start = vm_map_min(map);
		end = vm_map_max(map);
		if ((start < *min) || (end > *max)) {
			return (NULL);
		}
		vm_map_lock(map);
		error = vm_map_insert(map, object, offset, start, end);
		if (error != KERN_SUCCESS) {
			vm_map_remove(map, start, end);
			vm_map_unlock(map);
			return (NULL);
		}
		vm_map_unlock(map);
	}
	return (map);
}

/* allocate pages for pagemap */
vm_page_t
vm_idspace_pagemap_allocate(segment, nelems)
	vm_segment_t segment;
	int nelems;
{
	vm_page_t pagemap[NOVL];
	vm_offset_t pgoffset;

	pgoffset = nelems + NOVL_PAGES * PAGE_SIZE;
	pagemap[nelems] = vm_page_alloc(segment, pgoffset);
	if (pagemap[nelems] != NULL) {
		return (pagemap[nelems]);
	}
	return (NULL);
}
#endif
