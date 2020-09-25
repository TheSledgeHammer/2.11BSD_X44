/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 *
 * @(#)kern_overlay.c	1.00
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include "vm/ovl/koverlay.h"

//static size_t koverlay_ex_storage[EXTENT_FIXED_STORAGE_SIZE(NOVLSR)];
//koverlay_ex_storage = extent_create(kovl->kovl_name, kovl->kovl_start, kovl->kovl_end, (void *)koverlay_ex_storage, sizeof(koverlay_ex_storage), EX_NOCOALESCE | EX_NOWAIT);

/* Kernel Overlay Memory Management */

/* Allocate a region of memory for Kernel Overlay's */
struct koverlay *
koverlay_extent_create(kovl, start, end, addr, size)
	struct koverlay *kovl;
	u_long start, end;
	caddr_t addr;
	u_long size;
{
	if (kovl == NULL) {
		kovl = malloc(sizeof(*kovl), M_KOVL, M_NOWAIT);
		kovl->kovl_flags = KOVL_ALLOCATED;
	} else {
		memset(kovl, 0, sizeof(*kovl));
	}

	koverlay_init(kovl, start, end, addr, size);

	if (size == 0) {
		kovl->kovl_addr = addr;
	} else {
		kovl->kovl_extent = extent_create(kovl->kovl_name, start, end, M_KOVL, addr, size, EX_NOWAIT | EX_MALLOCOK);
		if(kovl->kovl_extent == NULL) {
			panic("%s:: unable to create kernel overlay space for ""0x%08lx-%#lx", __func__, addr, size);
		}
	}

	return (kovl);
}

static void
koverlay_init(kovl, start, end, addr, size)
	register struct koverlay *kovl;
	u_long start, end;
	caddr_t addr;
	u_long size;
{
	kovl->kovl_name = "kern_overlay";
	kovl->kovl_start = start;
	kovl->kovl_end = end;
	kovl->kovl_addr = addr;
	kovl->kovl_size = size;
	kovl->kovl_regcnt = 0;
	kovl->kovl_sregcnt = 0;
}

/* Destroy a kernel overlay region */
void
koverlay_destroy(kovl)
	struct koverlay *kovl;
{
	struct extent *ex = kovl->kovl_extent;

	if(ex != NULL) {
		extent_destroy(ex);
	}

	if(kovl->kovl_flags & KOVL_ALLOCATED) {
		free(kovl, M_KOVL);
	}
}

/* Allocate a region for kernel overlay */
int
koverlay_extent_alloc_region(kovl, size)
	struct koverlay *kovl;
	u_long size;
{
	struct extent *ex = kovl->kovl_extent;
	int error;

	if(ex == NULL) {
		return (0);
	}
	if(NOVL <= 1) {
		NOVL = 1;
	}
	if(kovl->kovl_regcnt < NOVL) {
		kovl->kovl_addr += ex->ex_start;
		error = extent_alloc_region(ex, kovl->kovl_addr, size, EX_NOWAIT | EX_MALLOCOK);
	} else {
		printf("No more overlay regions currently available");
	}

	if(error) {
		return (error);
	}

	kovl->kovl_regcnt++;
	return (0);
}

int
koverlay_extent_alloc_subregion(kovl, name, start, end, size, alignment, boundary, result)
	struct koverlay *kovl;
	char *name;
	u_long start, end;
	u_long size, alignment, boundary;
	u_long *result;
{
	struct extent *ex = kovl->kovl_extent;
	int error;

	if(NOVL == 1) {
		NOVLSR = 1;
	}

	if(kovl->kovl_regcnt < NOVLSR) {
		if(start > kovl->kovl_start && start < kovl->kovl_end) {
			if(end > start) {
				error = extent_alloc_subregion(ex, start, end, size, alignment, boundary, EX_FAST | EX_NOWAIT | EX_MALLOCOK, result);
			} else {
				printf("overlay end address must be greater than the overlay start address");
			}
		} else {
			printf("overlay sub-region outside of kernel overlay allocated space");
		}
	} else {
		printf("No more overlay sub-regions currently available");
	}

	if(error) {
		return (error);
	}

	kovl->kovl_sregcnt++;
	return (0);
}

void
koverlay_free(kovl, start, size)
	struct koverlay *kovl;
	u_long start, size;
{
	struct extent *ex = kovl->kovl_extent;
	if(ex != NULL) {
		koverlay_unmap(kovl, start, size);
	}
	kovl->kovl_regcnt--;
}

static void
koverlay_unmap(kovl, start, size)
	struct koverlay *kovl;
	u_long start, size;
{
	struct extent *ex = kovl->kovl_extent;
	int error;
	if(ex == NULL) {
		return;
	}

	error = extent_free(ex, start, size, EX_NOWAIT);

	if (error) {
		printf("%#lx-%#lx of %s space lost\n", start, start + size, ex->ex_name);
	}
}

/* TODO: Allocate by type (kernel overlay or vm overlay) */
vm_offset_t
ovl_alloc(map, size)
	register ovl_map_t	map;
	register vm_size_t	size;
{
	vm_offset_t 			addr;
	register vm_offset_t 	offset;
	extern ovl_object_t 	kern_ovl_object;
	vm_offset_t 			i;

	ovl_map_lock(map);
	if (ovl_map_findspace(map, 0, size, &addr)) {
		ovl_map_unlock(map);
		return (0);
	}

	ovl_object_reference(kern_ovl_object);
	ovl_map_insert(map, kern_ovl_object, offset, addr, addr + size);
	ovl_map_unlock(map);
	/*
	ovl_object_lock(kernel_object);
	for (i = 0; i < size; i += PAGE_SIZE) {
		vm_segment_t seg;

		while ((seg = vm_segment_alloc(kern_ovl_object, offset + i)) == NULL) {
			ovl_object_unlock(kern_ovl_object);
			VM_WAIT;
			ovl_object_lock(kern_ovl_object);
		}
		vm_page_zero_fill(seg);
		seg->os_flags &= ~SEG_BUSY;
	}
	ovl_object_unlock(kern_ovl_object);
	*/
	ovl_map_simplify(map, addr);

	return (addr);
}

void
ovl_free(map, addr, size)
	ovl_map_t				map;
	register vm_offset_t	addr;
	vm_size_t				size;
{
	(void) ovl_map_remove(map, addr, (addr + size));
}

ovl_map_t
ovl_suballoc(parent, min, max, size)
	register ovl_map_t	parent;
	vm_offset_t			*min, *max;
	register vm_size_t	size;
{
	register int ret;
	ovl_map_t result;

	*min = (vm_offset_t) ovl_map_min(parent);
	ret = ovl_map_find(parent, NULL, (vm_offset_t) 0, min, size);
	if (ret != KERN_SUCCESS) {
		printf("ovl_suballoc: bad status return of %d.\n", ret);
		panic("ovl_suballoc");
	}
	*max = *min + size;

	result = ovl_map_create(parent, *min, *max);
	if (result == NULL)
		panic("ovl_suballoc: cannot create submap");
	if ((ret = ovl_map_submap(parent, *min, *max, result)) != KERN_SUCCESS)
		panic("ovl_suballoc: unable to change range to submap");
	return (result);
}

vm_offset_t
ovl_malloc(map, size, canwait)
	register ovl_map_t	map;
	register vm_size_t	size;
	boolean_t			canwait;
{
	register vm_offset_t	offset, i;
	ovl_map_entry_t			entry;
	vm_offset_t				addr;
	ovl_segment_t			m;
	extern ovl_object_t		kmem_object;

	if (map != kmem_map && map != mb_map)
		panic("ovl_malloc_alloc: map != {kmem,mb}_map");

	addr = ovl_map_min(map);

	/*
	 * Locate sufficient space in the map.  This will give us the
	 * final virtual address for the new memory, and thus will tell
	 * us the offset within the kernel map.
	 */
	ovl_map_lock(map);
	if (ovl_map_findspace(map, 0, size, &addr)) {
		ovl_map_unlock(map);
		if (canwait) /* XXX  should wait */
			panic("kmem_malloc: %s too small",
					map == kmem_map ? "kmem_map" : "mb_map");
		return (0);
	}
	offset = addr - ovl_map_min(kmem_map);
	ovl_object_reference(kmem_object);
	ovl_map_insert(map, kmem_object, offset, addr, addr + size);

	/*
	 * If we can wait, just mark the range as wired
	 * (will fault pages as necessary).
	 */
	/*
	if (canwait) {
		ovl_map_unlock(map);
		(void) vm_map_pageable(map, (vm_offset_t) addr, addr + size, FALSE);
		ovl_map_simplify(map, addr);
		return (addr);
	}
	*/

	/*
	 * If we cannot wait then we must allocate all memory up front,
	 * pulling it off the active queue to prevent pageout.
	 */
	ovl_object_lock(kmem_object);
	for (i = 0; i < size; i += PAGE_SIZE) {
		m = ovl_segment_alloc(kmem_object, offset + i);

		/*
		 * Ran out of space, free everything up and return.
		 * Don't need to lock page queues here as we know
		 * that the pages we got aren't on any queues.
		 */
		if (m == NULL) {
			while (i != 0) {
				i -= PAGE_SIZE;
				m = ovl_segment_lookup(kmem_object, offset + i);
				ovl_segment_free(m);
			}
			ovl_object_unlock(kmem_object);
			ovl_map_delete(map, addr, addr + size);
			ovl_map_unlock(map);
			return (0);
		}
#if 0
			vm_page_zero_fill(m);
#endif
		m->os_flags &= ~SEG_BUSY;
	}
	ovl_object_unlock(kmem_object);

	/*
	 * Mark map entry as non-pageable.
	 * Assert: vm_map_insert() will never be able to extend the previous
	 * entry so there will be a new entry exactly corresponding to this
	 * address range and it will have wired_count == 0.
	 */
	if (!ovl_map_lookup_entry(map, addr, &entry) || entry->ovle_start != addr || entry->ovle_end != addr + size)
		panic("ovl_malloc: entry not found or misaligned");

	/*
	 * Loop thru pages, entering them in the pmap.
	 * (We cannot add them to the wired count without
	 * wrapping the vm_page_queue_lock in splimp...)
	 */
	for (i = 0; i < size; i += PAGE_SIZE) {
		ovl_object_lock(kmem_object);
		m = vm_page_lookup(kmem_object, offset + i);
		ovl_object_unlock(kmem_object);
		pmap_enter(map->ovl_pmap, addr + i, VM_PAGE_TO_PHYS(m), VM_PROT_DEFAULT, TRUE);
	}
	ovl_map_unlock(map);

	ovl_map_simplify(map, addr);

	return (addr);
}

vm_offset_t
ovl_alloc_wait(map, size)
	ovl_map_t	map;
	vm_size_t	size;
{
	vm_offset_t	addr;

	for (;;) {
		/*
		 * To make this work for more than one map,
		 * use the map's lock to lock out sleepers/wakers.
		 */
		ovl_map_lock(map);
		if (ovl_map_findspace(map, 0, size, &addr) == 0)
			break;
		/* no space now; see if we can ever get space */
		if (ovl_map_max(map) - ovl_map_min(map) < size) {
			ovl_map_unlock(map);
			return (0);
		}
		assert_wait(map, TRUE);
		ovl_map_unlock(map);
		thread_block();
	}
	ovl_map_insert(map, NULL, (vm_offset_t)0, addr, addr + size);
	ovl_map_unlock(map);
	return (addr);
}

void
ovl_free_wakeup(map, addr, size)
	ovl_map_t	map;
	vm_offset_t	addr;
	vm_size_t	size;
{
	ovl_map_lock(map);
	(void) ovl_map_delete(map, addr, (addr + size));
	thread_wakeup(map);
	ovl_map_unlock(map);
}

void
ovl_init(start, end)
	vm_offset_t start, end;
{
	register ovl_map_t map;

	map = ovl_map_create(/* to add (kernel_pmap) */, OVL_MIN_ADDRESS, end);
	ovl_map_lock(map);
	(void) ovl_map_insert(map, NULL, (vm_offset_t)0, OVL_MIN_ADDRESS, start);
	ovl_map_unlock(map);
}
