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

/* Separate I & D is currently not fully supported */
#define NONSEPARATE

#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/null.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>
#include <vm_idspace.h>

/*
 * segment registers:
 * Contains 16 Generic and 2 Save (18 Total)
 */
struct vm_segment_register segregs[NOVL+2];

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
 * Based around 2.11BSD's phys system call
 * Setup u.uisa and u.uisd from pmap.
 */

int
vm_pmap_find_phys(map, virt, phys, num, size, start, end)
	vm_map_t map;
	vm_offset_t *virt, *phys, *num;
	vm_size_t size;
	vm_offset_t start, end;
{
	pmap_t pmap;
	vm_offset_t stoso, stosa, nsegs, ovlrem;
	int i, j, error;

	pmap = vm_map_pmap(map);
	error = pmap_lookup(pmap, virt, phys, num, size, start, end);
	if (error != 0) {
		return (error);
	}

	/* convert npages to nsegments */
	nsegs = atos(ptoa(*num));
	if (nsegs > NOVL) {
		ovlrem = (nsegs - NOVL);
	}

	/* sanity check */
	for (i = 0; i < NOVL; i++) {
		stoso = segno_to_segment_offset(i);
		if (ovlrem > 0) {
			for (j = 0; j < ovlrem; j++) {
				stoso = segno_to_segment_offset(j);
				stosa = (stoso * nsegs);
				if (stosa == *virt) {
					return (0);
				}
			}
			return (1);
		}
		stosa = (stoso * NOVL);
		if (stosa == *virt) {
			return (0);
		}
	}
	return (1);
}

int
vm_pmap_phys(map, size, segno, start, end)
	vm_map_t map;
	vm_size_t size;
	int segno;
	vm_offset_t start, end;
{
	vm_offset_t virt, phys, num, data;
	int error, segnomax;

	error = vm_pmap_find_phys(map, &virt, &phys, &num, size, start, end);
	if (error != 0) {
		return (error);
	}

	segnomax = (NOVL/2); /* max of 8 */
	if ((segno < 0) || (segno > segnomax)) {
		error = EINVAL;
		goto bad;
	}

	if ((size < 0) || (size > (end - start))) {
		error = EINVAL;
		goto bad;
	}

#ifdef NONSEPARATE
	data = u.u_uisd[segno];
#else /* !NONSEPARATE */
	data = u.u_uisd[segno + segnomax];
#endif /* !NONSEPARATE */
	if ((data != 0) && ((data & SEGM_ABS) == 0)) {
		error = EINVAL;
		goto bad;
	}
#ifdef NONSEPARATE
	u.u_uisd[segno] = 0;
	u.u_uisa[segno] = 0;
#else /* !NONSEPARATE */
	u.u_uisd[segno + segnomax] = 0;
	u.u_uisa[segno + segnomax] = 0;
	if (!u.u_sep) {
		u.u_uisd[segno] = 0;
		u.u_uisa[segno] = 0;
	}
#endif /* !NONSEPARATE */
	if (size) {
#ifdef NONSEPARATE
		u.u_uisd[segno] = (novl_dmask(size, 1) | SEGM_RW | SEGM_ABS);
		u.u_uisa[segno] = phys;
#else /* !NONSEPARATE */
		u.u_uisd[segno + segnomax] = (novl_dmask(size, 1) | SEGM_RW | SEGM_ABS);
		u.u_uisa[segno + segnomax] = phys;
		if (!u.u_sep) {
			u.u_uisa[segno] = u.u_uisa[segno + segnomax];
			u.u_uisd[segno] = u.u_uisd[segno + segnomax];
		}
#endif /* !NONSEPARATE */
	}

	vm_sureg();
	return (0);

bad:
	return (error);
}

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
	vm_idspace_lock_init(idspace);
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

