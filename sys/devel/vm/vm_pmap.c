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

#include "vm_idspace.h"



vm_offset_t
vm_idspace_map_alloc(idspace, idspacemap, offset, size)
	vm_idspace_t idspace;
	vm_idspace_map_t idspacemap;
	vm_offset_t	offset;
	vm_size_t size;
{
	vm_offset_t	addr;
	int error;

	size = round_page(size);

	for (;;) {
		/*
		 * To make this work for more than one map,
		 * use the map's lock to lock out sleepers/wakers.
		 */
		vm_map_lock(idspacemap->map);
		if (vm_map_findspace(idspacemap->map, idspacemap->start, size, &addr) == 0) {
			break;
		}
		/* no space now; see if we can ever get space */
		if (vm_map_max(idspacemap->map) - vm_map_min(idspacemap->map) < size) {
			vm_map_unlock(idspacemap->map);
			return (0);
		}
		assert_wait(idspacemap->map, TRUE);
		vm_map_unlock(idspacemap->map);
		vm_thread_block();
	}
	error = vm_map_insert(idspacemap->map, idspace->object, offset, addr, (addr + size));
	if (error != 0) {
		vm_map_remove(idspacemap->map, addr, (addr + size));
		vm_map_unlock(idspacemap->map);
		return (0);
	}
	vm_map_unlock(idspacemap->map);
	return (addr);
}

void
vm_idspace_map_free(idspacemap, addr, size)
	vm_idspace_map_t idspacemap;
	vm_offset_t	addr;
	vm_size_t size;
{
	vm_map_lock(idspacemap->map);
	(void)vm_map_delete(idspacemap->map, trunc_page(addr), round_page(addr + size));
	vm_thread_wakeup(idspacemap->map);
	vm_map_unlock(idspacemap->map);
}


vm_idspace_map_insert(idspace, idspacemap, offset, size)
	vm_idspace_t idspace;
	vm_idspace_map_t idspacemap;
	vm_offset_t	offset;
	vm_size_t size;
{
	vm_offset_t	addr;

	addr = vm_idspace_map_alloc(idspace, offset, size);
	if (addr > 0) {
		idspacemap->addr = addr;
	}
}


vm_idspace_map_remove(idspace, idspacemap, addr, size)
	vm_idspace_t idspace;
	vm_idspace_map_t idspacemap;
	vm_offset_t	addr;
	vm_size_t size;
{
	if (idspacemap->map != NULL) {
		vm_idspace_map_free(idspacemap->map, addr, size);
	}
}

vm_offset_t
map_findspace(idspacemap, object)
	vm_idspace_map_t idspacemap;
	vm_object_t object;
{
	vm_map_t map;
	vm_offset_t	start, end, offset, addr;
	vm_size_t size;

	map = idspacemap->map;
	start = idspacemap->start;
	size = idspacemap->size;
	idspacemap->offset = offset;
	vm_map_lock(map);
	if (vm_map_findspace(map, start, size, &addr)) {
		vm_map_unlock(map);
		return (0);
	}
	offset = (addr - start);

	vm_map_insert(map, object, offset, addr, (addr + size));
	vm_map_unlock(map);
	return (addr);
}

static int
vm_idspace_map_check_entry(idspacemap, addr, offset)
	vm_idspace_map_t idspacemap;
	vm_offset_t addr;
	vm_offset_t offset;
{
	vm_map_t map;
	vm_map_entry_t entry, next;
	vm_object_t object;
	vm_offset_t estart, eend, eoffset, eaddr;
	vm_size_t esize;
	bool_t isentry;
	int error;

	map = idspacemap->map;
	isentry = vm_map_lookup_entry(map, addr, &entry);
retry:
	if (isentry) {
		estart = entry->start;
		eend = entry->end;
		eoffset = entry->offset;
		esize = round_novl(eend - estart);
		eaddr = (eoffset + esize);
		object = entry->object.vm_object;

		if ((estart < idspacemap->start) || (eend > idspacemap->end)
				|| (start > idspacemap->end) || (eend < idspacemap->start)) {
			error = ENOMEM;
			goto out;
		}
		if (eoffset != offset) {
			error = ENOMEM;
			goto out;
		}
		if (esize > idspacemap->size) {
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
	return (error);
}
