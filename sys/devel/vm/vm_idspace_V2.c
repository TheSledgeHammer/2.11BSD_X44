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

/* A revised version of vm_idspace.c and vm_pmap.c in devel/vm */

/* Code is based on 2.11BSD's PDP-11 code */

#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/null.h>

#include <vm/include/vm.h>
#include <vm_idspace.h>

struct vm_idspace_entry;
typedef struct vm_idspace_entry *vm_idspace_entry_t;

struct vm_idspace;
typedef struct vm_idspace *vm_idspace_t;

struct vm_segregion_queue;
TAILQ_HEAD(vm_segregion_queue, vm_segment_region);
struct vm_idspace_entry {
	struct vm_segregion_queue header;	/* list of regions */
	vm_segment_region_t region;			/* region back-pointer */
	vm_map_t map;						/* map */
	vm_offset_t start;					/* start address */
	vm_offset_t end;					/* end address */
	vm_size_t size;						/* size */
	vm_offset_t space;					/* temp storage */
	vm_object_t object;					/* object */
	vm_segment_t segment;				/* segment */
	vm_page_t page;						/* page */
	bool_t is_alloced;					/* is allocated (using kmem or omem) */
};

struct vm_idspace {
	struct vm_idspace_entry aspace;		/* address space (i.e kdsa_map, kisa_map, udsa_map, uisa_map) */
	struct vm_idspace_entry dspace;		/* descriptor space (i.e kdsd_map, kisd_map, udsd_map, uisd_map) */
	int mtype;							/* idspace malloctype */
};

/* generic registers */
struct vm_segment_register segregs[NOVL];

simple_lock_data_t vm_segment_region_lock;

static void vm_idspace_entry_alloc(vm_idspace_entry_t, vm_map_t, vm_offset_t,
		vm_offset_t, vm_size_t);
static int vm_idspace_entry_init(vm_idspace_entry_t, vm_map_t, vm_offset_t *,
		vm_offset_t *, vm_size_t, bool_t);
static int vm_idspace_entry_object_init(vm_idspace_entry_t, vm_object_t, vm_size_t);
static int vm_idspace_entry_segment_alloc(vm_idspace_entry_t, int);
static int vm_idspace_entry_page_alloc(vm_idspace_entry_t, int);

static int vm_segment_region_check_segment(vm_segment_region_t, vm_object_t, int);
static int vm_segment_region_check_page(vm_segment_region_t, int);
static void vm_segment_region_saveseg5(vm_segment_region_t, vm_offset_t *,
		vm_offset_t *);
static void vm_segment_region_saveseg6(vm_segment_region_t, vm_offset_t *,
		vm_offset_t *);
static void vm_genmap_get(int, vm_offset_t *, vm_offset_t *);
static void vm_genmap_put(int, vm_offset_t *, vm_offset_t *);
static void vm_savemap_get(int, vm_offset_t *, vm_offset_t *);
static void vm_savemap_put(int, vm_offset_t *, vm_offset_t *);

/*
 * vm_idspace
 */