vm_offset_t
vm_idspace_map_offset(idspace, entry, offset, use_min, use_max)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	vm_offset_t offset;
	bool_t use_min, use_max;
{
	vm_map_t map;

	if (entry == NULL) {
		return (0);
	}

	map = entry->map;
	if (map == NULL) {
		return (0);
	}
	return (vm_map_offset(map, offset, use_min, use_max));
}

int
vm_idspace_map(idspace, entry, segno)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	int segno;
{
	vm_map_t map;
	vm_segment_region_t region;
	vm_offset_t start, end;
	vm_prot_t prot;
	int error;

	if (entry == NULL) {
		return (ENOMEM);
	}

	map = entry->map;
	start = entry->start;
	end = entry->end;
	if (map == NULL) {
		return (ENOMEM);
	}

	error = vm_idspace_entry_region_allocate(idspace, entry, segno);
	if (error != 0) {
		return (error);
	}

	region = entry->region;
	if (region == NULL) {
		return (ENOMEM);
	}

	prot = region->protect;
	error = vm_map_check_protection(map, start, end, prot);
	if (error != 0) {
		return (error);
	}
	return (0);
}

int
vm_idspace_unmap(idspace, entry, segno)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	int segno;
{
	vm_map_t map;
	vm_segment_region_t region;
	vm_offset_t start, end;
	vm_prot_t prot;
	int error;

	if (entry == NULL) {
		return (ENOMEM);
	}

	map = entry->map;
	region = entry->region;
	start = entry->start;
	end = entry->end;
	if (map == NULL) {
		return (ENOMEM);
	}

	if (region == NULL) {
		/* nothing to free */
		return (0);
	}

	prot = region->protect;
	error = vm_map_check_protection(map, start, end, prot);
	if (error != 0) {
		return (error);
	}
	vm_idspace_entry_region_deallocate(idspace, entry, segno);
	return (0);
}

int
vm_idspace_write(idspace, entry, addr, desc, size, segno, is_txt, is_ext)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	vm_offset_t addr, desc;
	vm_size_t size;
	int segno;
	bool_t is_txt, is_ext;
{
	int error;

	vm_idspace_lock(idspace);
	error = vm_pmap_phys(entry->map, size, segno, entry->start, entry->end);
	if (error != 0) {
		vm_idspace_unlock(idspace);
		return (error);
	}
	if ((desc != (vm_offset_t)u.u_uisd[segno]) && (addr != (vm_offset_t)u.u_uisa[segno])) {
		vm_idspace_unlock(idspace);
		return (ENOMEM);
	}
	error = vm_idspace_entry_region_write(entry, segno, addr, desc,
			(SEGM_RW | SEGM_ACCESS), is_txt, is_ext, TRUE);
	if (error != 0) {
		vm_idspace_unlock(idspace);
		return (error);
	}

	error = vm_map_protect(entry->map, entry->start, entry->end,
			entry->region->protect, FALSE);
	if (error != 0) {
		vm_idspace_unlock(idspace);
		return (error);
	}
	vm_idspace_unlock(idspace);
	return (0);
}

int
vm_idspace_read(idspace, entry, addr, desc, size, segno, is_txt, is_ext)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	vm_offset_t addr, desc;
	vm_size_t size;
	int segno;
	bool_t is_txt, is_ext;
{
	int error;

	vm_idspace_lock(idspace);
	error = vm_pmap_phys(entry->map, size, segno, entry->start, entry->end);
	if (error != 0) {
		vm_idspace_unlock(idspace);
		return (error);
	}
	if ((desc != u.u_uisd[segno]) && (addr != u.u_uisa[segno])) {
		vm_idspace_unlock(idspace);
		return (ENOMEM);
	}
	error = vm_idspace_entry_region_read(entry, segno, addr, desc,
			(SEGM_RW | SEGM_RO | SEGM_ACCESS), is_txt, is_ext, TRUE);
	if (error != 0) {
		vm_idspace_unlock(idspace);
		return (error);
	}
	vm_idspace_unlock(idspace);
	return (0);
}

