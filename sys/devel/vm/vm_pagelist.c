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
#include <sys/fnv_hash.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>
#include <vm/include/vm_map.h>
#include <vm/include/vm_pageout.h>

#define	VM_PAGE_ALLOC_MEMORY_STATS

#ifdef VM_PAGE_ALLOC_MEMORY_STATS
#define	STAT_INCR(v)	(v)++
#define	STAT_DECR(v)												\
	do { 															\
		if ((v) == 0) {												\
			printf("%s:%d -- Already 0!\n", __FILE__, __LINE__); 	\
		} else {													\
			(v)--; 													\
		}															\
	} while (0);

u_long	vm_page_alloc_memory_npages;
u_long  vm_page_alloc_memory_nsegments;
#else
#define	STAT_INCR(v)
#define	STAT_DECR(v)
#endif

#define pagelist_lock(segmented) {               	\
    if ((segmented) == TRUE) {                      \
        simple_lock(&vm_segment_list_free_lock);    \
    } else {                                        \
        simple_lock(&vm_page_queue_free_lock);      \
    }												\
}

#define pagelist_unlock(segmented) {              	\
    if ((segmented) == TRUE) {                      \
        simple_lock(&vm_segment_list_free_lock);    \
    } else {                                        \
        simple_lock(&vm_page_queue_free_lock);      \
    }    											\
}

static void vm_pagelist_add_paged_memory(vm_page_t, vm_segment_t, struct pglist *);
static void vm_pagelist_add_segmented_memory(vm_segment_t, struct seglist *);
static int vm_pagelist_alloc_memory(vm_size_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_offset_t, struct pglist *);
static int vm_pagelist_alloc_memory_contig(vm_size_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_offset_t, struct pglist *);

static int vm_pagelist_first(bool_t);
static int vm_pagelist_index(bool_t, vm_offset_t);
static long vm_pagelist_array_size(bool_t);
static vm_offset_t vm_pagelist_to_phys(bool_t, int);
static int vm_pagelist_is_physaddr(bool_t, vm_offset_t);
static int vm_pagelist_is_free(bool_t, int);
static int vm_pagelist_check_memory(vm_size_t);
static int vm_pagelist_found_chunk(int, int, bool_t, struct seglist *, struct pglist *);

/*
 *	vm_page_alloc_memory:
 *
 *	Allocate physical pages conforming to the restrictions
 *	provided:
 *
 *		size		The size of the allocation,
 *				rounded to page size.
 *
 *		low		The low address of the allowed
 *				allocation range.
 *
 *		high		The high address of the allowed
 *				allocation range.
 *
 *		alignment	Allocation must be aligned to this
 *				power-of-two boundary.
 *
 *		boundary	No segment in the allocation may
 *				cross this power-of-two boundary
 *				(relative to zero).
 *
 *	The allocated pages are placed at the tail of `rlist'; `rlist'
 *	is assumed to be properly initialized by the caller.  The
 *	number of memory segments that the allocated memory may
 *	occupy is specified in the `nsegs' arguement.
 *
 *	Returns 0 on success or an errno value to indicate mode
 *	of failure.
 *
 *	XXX This implementation could be improved.  It only
 *	XXX allocates a single segment.
 */

static void
vm_pagelist_add_paged_memory(m, seg, rlist)
	vm_page_t m;
	vm_segment_t seg;
	struct pglist *rlist;
{
	struct pglist *pgfree;
#ifdef DEBUG
	vm_page_t tp;
#endif

	pgfree = &vm_page_queue_free;
#ifdef DEBUG
	TAILQ_FOREACH(tp, pgfree, pageq) {
		if (tp == m) {
			break;
		}
	}
	if (tp == NULL) {
		panic("vm_page_alloc_memory: page not on freelist");
	}
#endif
	TAILQ_REMOVE(pgfree, m, pageq);
	cnt.v_page_free_count--;
	m->flags = PG_CLEAN;
	if (seg != NULL) {
		seg->object = NULL;
		m->segment = seg;
	} else {
		m->segment->object = NULL;
	}
	m->anon = NULL;
	m->wire_count = 0;
	TAILQ_INSERT_TAIL(rlist, m, pageq);
}

static void
vm_pagelist_add_segmented_memory(seg, slist)
	vm_segment_t seg;
	struct seglist *slist;
{
	struct seglist *sgfree;
#ifdef DEBUG
	vm_segment_t ts;
#endif

	sgfree = &vm_segment_list_free;
#ifdef DEBUG
	CIRCLEQ_FOREACH(ts, sgfree, segmentq) {
		if (ts == seg) {
			break;
		}
	}
	if (ts == NULL) {
		panic("vm_page_add_segmented_memory: segment not on freelist");
	}
#endif
	CIRCLEQ_REMOVE(sgfree, seg, segmentq);
	cnt.v_segment_free_count--;
	seg->flags = SEG_CLEAN;
	seg->object = NULL;
	seg->anon = NULL;
	CIRCLEQ_INSERT_TAIL(slist, seg, segmentq);
}

