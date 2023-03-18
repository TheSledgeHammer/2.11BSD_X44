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
 *	Kernel memory management.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <devel/vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <devel/vm/include/vm_page.h>
#include <devel/vm/include/vm_pageout.h>
#include <devel/vm/include/vm_segment.h>

vm_map_t	kernel_map;


/*
 *	kmem_alloc_pageable:
 *
 *	Allocate pageable memory to the kernel's address map.
 *	map must be "kernel_map" below.
 */
vm_offset_t
kmem_alloc_pageable(map, size)
	vm_map_t			map;
	register vm_size_t	size;
{
	vm_offset_t			addr;
	register int		result;

#if	0
	if (map != kernel_map)
		panic("kmem_alloc_pageable: not called with kernel_map");
#endif

	size = round_page(size);

	addr = vm_map_min(map);
	result = vm_map_find(map, NULL, (vm_offset_t) 0, &addr, size, TRUE);
	if (result != KERN_SUCCESS) {
		return (0);
	}
	return (addr);
}

/*
 *	Allocate wired-down memory in the kernel's address map
 *	or a submap.
 */
vm_offset_t
kmem_alloc(map, size)
	register vm_map_t	map;
	register vm_size_t	size;
{
	vm_offset_t				addr;
	register vm_offset_t	offset;
	vm_segment_t 			segment;
	vm_page_t	 			page;
	vm_size_t				sgsize;
	vm_offset_t 			i, j;
	extern vm_object_t		kernel_object;

	size = round_page(size);
	sgsize = round_segment(size);

	vm_map_lock(map);
	if (vm_map_findspace(map, 0, size, &addr)) {
		vm_map_unlock(map);
		return (0);
	}
	offset = addr - VM_MIN_KERNEL_ADDRESS;
	vm_object_reference(kernel_object);
	vm_map_insert(map, kernel_object, offset, addr, addr + size);
	vm_map_unlock(map);

	vm_object_lock(kernel_object);
	for (i = 0 ; i < sgsize; i+= SEGMENT_SIZE) {
		segment = vm_segment_alloc(kernel_object, offset+i);
		if (segment != NULL) {
			while (segment != NULL) {
				for (j = 0 ; j < size; j+= PAGE_SIZE) {
					page = vm_page_alloc(segment, offset+j);
					while (page == NULL) {
						vm_object_unlock(kernel_object);
						vm_wait();
						vm_object_lock(kernel_object);
					}
					vm_page_zero_fill(page);
					page->flags &= ~PG_BUSY;
				}
			}
		} else {
			while (segment == NULL) {
				vm_object_unlock(kernel_object);
				vm_wait();
				vm_object_lock(kernel_object);
			}
			vm_segment_zero_fill(segment);
			segment->sg_flags &= ~SEG_BUSY;
		}
	}
	vm_object_unlock(kernel_object);

	(void) vm_map_pageable(map, (vm_offset_t) addr, addr + size, FALSE);

	vm_map_simplify(map, addr);

	return (addr);
}


/*
 * Allocate wired-down memory in the kernel's address map for the higher
 * level kernel memory allocator (kern/kern_malloc.c).  We cannot use
 * kmem_alloc() because we may need to allocate memory at interrupt
 * level where we cannot block (canwait == FALSE).
 *
 * This routine has its own private kernel submap (kmem_map) and object
 * (kmem_object).  This, combined with the fact that only malloc uses
 * this routine, ensures that we will never block in map or object waits.
 *
 * Note that this still only works in a uni-processor environment and
 * when called at splhigh().
 *
 * We don't worry about expanding the map (adding entries) since entries
 * for wired maps are statically allocated.
 */