int
vm_idspace_save(idspace, entry, val, size, flags)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	vm_offset_t val;
	vm_size_t size;
	int flags;
{
	int error;

	vm_idspace_lock(idspace);
	val = kmem_alloc_wait(entry->map, size);
	error = vm_idspace_entry_region_save(entry, val, size,
			(SEGM_SAVE | SEGM_RW | SEGM_ACCESS | flags));
	if (error != 0) {
		vm_idspace_unlock(idspace);
		return (error);
	}

	error = vm_map_protect(entry->map, entry->start, entry->end,
			entry->region->protect, FALSE);
	if (error != 0) {
		vm_idspace_unlock(idspace);
		return (error);
	}
	vm_idspace_unlock(idspace);
	return (0);
}

int
vm_idspace_restore(idspace, entry, val, size, flags)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	vm_offset_t val;
	vm_size_t size;
	int flags;
{
	int error;

	vm_idspace_lock(idspace);
	error = vm_idspace_entry_region_restore(entry, val, size,
			(SEGM_RESTORE | SEGM_RO | SEGM_RW | SEGM_ACCESS | flags));
	if (error != 0) {
		vm_idspace_unlock(idspace);
		return (error);
	}

	kmem_free_wakeup(entry->map, val, size);
	vm_idspace_unlock(idspace);
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
vm_idspace_entry_segment_alloc(entry, segno)
	vm_idspace_entry_t entry;
	int segno;
{
	vm_segment_t segment;
	vm_offset_t offset;

	if (entry->object == NULL) {
		return (1);
	}

	offset = segno_to_segment_offset(segno);
	segment = vm_segment_alloc(entry->object, offset);
	if (segment != NULL) {
		entry->segment = segment;
		return (0);
	}
	return (1);
}

static int
vm_idspace_entry_page_alloc(entry, segno)
	vm_idspace_entry_t entry;
	int segno;
{
	vm_page_t page;
	vm_offset_t offset;

	if (entry->segment == NULL) {
		return (1);
	}

	offset = segno_to_page_offset(segno);
	page = vm_page_alloc(entry->segment, offset);
	if (page != NULL) {
		entry->page = page;
		return (0);
	}
	return (1);
}

int
vm_idspace_entry_region_allocate(idspace, entry, segno)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	int segno;
{
	vm_segment_region_t region;

	region = vm_segment_region_alloc(idspace->mtype);
	if (region != NULL) {
		vm_segment_region_insert(entry, region, segno);
		entry->region = region;
		return (0);
	}
	return (ENOMEM);
}

void
vm_idspace_entry_region_deallocate(idspace, entry, segno)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	int segno;
{
	vm_segment_region_t region;

	region = entry->region;
	if (region != NULL) {
		vm_segment_region_remove(region, segno);
		if (TAILQ_EMPTY(&entry->header)) {
			vm_segment_region_free(region, idspace->mtype);
			entry->region = region;
		}
	}
}

int
vm_idspace_entry_region_read(entry, segno, addr, desc, flags, is_txt, is_ext, is_abs)
	vm_idspace_entry_t entry;
	int segno, flags;
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
		error = vm_segment_register_read(region, segno, addr, desc);
		if (error != 0) {
			return (error);
		}
		return (0);
	}
	return (1);
}

int
vm_idspace_entry_region_write(entry, segno, addr, desc, flags, is_txt, is_ext, is_abs)
	vm_idspace_entry_t entry;
	int segno, flags;
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
		error = vm_segment_register_write(region, segno, addr, desc);
		if (error != 0) {
			return (error);
		}
		return (0);
	}
	return (1);
}

int
vm_idspace_entry_region_save(entry, addr, desc, flags)
	vm_idspace_entry_t entry;
	vm_offset_t addr, desc;
	int flags;
{
	vm_segment_region_t region;
	int error;

	region = entry->region;
	if (region != NULL) {
		region->flags = flags;
		region->is_text = FALSE;
		region->is_extension = FALSE;
		region->is_abs = FALSE;
		error = vm_segment_register_save(region, addr, desc, flags);
		if (error != 0) {
			return (error);
		}
		return (0);
	}
	return (1);
}

