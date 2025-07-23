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
#else
#define	STAT_INCR(v)
#define	STAT_DECR(v)
#endif

static void vm_page_add_memory(vm_page_t, struct pglist *);
static void vm_page_add_segmented_memory(vm_segment_t, struct seglist *);
static int vm_pagelist_alloc_memory(vm_size_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_offset_t, struct pglist *);
static int vm_pagelist_alloc_memory_contig(vm_size_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_offset_t, struct pglist *);

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
vm_page_add_memory(m, seg, rlist)
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
vm_page_add_segmented_memory(seg, slist)
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
vm_pagelist_alloc_memory(size, low, high, alignment, boundary, rlist)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	struct pglist *rlist;
{
	vm_offset_t try, idxpa, lastidxpa;
	int s, tryidx, idx, end, error;
	vm_page_t m;
	u_long pagemask;

	size = round_page(size);
	try = roundup(low, alignment);

	pagemask = ~(boundary - 1);

	/* Default to "lose". */
	error = ENOMEM;

	/*
	 * Block all memory allocation and lock the free list.
	 */
	s = splimp();
	simple_lock(&vm_page_queue_free_lock);

	/* Are there even any free pages? */
	if (TAILQ_FIRST(&vm_page_queue_free) == NULL) {
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
		if (IS_VM_PHYSADDR(try) == 0) {
			continue;
		}

		tryidx = idx = VM_PAGE_INDEX(try);
		end = idx + (size / PAGE_SIZE);
		if (end > vm_page_array_size) {
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
			if (VM_PAGE_IS_FREE(&vm_page_array[idx]) == 0) {
				/*
				 * Page not available.
				 */
				break;
			}

			idxpa = VM_PAGE_TO_PHYS(&vm_page_array[idx]);

			/*
			 * Make sure this is a managed physical page.
			 * XXX Necessary?  I guess only if there
			 * XXX are holes in the vm_page_array[].
			 */
			if (IS_VM_PHYSADDR(idxpa) == 0) {
				break;
			}

			if (idx > tryidx) {
				lastidxpa = VM_PAGE_TO_PHYS(&vm_page_array[idx - 1]);

				if ((lastidxpa + PAGE_SIZE) != idxpa) {
					/*
					 * Region not contiguous.
					 */
					break;
				}
				if (boundary != 0 && ((lastidxpa ^ idxpa) & pagemask) != 0) {
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
	while (idx < end) {
		m = &vm_page_array[idx];
		vm_page_add_memory(m, NULL, rlist);
		idx++;
		STAT_INCR(vm_page_alloc_memory_npages);
	}
	error = 0;

out:
	simple_unlock(&vm_page_queue_free_lock);
	splx(s);
	return (error);
}

static int
vm_pagelist_alloc_segmented_memory(size, low, high, alignment, boundary, slist, rlist)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	struct seglist *slist;
	struct pglist *rlist;
{
	vm_offset_t try, idxpa, lastidxpa;
	int s, tryidx, idx, end, error;
	vm_segment_t seg;
	vm_page_t pg;
	u_long segmentmask;

	try = roundup(low, alignment);

	segmentmask = ~(boundary - 1);

	/* Default to "lose". */
	error = ENOMEM;

	/*
	 * Block all memory allocation and lock the free list.
	 */
	s = splimp();
	simple_lock(&vm_segment_list_free_lock);

	/* Are there even any free pages? */
	if (CIRCLEQ_FIRST(&vm_segment_list_free) == NULL) {
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
		if (IS_VM_LOGICALADDR(try) == 0) {
			continue;
		}

		tryidx = idx = VM_SEGMENT_INDEX(try);
		end = idx + (size / SEGMENT_SIZE);
		if (end > vm_segment_array_size) {
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
			if (VM_SEGMENT_IS_FREE(&vm_segment_array[idx]) == 0) {
				/*
				 * Page not available.
				 */
				break;
			}

			idxpa = VM_SEGMENT_TO_PHYS(&vm_segment_array[idx]);

			/*
			 * Make sure this is a managed physical page.
			 * XXX Necessary?  I guess only if there
			 * XXX are holes in the vm_page_array[].
			 */
			if (IS_VM_LOGICALADDR(idxpa) == 0) {
				break;
			}

			if (idx > tryidx) {
				lastidxpa = VM_SEGMENT_TO_PHYS(&vm_segment_array[idx - 1]);

				if ((lastidxpa + SEGMENT_SIZE) != idxpa) {
					/*
					 * Region not contiguous.
					 */
					break;
				}
				if (boundary != 0 && ((lastidxpa ^ idxpa) & segmentmask) != 0) {
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

	idx = tryidx;
	while (idx < end) {
		seg = &vm_segment_array[idx];
		vm_page_add_segmented_memory(seg, slist);
		/* page free lock ?? */
		if (TAILQ_FIRST(&seg->memq) == NULL) {
			goto out;
		}
		TAILQ_FOREACH(pg, &seg->memq, pageq) {
			if (pg->segment == seg) {
				/* TODO: check page flags */
				if (pg->wire_count > 0) {
					/*
					 * Segment Contains Wired Down Pages.
					 */
					goto out;
				}
				/* Finally add memory */
				vm_page_add_memory(pg, seg, rlist);
				STAT_INCR(vm_page_alloc_memory_npages);
			}
		}
		idx++;
	}
	error = 0;

out:
	simple_unlock(&vm_segment_list_free_lock);
	splx(s);
	return (error);
}

static int
vm_pagelist_alloc_memory_contig(size, low, high, alignment, boundary, rlist)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	struct pglist *rlist;
{
	int s, error;
	vm_offset_t nsegs, npgs;

	nsegs = num_segments(size);
	npgs = num_pages(size);

	/*
	 * Block all memory allocation and lock the free list.
	 */
	s = splimp();

	/* Check if number of pages expands beyond a single segment */
	if ((nsegs > 1) || (npgs > (SEGMENT_SIZE/PAGE_SIZE))) {
		struct seglist tmp;

		CIRCLEQ_INIT(&tmp);
		error = vm_pagelist_alloc_segmented_memory(size, low, high, alignment, boundary, &tmp, rlist);
	} else {
		error = vm_pagelist_alloc_memory(size, low, high, alignment, boundary, rlist);
	}
	splx(s);
	return (error);
}

int
vm_page_alloc_memory(size, low, high, alignment, boundary, rlist, nsegs, waitok)
	vm_size_t size;
	vm_offset_t low, high, alignment, boundary;
	struct pglist *rlist;
	int nsegs, waitok;
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
	} else {
		error = vm_pagelist_alloc_memory(size, low, high, alignment, boundary,
				rlist);
	}
	return (error);
}
