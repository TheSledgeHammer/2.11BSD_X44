/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm_page.c	8.4 (Berkeley) 1/9/95
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>

vm_offset_t	kentry_data;

/*
 * vm_pmap_bootstrap:
 *
 * Allocates virtual address space from pmap_bootstrap_alloc.
 */
void
vm_pmap_bootstrap(void)
{
    extern vm_offset_t	kentry_data;
    vm_size_t kentry_data_size, kmap_size, kentry_size;

    kmap_size = (MAX_KMAP * sizeof(struct vm_map));
	kentry_size = (MAX_KMAPENT * sizeof(struct vm_map_entry));
	kentry_data_size = round_page(kmap_size + kentry_size);
	kentry_data = (vm_offset_t)pmap_bootstrap_alloc(kentry_data_size);
}

/*
 * vm_pmap_bootinit:
 *
 * Allocates item from space made available by vm_pmap_bootstrap.
 */
void *
vm_pmap_bootinit(item, size, nitems)
	void 		*item;
	vm_size_t 	size;
	int 		nitems;
{
	extern vm_offset_t	kentry_data;
	vm_size_t free, totsize, result, itemsize;

	free = kentry_data;
	totsize = (size * nitems);
	result = (free - totsize);
	itemsize = (vm_size_t)(sizeof(item) + size);
	if ((free < totsize) || (itemsize > result)) {
		panic("vm_pmap_bootinit: not enough space allocated");
	}
	bzero(item, totsize);
	item = (void *)(vm_size_t)itemsize;
	kentry_data = result;
	return (item);
}


void
vm_pmap_bootstrap(vm_offset_t *data, vm_size_t map_size, unsigned long map_number, vm_size_t entry_size, unsigned long entry_number)
{
	vm_size_t entry_data_size, mapsize, entrysize;

    mapsize = (map_number * map_size);
	entrysize = (entry_number * entry_size);
	entry_data_size = round_page(mapsize + entrysize);
	*data = (vm_offset_t)pmap_bootstrap_alloc(entry_data_size);
}

/*
 * Based around 2.11BSD's phys system call
 * Setup u.uisa and u.uisd from pmap.
 */
vm_offset_t uisd_tmp, uisa_tmp;
void
vm_pmap_phys(map, va, start, end)
	vm_map_t map;
	vm_offset_t va, start, end;
{
	size = round_page(size);
	for (va = start; va < end; va += PAGE_SIZE) {

	}
	if (va) {
		uisd_tmp = va;
		uisa_tmp = pmap_extract(vm_map_pmap(map), va);
	}
}

#include "vm_idspace.h"

int
vm_idspace_region_allocate(idspace, segnum)
	vm_idspace_t idspace;
	int segnum;
{
	vm_segment_region_t region;

	region = vm_segment_region_alloc(idspace->mtype);
	if (region != NULL) {
		vm_segment_region_insert(idspace, region, segnum);
		idspace->region = region;
		return (0);
	}
	return (ENOMEM);
}

void
vm_idspace_region_deallocate(idspace, segnum)
	vm_idspace_t idspace;
	int segnum;
{
	vm_segment_region_t region;

	region = idspace->region;
	if (region != NULL) {
		vm_segment_region_remove(region, segnum);
		if (TAILQ_EMPTY(&idspace->header)) {
			vm_segment_region_free(region, idspace->mtype);
			idspace->region = region;
		}
	}
}