vm_offset_t
kmem_malloc(map, size, canwait)
	register vm_map_t	map;
	register vm_size_t	size;
	bool_t				canwait;
{
	register vm_offset_t	offset, i, j;
	vm_map_entry_t			entry;
	vm_offset_t				addr;
	vm_segment_t 			segment;
	vm_page_t				page;
	vm_size_t				sgsize;
	extern vm_object_t		kmem_object;

	if (map != kmem_map && map != mb_map) {
		panic("kern_malloc_alloc: map != {kmem,mb}_map");
	}
	size = round_page(size);
	sgsize = round_segment(size);
	addr = vm_map_min(map);

	vm_map_lock(map);
	if (vm_map_findspace(map, 0, size, &addr)) {
		vm_map_unlock(map);
		if (canwait) {		/* XXX  should wait */
			panic("kmem_malloc: %s too small", map == kmem_map ? "kmem_map" : "mb_map");
		}
		return (0);
	}
	offset = addr - vm_map_min(kmem_map);
	vm_object_reference(kmem_object);
	vm_map_insert(map, kmem_object, offset, addr, addr + size);

	/*
	 * If we can wait, just mark the range as wired
	 * (will fault pages as necessary).
	 */
	if (canwait) {
		vm_map_unlock(map);
		(void) vm_map_pageable(map, (vm_offset_t) addr, addr + size, FALSE);
		vm_map_simplify(map, addr);
		return (addr);
	}

	/*
	 * If we cannot wait then we must allocate all memory up front,
	 * pulling it off the active queue to prevent pageout.
	 */
	vm_object_lock(kmem_object);
	for (i = 0; i < sgsize; i += SEGMENT_SIZE) {
		segment = vm_segment_alloc(kernel_object, offset+i);
		if (segment != NULL) {
			for (j = 0 ; j < size; j+= PAGE_SIZE) {
				page = vm_page_alloc(segment, offset+j);
				if (page == NULL) {
					while (j != 0) {
						i -= PAGE_SIZE;
						page = vm_page_lookup(segment, offset + j);
						vm_page_free(page);
					}
					vm_object_unlock(kmem_object);
					vm_map_delete(map, addr, addr + size);
					vm_map_unlock(map);
					return (0);
				}
#if 0
				vm_page_zero_fill(page);
#endif
				page->flags &= ~PG_BUSY;
			}
		} else {
			if (segment == NULL) {
				while (i != 0) {
					i -= SEGMENT_SIZE;
					segment = vm_segment_lookup(kmem_object, offset + i);
					vm_segment_free(segment);
				}
				vm_object_unlock(kmem_object);
				vm_map_delete(map, addr, addr + size);
				vm_map_unlock(map);
				return (0);
			}
#if 0
			vm_segment_zero_fill(segment);
#endif
			segment->sg_flags &= ~SEG_BUSY;
		}
	}
	vm_object_unlock(kmem_object);

	if (!vm_map_lookup_entry(map, addr, &entry) ||
	    entry->start != addr || entry->end != addr + size ||
	    entry->wired_count) {
		panic("kmem_malloc: entry not found or misaligned");
	}
	entry->wired_count++;

	for (i = 0; i < sgsize; i += SEGMENT_SIZE) {
		vm_object_lock(kmem_object);
		segment = vm_segment_lookup(kmem_object, offset + i);
		if (segment != NULL) {
			for (j = 0; j < size; j += PAGE_SIZE) {
				page = vm_page_lookup(segment, offset + j);
			}
			vm_object_unlock(kmem_object);
			pmap_enter(map->pmap, addr + j, VM_PAGE_TO_PHYS(page), VM_PROT_DEFAULT, TRUE);
		} else {
			panic("kmem_malloc: segment not found or empty");
		}
	}
	vm_map_unlock(map);

	vm_map_simplify(map, addr);
	return (addr);
}

vm_size_t kmem_round(vm_size_t, int);
vm_size_t kmem_trunc(vm_size_t, int);

/*
 * kmem_round:
 * Rounds segment size to page size or vice versa.
 * Type:
 * 0 : page size to segment size
 * 1 : segment size to page size
 */
vm_size_t
kmem_round(vm_size_t size, int type)
{
    vm_size_t newsize = 0;
    vm_size_t sgsize = round_segment(size);
    vm_size_t pgsize = round_page(size);

	switch (type) {
	case 0:
		if (size == pgsize) {
			newsize = round_segment(pgsize);
			if (newsize == sgsize) {
				break;
			}
		}
		break;

	case 1:
		if (size == sgsize) {
			newsize = round_page(sgsize / size);
			if (newsize == pgsize) {
				break;
			}
		}
		break;

	default:
		 return (size);
	}
	return (newsize);
}

/*
 * kmem_trunc:
 * Truncates segment size to page size or vice versa.
 * Type:
 * 0 : page size to segment size
 * 1 : segment size to page size
 */
vm_size_t
kmem_trunc(vm_size_t size, int type)
{
    vm_size_t newsize = 0;
    vm_size_t sgsize = trunc_segment(size);
    vm_size_t pgsize = trunc_page(size);

	switch (type) {
	case 0:
		if (size == pgsize) {
			newsize = trunc_segment(pgsize);
			if (newsize == sgsize) {
				break;
			}
		}
		break;

	case 1:
		if (size == sgsize) {
			newsize = trunc_page(sgsize / size);
			if (newsize == pgsize) {
				break;
			}
		}
		break;

	default:
		 return (size);
	}
	return (newsize);
}