int
vm_idspace_init(idspace, entry, mtype, map, min, max, object, size, pageable)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
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
		error = vm_idspace_entry_init(entry, map, min, max, size, pageable);
		if (error != 0) {
			vm_idspace_deallocate(idspace, entry, mtype);
			return (error);
		}
		error = vm_idspace_entry_object_init(entry, object, size);
		if (error != 0) {
			vm_idspace_deallocate(idspace, entry, mtype);
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
	idspace->mtype = mtype;
	simple_lock_init(&vm_segment_region_lock, "vm_segment_region_lock");
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
vm_idspace_deallocate(idspace, entry, mtype)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	int mtype;
{
	if (idspace != NULL) {
		if (entry != NULL) {
			return;
		}
		FREE(idspace, mtype);
	}
}

int
vm_idspace_map(idspace, entry, val, size, segnum)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	vm_offset_t val;
	vm_size_t size;
	int segnum;
{
	vm_map_t map;
	vm_segment_region_t region;
	int error;

	if (entry == NULL) {
		return (ENOMEM);
	}

	map = entry->map;
	if (map == NULL) {
		return (ENOMEM);
	}

	/*
	 * allocated val with kmem if is_alloced is false and val equals
	 * 0.
	 */
	if ((entry->is_alloced != TRUE) && (val == 0)) {
		val = kmem_alloc_wait(map, size);
		bcopy(val, entry->space, size);
		entry->is_alloced = TRUE;
	}

	error = vm_idspace_entry_region_allocate(idspace, entry, segnum);
	if (error != 0) {
		return (error);
	}

	region = entry->region;
	if (region == NULL) {
		return (ENOMEM);
	}

	error = vm_map_check_protection(entry->map, entry->start, entry->end, region->protect);
	if (error != 0) {
		return (error);
	}

	return (0);
}

int
vm_idspace_unmap(idspace, entry, val, size, segnum)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	vm_offset_t val;
	vm_size_t size;
	int segnum;
{
	vm_map_t map;
	vm_segment_region_t region;
	int error;

	if (entry == NULL) {
		return (ENOMEM);
	}

	map = entry->map;
	if (map == NULL) {
		return (ENOMEM);
	}

	/*
	 * free val from kmem if is_alloced is true and val is greater
	 * than 0.
	 */
	if ((entry->is_alloced != FALSE) && (val != 0)) {
		kmem_free_wakeup(map, val, size);
		bcopy(val, entry->space, size);
		entry->is_alloced = FALSE;
	}

	region = entry->region;
	if (region == NULL) {
		return (ENOMEM);
	}

	error = vm_map_check_protection(entry->map, entry->start, entry->end, region->protect);
	if (error != 0) {
		return (error);
	}

	vm_idspace_entry_region_deallocate(idspace, entry, segnum);
	return (0);
}

/*
 * vm_idspace_entry
 */
static void
vm_idspace_entry_alloc(entry, map, start, end, size)
	vm_idspace_entry_t entry;
	vm_map_t map;
	vm_offset_t start, end;
	vm_size_t size;
{
	TAILQ_INIT(&entry->header);
	entry->region = NULL;
	entry->map = map;
	entry->start = start;
	entry->end = end;
	entry->size = size;
	entry->space = 0;
	entry->is_alloced = FALSE;
}

static int
vm_idspace_entry_init(entry, map, min, max, size, pageable)
	vm_idspace_entry_t entry;
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
		vm_idspace_entry_alloc(entry, map, *min, *max, size);
		return (0);
	}
	return (1);
}

static int
vm_idspace_entry_object_init(entry, object, size)
	vm_idspace_entry_t entry;
	vm_object_t object;
	vm_size_t size;
{
	object = vm_object_allocate(size);
	if (object != NULL) {
		entry->object = object;
		return (0);
	}
	return (1);
}

static int
vm_idspace_entry_segment_alloc(entry, segnum)
	vm_idspace_entry_t entry;
	int segnum;
{
	vm_segment_t segment;
	vm_offset_t offset;

	if (entry->object == NULL) {
		return (1);
	}

	offset = segnum_to_segment_offset(segnum);
	segment = vm_segment_alloc(entry->object, offset);
	if (segment != NULL) {
		entry->segment = segment;
		return (0);
	}
	return (1);
}

static int
vm_idspace_entry_page_alloc(entry, segnum)
	vm_idspace_entry_t entry;
	int segnum;
{
	vm_page_t page;
	vm_offset_t offset;

	if (entry->segment == NULL) {
		return (1);
	}

	offset = segnum_to_page_offset(segnum);
	page = vm_page_alloc(entry->segment, offset);
	if (page != NULL) {
		entry->page = page;
		return (0);
	}
	return (1);
}