int
vm_idspace_region_read(idspace, segnum, addr, desc, flags, is_txt, is_ext, is_abs)
	vm_idspace_t idspace;
	int segnum, flags;
	vm_offset_t addr, desc;
	bool_t is_txt, is_ext, is_abs;
{
	vm_segment_region_t region;
	int error;

	region = idspace->region;
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
vm_idspace_region_write(idspace, segnum, addr, desc, flags, is_txt, is_ext, is_abs)
	vm_idspace_t idspace;
	int segnum, flags;
	vm_offset_t addr, desc;
	bool_t is_txt, is_ext, is_abs;
{
	vm_segment_region_t region;
	int error;

	region = idspace->region;
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

int
vm_idspace_map(idspace, idspacemap, val, size, segnum)
	vm_idspace_t idspace;
	vm_idspace_map_t idspacemap;
	vm_offset_t val;
	vm_size_t size;
	int segnum;
{
	vm_map_t map;
	vm_segment_region_t region;
	int error;

	error = vm_idspace_region_allocate(idspace, segnum);
	if (error != 0) {
		return (error);
	}
	region = idspace->region;
	if (region != NULL) {
		map = idspacemap->map;
		if (map != NULL) {
			/*
			 * allocated val with kmem if is_alloced is false and val equals
			 * 0.
			 */
			if ((idspacemap->is_alloced != TRUE) && (val == 0)) {
				val = kmem_alloc_wait(map, size);
				idspacemap->is_alloced = TRUE;
			}
		} else {
			return (ENOMEM);
		}
		error = vm_idspace_map_check_protection(idspace, idspacemap);
		if (error != 0) {
			return (error);
		}

		return (0);
	}
	return (ENOMEM);
}

int
vm_idspace_unmap(idspace, idspacemap, val, size, segnum)
	vm_idspace_t idspace;
	vm_idspace_map_t idspacemap;
	vm_offset_t val;
	vm_size_t size;
	int segnum;
{
	vm_map_t map;
	vm_segment_region_t region;
	int error;

	region = idspace->region;
	if (region != NULL) {
		map = idspacemap->map;
		if (map != NULL) {
			/*
			 * free val from kmem if is_alloced is true and val is greater
			 * than 0.
			 */
			if ((idspacemap->is_alloced != FALSE) && (val > 0)) {
				kmem_free_wakeup(map, val, size);
				idspacemap->is_alloced = FALSE;
			}
		} else {
			return (ENOMEM);
		}
		error = vm_idspace_map_check_protection(idspace, idspacemap);
		if (error != 0) {
			return (error);
		}
		vm_idspace_region_deallocate(idspace, segnum);
		return (0);
	}
	return (ENOMEM);
}

int
vm_idspace_map_protect(idspacemap, region, set_max)
	vm_idspace_map_t idspacemap;
	vm_segment_region_t region;
	bool_t	set_max;
{
	return (vm_map_protect(idspacemap->map, idspacemap->start, idspacemap->end,
			region->protect, set_max));
}

int
vm_idspace_map_check_protection(idspacemap, region)
	vm_idspace_map_t idspacemap;
	vm_segment_region_t region;
{
	return (vm_map_check_protection(idspacemap->map, idspacemap->start,
			idspacemap->end, region->protect));
}


static int
vm_idspace_map_check_map_entry(idspacemap, addr, size)
	vm_idspace_map_t idspacemap;
	vm_offset_t addr;
	vm_size_t size;
{
	vm_map_t map;
	vm_map_entry_t entry, next;
	vm_object_t object;
	vm_offset_t estart, eend, eoffset, eaddr;
	vm_size_t esize;
	bool_t isentry;
	int error;

	map = idspacemap->map;
	vm_map_lock_read(map);
	isentry = vm_map_lookup_entry(map, addr, &entry);
retry:
	if (isentry) {
		estart = entry->start;
		eend = entry->end;
		eoffset = entry->offset;
		esize = round_novl(eend - estart);
		eaddr = (eoffset + esize);
		object = entry->object.vm_object;

		if (eoffset != offset) {
			error = ENOMEM;
			goto out;
		}
		if (esize != size) {
			error = ENOMEM;
			goto out;
		}
		if (eaddr != addr) {
			error = ENOMEM;
			goto out;
		}
		if (object == NULL) {
			error = ENOMEM;
			goto out;
		}
		kmem_free_wakeup(map, eaddr, esize);
		error = 0;
		goto out;
	} else {
		CIRCLEQ_FOREACH(next, &map->cl_header, cl_entry) {
			if (next != entry) {
				estart = next->start;
				eend = next->end;
				eoffset = next->offset;
				esize = round_novl(eend - estart);
				eaddr = (eoffset + esize);

				if (eaddr <= addr) {
					isentry = vm_map_lookup_entry(map, addr, &entry);
					goto retry;
				} else {
					error = ENOMEM;
					goto out;
				}
			}  else {
				error = ENOMEM;
				goto out;
			}
		}
	}

out:
	vm_map_unlock_read(map);
	return (error);
}