static int
vm_pagelist_alloc_memory(size, low, high, alignment, boundary, segmented, slist, rlist)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	bool_t segmented;
	struct seglist *slist;
	struct pglist  *rlist;
{
	vm_offset_t try, idxpa, lastidxpa;
	int s, tryidx, idx, end, error;
	vm_segment_t seg;
	vm_page_t pg;
	u_long mask;

	try = roundup(low, alignment);
	mask = ~(boundary - 1);

	/* Default to "lose". */
	error = ENOMEM;

	/*
	 * Block all memory allocation and lock the free list.
	 */
	s = splimp();
	pagelist_lock(segmented);

	/* Are there even any free pages / segments ? */
	if (vm_pagelist_first(segmented) == 0) {
		goto out;
	}

	for (;; try += alignment) {
		if (try + size > high) {
			/*
			 * We've run past the allowable range.
			 */
			goto out;
		}

		/*
		 * Make sure this is a managed physical page.
		 */
		if (vm_pagelist_is_physaddr(segmented, try) == 0) {
			continue;
		}

		tryidx = idx = vm_pagelist_index(segmented, try);
		end = idx + (size / PAGE_SIZE);
		if (end > vm_pagelist_array_size(segmented)) {
			/*
			 * No more physical memory.
			 */
			goto out;
		}

		/*
		 * Found a suitable starting page.  See of the range
		 * is free.
		 */
		for (idx = tryidx; idx < end; idx++) {
			if (vm_pagelist_is_free(segmented, idx) == 0) {
				/*
				 * Page not available.
				 */
				break;
			}

			idxpa = vm_pagelist_to_phys(segmented, idx);

			/*
			 * Make sure this is a managed physical page.
			 * XXX Necessary?  I guess only if there
			 * XXX are holes in the vm_page_array[].
			 */
			if (vm_pagelist_is_physaddr(segmented, idxpa) == 0) {
				break;
			}

			if (idx > tryidx) {
				lastidxpa = vm_pagelist_to_phys(segmented, idx - 1);
				if (segmented == TRUE) {
					if ((lastidxpa + SEGMENT_SIZE) != idxpa) {
						/*
						 * Region not contiguous.
						 */
						break;
					}
				} else {
					if ((lastidxpa + PAGE_SIZE) != idxpa) {
						/*
						 * Region not contiguous.
						 */
						break;
					}
				}
				if (boundary != 0 && ((lastidxpa ^ idxpa) & mask) != 0) {
					/*
					 * Region crosses boundary.
					 */
					break;
				}
			}
		}
		if (idx == end) {
			/*
			 * Woo hoo!  Found one.
			 */
			break;
		}
	}

	/*
	 * Okay, we have a chunk of memory that conforms to
	 * the requested constraints.
	 */
	idx = tryidx;
	error = vm_pagelist_found_chunk(idx, end, segmented, slist, rlist);

out:
	pagelist_unlock(segmented);
	splx(s);
	return (error);
}

/*
 * Allocates pagelist memory from segments not pages.
 * Will ignore memory size checks.
 */
/*
* NOTE: Care should be taken when using this function.
* - It is very likely to over-allocate on space if used inappropriately, or
*  without having memory checks in place.
*/
static int
vm_pagelist_alloc_segmented_memory(size, low, high, alignment, boundary, rlist)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	struct pglist *rlist;
{
	struct seglist tmp;

	CIRCLEQ_INIT(&tmp);
	return (vm_pagelist_alloc_memory(size, low, high, alignment, boundary, TRUE, &tmp, rlist));
}

static int
vm_pagelist_alloc_paged_memory(size, low, high, alignment, boundary, rlist)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	struct pglist *rlist;
{
	return (vm_pagelist_alloc_memory(size, low, high, alignment, boundary, FALSE, NULL, rlist));
}

static int
vm_pagelist_alloc_memory_contig(size, low, high, alignment, boundary, rlist)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	struct pglist *rlist;
{
	int s, error, rval;
	bool_t segmented;
	struct seglist tmp;

	/*
	 * Block all memory allocation and lock the free list.
	 */
	s = splimp();

	/* Check if number of pages expands beyond a single segment */

	if (vm_pagelist_check_memory(size)) {
		if (rval > 0) {
			segmented = TRUE;
		}
		if (rval < 0) {
			segmented = FALSE;
		}
	} else {
		segmented = FALSE;
	}
	if (segmented == TRUE) {
		CIRCLEQ_INIT(&tmp);
		error = vm_pagelist_alloc_memory(size, low, high, alignment, boundary, segmented, &tmp, rlist);
	} else {
		error = vm_pagelist_alloc_memory(size, low, high, alignment, boundary, segmented, NULL, rlist);
	}
	splx(s);
	return (error);
}

/*
 * vm_page_alloc_memory:
 * forced: (Below Assumes the parameters are met.)
 * 		- 0) will allocate in segments (4mb chunks).
 * 		- 1) will allocate according to size requested,
 * 			 this could be in pages (4kb chunks) or
 * 			 segments (4mb chunks). (Recommended)
 */