int
vm_idspace_entry_region_restore(entry, addr, desc, flags)
	vm_idspace_entry_t entry;
	vm_offset_t addr, desc;
	int flags;
{
	vm_segment_region_t region;
	int error;

	region = entry->region;
	if (region != NULL) {
		region->flags = flags;
		region->is_text = FALSE;
		region->is_extension = FALSE;
		region->is_abs = FALSE;
		error = vm_segment_register_restore(region, addr, desc, flags);
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
vm_segment_region_check_segment(region, object, segno)
	vm_segment_region_t region;
	vm_object_t object;
	int segno;
{
	vm_segment_t segment;
	vm_offset_t offset;

	offset = segno_to_segment_offset(segno);
	segment = vm_segment_lookup(object, offset);
	if (region->segment == segment) {
		return (0);
	}
	return (1);
}

static int
vm_segment_region_check_page(region, segno)
	vm_segment_region_t region;
	int segno;
{
	vm_page_t page;
	vm_offset_t offset;

	offset = segno_to_page_offset(segno);
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
vm_segment_region_insert(entry, region, segno)
	vm_idspace_entry_t entry;
	vm_segment_region_t region;
	int segno;
{
	if ((entry == NULL) ||
			(region == NULL) ||
			(vm_idspace_entry_segment_alloc(entry, segno) != 0) ||
			(vm_idspace_entry_page_alloc(entry, segno) != 0)) {
		return;
	}

	region->segment = entry->segment;
	region->page = entry->page;
	region->segreg = &segregs[segno];
	region->segno = segno;
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
vm_segment_region_remove(entry, segno)
	vm_idspace_entry_t entry;
	int segno;
{
	vm_segment_region_t region;

	simple_lock(&vm_segment_region_lock);
	TAILQ_FOREACH(region, &entry->header, segm) {
		if (region->segno == segno) {
			if (vm_segment_region_check_segment(region, entry->object, segno)
					&& vm_segment_region_check_page(region, segno)) {
				TAILQ_REMOVE(&entry->header, region, segm);
			}
		}
		simple_unlock(&vm_segment_region_lock);
	}
}

vm_segment_region_t
vm_segment_region_lookup(entry, segno)
	vm_idspace_entry_t entry;
	int segno;
{
	vm_segment_region_t region;

	simple_lock(&vm_segment_region_lock);
	TAILQ_FOREACH(region, &entry->header, segm) {
		if (region->segno == segno) {
			if (vm_segment_region_check_segment(region, entry->object, segno)
					&& vm_segment_region_check_page(region, segno)) {
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
 * returns 0 on success or 1 if unsuccessful.
 */
int
vm_segment_register_write(region, segno, addr, desc)
	vm_segment_region_t region;
	int segno;
	vm_offset_t *addr, *desc;
{
	if (region == NULL) {
		return (1);
	}
	if (region->protect & VM_PROT_WRITE) {
		if (region->flags & SEGM_SAVE) {
			if (segno <= NOVL) {
				goto bad;
			}
			switch (region->flags) {
			case SEGM_SEG5:
				vm_segment_region_saveseg5(region, addr, desc);
				vm_segmap_put((NOVL + 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				break;
			case SEGM_SEG6:
				vm_segment_region_saveseg6(region, addr, desc);
				vm_segmap_put((NOVL + 2), &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			case SEGM_SEG56:
				vm_segment_region_saveseg5(region, addr, desc);
				vm_savemap_put((NOVL + 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				vm_segment_region_saveseg6(region, addr, desc);
				vm_savemap_put((NOVL + 2), &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			default:
bad:
				panic("vm_segment_register_write: no valid save register specified");
				return (1);
			}
			goto out;
		}
		vm_genmap_put(segno, addr, desc);
	}

out:
	if (region->segreg == &segregs[segno]) {
		if ((region->segreg->addr == addr) && (region->segreg->desc == desc)) {
			return (0);
		}
	}
	return (1);
}

/*
 * Reads from a segment register.
 * returns 0 on success or 1 if unsuccessful.
 */
int
vm_segment_register_read(region, segno, addr, desc)
	vm_segment_region_t region;
	int segno;
	vm_offset_t *addr, *desc;
{
	if (region == NULL) {
		return (1);
	}
	if (region->protect & VM_PROT_READ) {
		if (region->flags & SEGM_RESTORE) {
			if (segno <= NOVL) {
				goto bad;
			}
			switch (region->flags) {
			case SEGM_SEG5:
				vm_savemap_get((NOVL + 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				break;
			case SEGM_SEG6:
				vm_savemap_get((NOVL + 2), &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			case SEGM_SEG56:
				vm_savemap_get((NOVL + 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				vm_savemap_get((NOVL + 2), &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			default:
bad:
				panic("vm_segment_register_read: no valid save register specified");
				return (1);
			}
			goto out;
		}
		vm_genmap_get(segno, addr, desc);
	}

out:
	if (region->segreg == &segregs[segno]) {
		if ((region->segreg->addr == addr) && (region->segreg->desc == desc)) {
			return (0);
		}
	}
	return (1);
}

/*
 * Saves to segment register.
 */
int
vm_segment_register_save(region, addr, desc, flags)
	vm_segment_region_t region;
	vm_offset_t *addr, *desc;
	int flags;
{
	int segno;

	switch (flags) {
	case SEGM_SEG5:
		segno = (NOVL + 1);
		break;
	case SEGM_SEG6:
		segno = (NOVL + 2);
		break;
	case SEGM_SEG56:
		segno = (NOVL + 1);
		break;
	default:
		return (1);
	}
	return (vm_segment_register_write(region, segno, addr, desc));
}

/*
 * Restores from segment register.
 */
int
vm_segment_register_restore(region, addr, desc, flags)
	vm_segment_region_t region;
	vm_offset_t *addr, *desc;
	int flags;
{
	int segno;

	switch (flags) {
	case SEGM_SEG5:
		segno = (NOVL + 1);
		break;
	case SEGM_SEG6:
		segno = (NOVL + 2);
		break;
	case SEGM_SEG56:
		segno = (NOVL + 1);
		break;
	default:
		return (1);
	}
	return (vm_segment_register_read(region, segno, addr, desc));
}

/* vm_segment_register: infomap */
static void
vm_genmap_get(segno, addr, desc)
	int segno;
	vm_offset_t *addr, *desc;
{
	if (&segregs[segno] != NULL) {
		if ((segno >= 0) && (segno <= NOVL)) {
			*addr = segregs[segno].addr;
			*desc = segregs[segno].desc;
		}
	}
}

static void
vm_genmap_put(segno, addr, desc)
	int segno;
	vm_offset_t *addr, *desc;
{
	if ((addr != NULL) && (desc != NULL)) {
		if ((segno >= 0) && (segno <= NOVL)) {
			segregs[segno].addr = *addr;
			segregs[segno].desc = *desc;
		}
	}
}

/* vm_segment_register savemap */
static void
vm_savemap_get(segno, addr, desc)
	int segno;
	vm_offset_t *addr, *desc;
{
	if (&segregs[segno] != NULL) {
		if ((segno >= (NOVL + 1)) && (segno <= (NOVL + 2))) {
			*addr = segregs[segno].addr;
			*desc = segregs[segno].desc;
		}
	}
}

static void
vm_savemap_put(segno, addr, desc)
	int segno;
	vm_offset_t *addr, *desc;
{
	if ((addr != NULL) && (desc != NULL)) {
		if ((segno >= (NOVL + 1)) && (segno <= (NOVL + 2))) {
			segregs[segno].addr = *addr;
			segregs[segno].desc = *desc;
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