int
vm_idspace_entry_region_allocate(idspace, entry, segnum)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	int segnum;
{
	vm_segment_region_t region;

	region = vm_segment_region_alloc(idspace->mtype);
	if (region != NULL) {
		vm_segment_region_insert(entry, region, segnum);
		entry->region = region;
		return (0);
	}
	return (ENOMEM);
}

void
vm_idspace_entry_region_deallocate(idspace, entry, segnum)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	int segnum;
{
	vm_segment_region_t region;

	region = entry->region;
	if (region != NULL) {
		vm_segment_region_remove(region, segnum);
		if (TAILQ_EMPTY(&entry->header)) {
			vm_segment_region_free(region, idspace->mtype);
			entry->region = region;
		}
	}
}

int
vm_idspace_entry_region_read(entry, segnum, addr, desc, flags, is_txt, is_ext, is_abs)
	vm_idspace_entry_t entry;
	int segnum, flags;
	vm_offset_t addr, desc;
	bool_t is_txt, is_ext, is_abs;
{
	vm_segment_region_t region;
	int error;

	region = entry->region;
	if (region != NULL) {
		region->flags = flags;
		region->is_text = is_txt;
		region->is_extension = is_ext;
		region->is_abs = is_abs;
		error = vm_segment_register_read(region, segnum, addr, desc);
		if (error != 0) {
			return (error);
		}
		return (0);
	}
	return (1);
}

int
vm_idspace_entry_region_write(entry, segnum, addr, desc, flags, is_txt, is_ext, is_abs)
	vm_idspace_entry_t entry;
	int segnum, flags;
	vm_offset_t addr, desc;
	bool_t is_txt, is_ext, is_abs;
{
	vm_segment_region_t region;
	int error;

	region = entry->region;
	if (region != NULL) {
		region->flags = flags;
		region->is_text = is_txt;
		region->is_extension = is_ext;
		region->is_abs = is_abs;
		error = vm_segment_register_write(region, segnum, addr, desc);
		if (error != 0) {
			return (error);
		}
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
vm_segment_region_insert(entry, region, segnum)
	vm_idspace_entry_t entry;
	vm_segment_region_t region;
	int segnum;
{
	if ((entry == NULL) ||
			(region == NULL) ||
			(vm_idspace_entry_segment_alloc(entry, segnum) != 0) ||
			(vm_idspace_entry_page_alloc(entry, segnum) != 0)) {
		return;
	}

	region->segment = entry->segment;
	region->page = entry->page;
	region->segreg = &segregs[segnum];
	region->segnum = segnum;
	region->flags = 0;
	region->protect = VM_PROT_ALL;
	region->is_text = FALSE;
	region->is_extension = FALSE;
	region->is_abs = FALSE;

	simple_lock(&vm_segment_region_lock);
	TAILQ_INSERT_TAIL(&entry->header, region, segm);
	simple_unlock(&vm_segment_region_lock);
}

void
vm_segment_region_remove(entry, segnum)
	vm_idspace_entry_t entry;
	int segnum;
{
	vm_segment_region_t region;

	simple_lock(&vm_segment_region_lock);
	TAILQ_FOREACH(region, &entry->header, segm) {
		if (region->segnum == segnum) {
			if (vm_segment_region_check_segment(region, entry->object, segnum)
					&& vm_segment_region_check_page(region, segnum)) {
				TAILQ_REMOVE(&entry->header, region, segm);
				simple_unlock(&vm_segment_region_lock);
			}
		}
	}
}

vm_segment_region_t
vm_segment_region_lookup(entry, segnum)
	vm_idspace_entry_t entry;
	int segnum;
{
	vm_segment_region_t region;

	simple_lock(&vm_segment_region_lock);
	TAILQ_FOREACH(region, &entry->header, segm) {
		if (region->segnum == segnum) {
			if (vm_segment_region_check_segment(region, entry->object, segnum)
					&& vm_segment_region_check_page(region, segnum)) {
				simple_unlock(&vm_segment_region_lock);
				return (NULL);
			}
		}
	}
	simple_unlock(&vm_segment_region_lock);
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