int
vm_page_alloc_memory(size, low, high, alignment, boundary, rlist, nsegs, waitok, forced)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	struct pglist *rlist;
	int nsegs, waitok, forced;
{
	int error;

#ifdef DIAGNOSTIC
	if ((alignment & (alignment - 1)) != 0) {
		panic("vm_page_alloc_memory: alignment must be power of 2");
	}

	if ((boundary & (boundary - 1)) != 0) {
		panic("vm_page_alloc_memory: boundary must be power of 2");
	}
#endif

	/*
	 * Our allocations are always page granularity, so our alignment
	 * must be, too.
	 */
	if (alignment < PAGE_SIZE) {
		alignment = PAGE_SIZE;
	}

	if (boundary != 0 && boundary < size) {
		return (EINVAL);
	}

	TAILQ_INIT(rlist);

	if ((nsegs < size >> PAGE_SHIFT) || (alignment != PAGE_SIZE)
			|| (boundary != 0)) {
		error = vm_pagelist_alloc_memory_contig(size, low, high, alignment,
				boundary, rlist);
	} else if (forced == 0) {
		error = vm_pagelist_alloc_segmented_memory(size, low, high, alignment,
				boundary, rlist);
	} else {
		error = vm_pagelist_alloc_paged_memory(size, low, high, alignment,
				boundary, rlist);
	}
	return (error);
}

/* Utility routines */
static int
vm_pagelist_found_chunk(idx, end, segmented, slist, rlist)
	int idx, end;
	bool_t segmented;
	struct seglist *slist;
	struct pglist  *rlist;
{
	vm_segment_t seg;
	vm_page_t pg;

	while (idx < end) {
		if (segmented == TRUE) {
			seg = &vm_segment_array[idx];
			vm_pagelist_add_segmented_memory(seg, slist);
			/* check if list of pages is empty */
			if (TAILQ_FIRST(&seg->memq) != NULL) {
				TAILQ_FOREACH(pg, &seg->memq, pageq) {
					if (pg->wire_count > 0) {
						/*
						 * Segment Contains Wired Down Pages.
						 */
						return (1);
					} else if ((pg->flags & PG_FREE) == 0) {
						/*
						 * Page not available.
						 */
						return (1);
					} else {
						vm_pagelist_add_paged_memory(pg, seg, rlist);
						STAT_INCR(vm_page_alloc_memory_npages);
					}
				}
			}
			idx++;
			STAT_INCR(vm_page_alloc_memory_nsegments);
		} else {
			pg = &vm_page_array[idx];
			vm_pagelist_add_paged_memory(pg, NULL, rlist);
			idx++;
			STAT_INCR(vm_page_alloc_memory_npages);
		}
	}
	return (0);
}

static int
vm_pagelist_check_memory(size)
    vm_size_t size;
{
    vm_offset_t nsegs, npgs;

    nsegs = num_segments(size);
	npgs = num_pages(size);
    if ((nsegs > 1) || (npgs > (SEGMENT_SIZE/PAGE_SIZE))) {
        return (1);
    } else if ((nsegs <= 1) || (npgs <= (SEGMENT_SIZE/PAGE_SIZE))) {
        return (-1);
    } else {
         return (0);
    }
}

static int
vm_pagelist_first(segmented)
	bool_t segmented;
{
	int first;

	if (segmented == TRUE) {
		first = (CIRCLEQ_FIRST(&vm_segment_list_free) == NULL);
	} else {
		first = (TAILQ_FIRST(&vm_page_queue_free) == NULL);
	}
	return (first);
}

static int
vm_pagelist_index(segmented, pa)
	bool_t segmented;
	vm_offset_t pa;
{
	int idx;

	if (segmented == TRUE) {
		idx = VM_SEGMENT_INDEX(pa);
	} else {
		idx = VM_PAGE_INDEX(pa);
	}
	return (idx);
}

static long
vm_pagelist_array_size(segmented)
	bool_t segmented;
{
	long array_size;

	if (segmented == TRUE) {
		array_size = vm_segment_array_size;
	} else {
		array_size = vm_page_array_size;
	}
	return (array_size);
}

static vm_offset_t
vm_pagelist_to_phys(segmented, idx)
	bool_t segmented;
	int idx;
{
	vm_offset_t idxpa;

	if (segmented == TRUE) {
		idxpa = VM_SEGMENT_TO_PHYS(&vm_segment_array[idx]);
	} else {
		idxpa = VM_PAGE_TO_PHYS(&vm_page_array[idx]);
	}
	return (idxpa);
}

static int
vm_pagelist_is_free(segmented, idx)
	bool_t segmented;
	int idx;
{
	int isfree;

	if (segmented == TRUE) {
		isfree = VM_SEGMENT_IS_FREE(&vm_segment_array[idx]);
	} else {
		isfree = VM_PAGE_IS_FREE(&vm_page_array[idx]);
	}
	return (isfree);
}

static int
vm_pagelist_is_physaddr(segmented, pa)
	bool_t segmented;
	vm_offset_t pa;
{
	int isphys;

	if (segmented == TRUE) {
		isphys = IS_VM_LOGICALADDR(pa);
	} else {
		isphys = IS_VM_PHYSADDR(pa);
	}
	return (isphys);
}
