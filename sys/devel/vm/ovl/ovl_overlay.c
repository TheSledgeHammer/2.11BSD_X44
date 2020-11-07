/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Copyright (c) 2020
 *	Martin Kelly. All rights reserved.
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
 *	@(#)vm_kern.c	8.4 (Berkeley) 1/9/95
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

/*
 *	Overlay memory management.
 */

#include <devel/vm/ovl/ovl.h>

vm_offset_t
ovlmem_alloc(map, size)
	register ovl_map_t		map;
	register vm_size_t		size;
{
	vm_offset_t 			addr;
	register vm_offset_t 	offset;
	extern ovl_object_t		overlay_object;
	vm_offset_t 			i;

	ovl_map_lock(map);
	if (ovl_map_findspace(map, 0, size, &addr)) {
		ovl_map_unlock(map);
		return (0);
	}
	offset = addr - OVL_MIN_KERNEL_ADDRESS;
	ovl_object_reference(overlay_object);
	ovl_map_insert(map, overlay_object, offset, addr, addr + size);
	ovl_map_unlock(map);

	return (addr);
}

void
ovlmem_free(map, addr, size)
	ovl_map_t				map;
	register vm_offset_t	addr;
	vm_size_t				size;
{
	(void) ovl_map_remove(map, addr, (addr + size));
}

ovl_map_t
ovlmem_suballoc(parent, min, max, size)
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
	pmap_reference(ovl_map_pmap(parent));
	result = ovl_map_create(ovl_map_pmap(parent), *min, *max);
	if (result == NULL)
		panic("ovl_suballoc: cannot create submap");
	if ((ret = ovl_map_submap(parent, *min, *max, result)) != KERN_SUCCESS)
		panic("ovl_suballoc: unable to change range to submap");
	return (result);
}

vm_offset_t
ovlmem_malloc(map, size, canwait)
	register ovl_map_t		map;
	register vm_size_t		size;
	boolean_t				canwait;
{
	register vm_offset_t	offset, i;
	ovl_map_entry_t			entry;
	vm_offset_t				addr;
	extern ovl_object_t		omem_object;

	if (map != omem_map)
		panic("ovl_malloc_alloc: map != omem_map");

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
			panic("ovl_malloc: %s too small", map == omem_map ? "omem_map" : "mb_map");
		return (0);
	}
	offset = addr - ovl_map_min(omem_map);
	ovl_object_reference(omem_object);
	ovl_map_insert(map, omem_object, offset, addr, addr + size);

	/*
	 * Mark map entry as non-pageable.
	 * Assert: vm_map_insert() will never be able to extend the previous
	 * entry so there will be a new entry exactly corresponding to this
	 * address range and it will have wired_count == 0.
	 */
	if (!ovl_map_lookup_entry(map, addr, &entry) || entry->ovle_start != addr || entry->ovle_end != addr + size)
		panic("ovl_malloc: entry not found or misaligned");

	ovl_map_unlock(map);

	return (addr);
}

vm_offset_t
ovlmem_alloc_wait(map, size)
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
ovlmem_free_wakeup(map, addr, size)
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
ovlmem_init(start, end)
	vm_offset_t start, end;
{
	register ovl_map_t map;

	map = ovl_map_create(overlay_pmap, OVL_MIN_ADDRESS, end);
	ovl_map_lock(map);
	overlay_map = map;
	(void) ovl_map_insert(map, NULL, (vm_offset_t)0, OVL_MIN_ADDRESS, start);
	ovl_map_unlock(map);
}
