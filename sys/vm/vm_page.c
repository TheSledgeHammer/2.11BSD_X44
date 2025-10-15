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

/*
 *	Resident memory management module.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fnv_hash.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>
#include <vm/include/vm_map.h>
#include <vm/include/vm_pageout.h>

/*
 *	Associated with page of user-allocatable memory is a
 *	page structure.
 */

struct pglist		*vm_page_buckets;			/* Array of buckets */
int					vm_page_bucket_count = 0;	/* How big is array? */
int					vm_page_hash_mask;			/* Mask for hash function */
simple_lock_data_t	bucket_lock;			/* lock for all page buckets XXX */

struct pglist		vm_page_queue_free;
struct pglist		vm_page_queue_active;
struct pglist		vm_page_queue_inactive;
simple_lock_data_t	vm_page_queue_lock;
simple_lock_data_t	vm_page_queue_free_lock;
simple_lock_data_t	vm_pages_needed_lock;

/* has physical page allocation been initialized? */
bool_t 				vm_page_startup_initialized;

vm_page_t			vm_page_array;
long				vm_page_array_size;
long				first_page;
long				last_page;
vm_offset_t			first_phys_addr;
vm_offset_t			last_phys_addr;
vm_size_t			page_mask;
int					page_shift;
vm_offset_t			kentry_data;

void i386_vm_page_startup(vm_page_t);

/*
 * vm_pmap_bootstrap:
 *
 * Allocates virtual address space from pmap_bootstrap_alloc.
 */
void
vm_pmap_bootstrap(void)
{
    extern vm_offset_t	kentry_data;
	vm_size_t			kentry_data_size;
	vm_size_t 			kmap_size, kentry_size;

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
	vm_size_t 			free;
	vm_size_t 			totsize;
	vm_size_t 			result;

	free = kentry_data;
	totsize = (size * nitems);
	result = free - totsize;
	if (free < totsize) {
		panic("vm_pmap_bootinit: not enough space allocated");
	}
	bzero(item, totsize);
	item = (item + size);
	kentry_data = result;

	return (item);
}

/*
 *	vm_set_page_size:
 *
 *	Sets the page size, perhaps based upon the memory
 *	size.  Must be called before any use of page-size
 *	dependent functions.
 *
 *	Sets page_shift and page_mask from cnt.v_page_size.
 */
void
vm_set_page_size(void)
 {
	if (cnt.v_page_size == 0) {
		cnt.v_page_size = DEFAULT_PAGE_SIZE;
	}
	page_mask = cnt.v_page_size - 1;
	if ((page_mask & cnt.v_page_size) != 0) {
		panic("vm_set_page_size: page size not a power of two");
	}
	for (page_shift = 0;; page_shift++) {
		if ((1 << page_shift) == cnt.v_page_size) {
			break;
		}
	}
}

/*
 *	vm_page_startup:
 *
 *	Initializes the resident memory module.
 *
 *	Allocates memory for the page cells, and
 *	for the object/offset-to-page hash table headers.
 *	Each page cell is initialized and placed on the free list.
 */
void
vm_page_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register vm_page_t		m;
	register struct pglist	*bucket;
	vm_size_t				npages;
	int						i;
	vm_offset_t				pa;

	/*
	 *	Initialize the locks
	 */

	simple_lock_init(&vm_page_queue_free_lock, "vm_page_queue_free_lock");
	simple_lock_init(&vm_page_queue_lock, "vm_page_queue_lock");

	/*
	 *	Initialize the queue headers for the free queue,
	 *	the active queue and the inactive queue.
	 */

	TAILQ_INIT(&vm_page_queue_free);
	TAILQ_INIT(&vm_page_queue_active);
	TAILQ_INIT(&vm_page_queue_inactive);

	/*
	 *	Calculate the number of hash table buckets.
	 *
	 *	The number of buckets MUST BE a power of 2, and
	 *	the actual value is the next power of 2 greater
	 *	than the number of physical pages in the system.
	 *
	 *	Note:
	 *		This computation can be tweaked if desired.
	 */

	if (vm_page_bucket_count == 0) {
		vm_page_bucket_count = 1;
		while (vm_page_bucket_count < atop(*end - *start)) {
			vm_page_bucket_count <<= 1;
		}
	}

	vm_page_hash_mask = vm_page_bucket_count - 1;

	/*
	 *	Allocate (and initialize) the hash table buckets.
	 */
	vm_page_buckets = (struct pglist *)pmap_bootstrap_alloc(vm_page_bucket_count * sizeof(struct pglist));
	bucket = vm_page_buckets;

	for (i = vm_page_bucket_count; i--;) {
		TAILQ_INIT(bucket);
		bucket++;
	}

	simple_lock_init(&bucket_lock, "page_bucket_lock");

	/*
	 *	Truncate the remainder of physical memory to our page size.
	 */

	*end = trunc_page(*end);

	/*
	 *	Pre-allocate maps and map entries that cannot be dynamically
	 *	allocated via malloc().  The maps include the kernel_map and
	 *	kmem_map which must be initialized before malloc() will
	 *	work (obviously).  Also could include pager maps which would
	 *	be allocated before kmeminit.
	 *
	 *	Allow some kernel map entries... this should be plenty
	 *	since people shouldn't be cluttering up the kernel
	 *	map (they should use their own maps).
	 */
	vm_pmap_bootstrap();

	/*
 	 *	Compute the number of pages of memory that will be
	 *	available for use (taking into account the overhead
	 *	of a page structure per page).
	 */

	cnt.v_page_free_count = npages = (*end - *start + sizeof(struct vm_page)) / (PAGE_SIZE + sizeof(struct vm_page));
    vm_page_array_size = npages;

	/*
	 *	Record the extent of physical memory that the
	 *	virtual memory system manages.
	 */

	first_page = *start;
	first_page += npages * sizeof(struct vm_page);
	first_page = atop(round_page(first_page));
	last_page  = first_page + npages - 1;

	first_phys_addr = ptoa(first_page);
	last_phys_addr  = ptoa(last_page) + PAGE_MASK;

	/*
	 *	Allocate and clear the mem entry structures.
	 */

	m = vm_page_array = (vm_page_t)pmap_bootstrap_alloc(npages * sizeof(struct vm_page));

	/*
	 *	Initialize the mem entry structures now, and
	 *	put them in the free queue.
	 */

	pa = first_phys_addr;
	while (npages--) {
		m->flags = 0;
		m->segment = NULL;
		m->phys_addr = pa;
#ifdef i386
		i386_vm_page_startup(m);
#else /* i386 */
		TAILQ_INSERT_TAIL(&vm_page_queue_free, m, pageq);
#endif /* i386 */
		m++;
		pa += PAGE_SIZE;
	}

	/*
	 *	Initialize vm_pages_needed lock here - don't wait for pageout
	 *	daemon	XXX
	 */
	simple_lock_init(&vm_pages_needed_lock, "vm_pages_needed_lock");

	/* from now on, pmap_bootstrap_alloc can't be used */
	vm_page_startup_initialized = TRUE;
}

void
i386_vm_page_startup(page)
	register vm_page_t		page;
{
	if (pmap_isvalidphys(page->phys_addr)) {
		TAILQ_INSERT_TAIL(&vm_page_queue_free, page, pageq);
	} else {
		/* perhaps iomem needs it's own type, or dev pager? */
		page->flags |= PG_FICTITIOUS | PG_BUSY;
		cnt.v_page_free_count--;
	}
}

/*
 *	vm_page_hash:
 *
 *	Distributes the segment/offset key pair among hash buckets.
 *
 *	NOTE:  This macro depends on vm_page_bucket_count being a power of 2.
 */
unsigned long
vm_page_hash(segment, offset)
    vm_segment_t    segment;
    vm_offset_t     offset;
{
    Fnv32_t hash1 = fnv_32_buf(&segment, (sizeof(&segment) + offset)&vm_page_hash_mask, FNV1_32_INIT)%vm_page_hash_mask;
    Fnv32_t hash2 = (((unsigned long)segment+(unsigned long)offset)&vm_page_hash_mask);
    return (hash1^hash2);
}

/*
 *	vm_page_insert:		[ internal use only ]
 *
 *	Inserts the given mem entry into the segment/segment-page
 *	table and object list.
 *
 *	The object, segment and page must be locked.
 */

void
vm_page_insert(mem, segment, offset)
	register vm_page_t		mem;
	register vm_segment_t	segment;
	register vm_offset_t	offset;
{
	register struct pglist	*bucket;
	int						spl;

	mem->segment = segment;
	mem->offset = offset;

	bucket = &vm_page_buckets[vm_page_hash(segment, offset)];
	spl = splimp();
	simple_lock(&bucket_lock);
	TAILQ_INSERT_TAIL(bucket, mem, hashq);
	simple_unlock(&bucket_lock);
	(void) splx(spl);

	TAILQ_INSERT_TAIL(&segment->memq, mem, listq);
	mem->flags |= PG_TABLED;

	segment->object->resident_page_count++;
}

/*
 *	vm_page_remove:		[ internal use only ]
 *				NOTE: used by device pager as well -wfj
 *
 *	Removes the given mem entry from the segment/offset-page
 *	table and the segment page list.
 *
 *	The object, segment and page must be locked.
 */

void
vm_page_remove(mem)
	register vm_page_t	mem;
{
	register struct pglist	*bucket;
	int			spl;

	VM_PAGE_CHECK(mem);

	if (!(mem->flags & PG_TABLED))
		return;

	/*
	 *	Remove from the object_segment/offset hash table
	 */

	bucket = &vm_page_buckets[vm_page_hash(mem->segment, mem->offset)];
	spl = splimp();
	simple_lock(&bucket_lock);
	TAILQ_REMOVE(bucket, mem, hashq);
	simple_unlock(&bucket_lock);
	(void) splx(spl);

	/*
	 *	Now remove from the segment's list of backed pages.
	 */

	TAILQ_REMOVE(&mem->segment->memq, mem, listq);

	/*
	 *	And show that the segment has one fewer resident
	 *	page.
	 */

	mem->segment->object->resident_page_count--;

	mem->flags &= ~PG_TABLED;
}

/*
 *	vm_page_lookup:
 *
 *	Returns the page associated with the segment/offset
 *	pair specified; if none is found, NULL is returned.
 *
 *	The object must be locked.  No side effects.
 */

vm_page_t
vm_page_lookup(segment, offset)
	register vm_segment_t	segment;
	register vm_offset_t	offset;
{
	register vm_page_t		mem;
	register struct pglist	*bucket;
	int						spl;

	/*
	 *	Search the hash table for this segment/offset pair
	 */

	bucket = &vm_page_buckets[vm_page_hash(segment, offset)];

	spl = splimp();
	simple_lock(&bucket_lock);
	TAILQ_FOREACH(mem, bucket, hashq) {
		VM_PAGE_CHECK(mem);
		if ((mem->segment == segment) && (mem->offset == offset)) {
			simple_unlock(&bucket_lock);
			splx(spl);
			return (mem);
		}
	}

	simple_unlock(&bucket_lock);
	splx(spl);
	return (NULL);
}

/*
 *	vm_page_rename:
 *
 *	Move the given memory entry from its
 *	current segment to the specified target segment/offset.
 *
 *	The segment must be locked.
 */
void
vm_page_rename(mem, new_segment, new_offset)
	register vm_page_t		mem;
	register vm_segment_t	new_segment;
	vm_offset_t				new_offset;
{
	if (mem->segment == new_segment) {
		return;
	}

	vm_page_lock_queues(); /* keep page from moving out from under pageout daemon */
	vm_page_remove(mem);
	vm_page_insert(mem, new_segment, new_offset);
	vm_page_unlock_queues();
}

/*
 *	vm_page_alloc:
 *
 *	Allocate and return a memory cell associated
 *	with this VM segment/offset pair.
 *
 *	Segment must be locked.
 */
vm_page_t
vm_page_alloc(segment, offset)
	vm_segment_t segment;
	vm_offset_t	offset;
{
	register vm_page_t	mem;
	int		spl;

	spl = splimp();				/* XXX */
	simple_lock(&vm_page_queue_free_lock);
	if (TAILQ_FIRST(&vm_page_queue_free) == NULL) {
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
		return(NULL);
	}

	mem = TAILQ_FIRST(&vm_page_queue_free);
	TAILQ_REMOVE(&vm_page_queue_free, mem, pageq);

	cnt.v_page_free_count--;
	simple_unlock(&vm_page_queue_free_lock);
	splx(spl);

	VM_PAGE_INIT(mem, segment, offset);

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the inactive
	 *	count is less than its target.
	 *
	 *	We don't have the counts locked ... if they change a little,
	 *	it doesn't really matter.
	 */

	if (cnt.v_page_free_count < cnt.v_page_free_min ||
	    (cnt.v_page_free_count < cnt.v_page_free_target &&
	     cnt.v_page_inactive_count < cnt.v_page_inactive_target)) {
		vm_thread_wakeup(&vm_pages_needed);
	}
	return (mem);
}

/*
 *	vm_page_free:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Segment and page must be locked prior to entry.
 */
void
vm_page_free(mem)
	register vm_page_t	mem;
{
	vm_page_remove(mem);
	if (mem->flags & PG_ACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_active, mem, pageq);
		mem->flags &= ~PG_ACTIVE;
		cnt.v_page_active_count--;
	}

	if (mem->flags & PG_INACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_inactive, mem, pageq);
		mem->flags &= ~PG_INACTIVE;
		cnt.v_page_inactive_count--;
	}

	if (!(mem->flags & PG_FICTITIOUS)) {
		int spl;

		spl = splimp();
		simple_lock(&vm_page_queue_free_lock);
		TAILQ_INSERT_TAIL(&vm_page_queue_free, mem, pageq);

		cnt.v_page_free_count++;
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
	}
}

/*
 *	vm_page_wire:
 *
 *	Mark this page as wired down by yet
 *	another map, removing it from paging queues
 *	as necessary.
 *
 *	The page queues must be locked.
 */
void
vm_page_wire(mem)
	register vm_page_t	mem;
{
	VM_PAGE_CHECK(mem);

	if (mem->wire_count == 0) {
		if (mem->flags & PG_ACTIVE) {
			TAILQ_REMOVE(&vm_page_queue_active, mem, pageq);
			cnt.v_page_active_count--;
			mem->flags &= ~PG_ACTIVE;
		}
		if (mem->flags & PG_INACTIVE) {
			TAILQ_REMOVE(&vm_page_queue_inactive, mem, pageq);
			cnt.v_page_inactive_count--;
			mem->flags &= ~PG_INACTIVE;
		}
		cnt.v_page_wire_count++;
	}
	mem->wire_count++;
}

/*
 *	vm_page_unwire:
 *
 *	Release one wiring of this page, potentially
 *	enabling it to be paged again.
 *
 *	The page queues must be locked.
 */
void
vm_page_unwire(mem)
	register vm_page_t	mem;
{
	VM_PAGE_CHECK(mem);

	mem->wire_count--;
	if (mem->wire_count == 0) {
		TAILQ_INSERT_TAIL(&vm_page_queue_active, mem, pageq);
		cnt.v_page_active_count++;
		mem->flags |= PG_ACTIVE;
		cnt.v_page_wire_count--;
	}
}

/*
 *	vm_page_deactivate:
 *
 *	Returns the given page to the inactive list,
 *	indicating that no physical maps have access
 *	to this page.  [Used by the physical mapping system.]
 *
 *	The page queues must be locked.
 */
void
vm_page_deactivate(m)
	register vm_page_t	m;
{
	VM_PAGE_CHECK(m);

	/*
	 *	Only move active pages -- ignore locked or already
	 *	inactive ones.
	 */
	if (m->flags & PG_ACTIVE) {
		pmap_clear_reference(VM_PAGE_TO_PHYS(m));
		TAILQ_REMOVE(&vm_page_queue_active, m, pageq);
		TAILQ_INSERT_TAIL(&vm_page_queue_inactive, m, pageq);
		m->flags &= ~PG_ACTIVE;
		m->flags |= PG_INACTIVE;
		cnt.v_page_active_count--;
		cnt.v_page_inactive_count++;
		if (pmap_is_modified(VM_PAGE_TO_PHYS(m))) {
			m->flags &= ~PG_CLEAN;
		}
		if (m->flags & PG_CLEAN) {
			m->flags &= ~PG_LAUNDRY;
		} else {
			m->flags |= PG_LAUNDRY;
		}
	}
}

/*
 *	vm_page_activate:
 *
 *	Put the specified page on the active list (if appropriate).
 *
 *	The page queues must be locked.
 */

void
vm_page_activate(m)
	register vm_page_t	m;
{
	VM_PAGE_CHECK(m);

	if (m->flags & PG_INACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_inactive, m, pageq);
		cnt.v_page_inactive_count--;
		m->flags &= ~PG_INACTIVE;
	}
	if (m->wire_count == 0) {
		if (m->flags & PG_ACTIVE) {
			panic("vm_page_activate: already active");
		}
		TAILQ_INSERT_TAIL(&vm_page_queue_active, m, pageq);
		m->flags |= PG_ACTIVE;
		cnt.v_page_active_count++;
	}
}

/*
 *	vm_page_zero_fill:
 *
 *	Zero-fill the specified page.
 *	Written as a standard pagein routine, to
 *	be used by the zero-fill object.
 */

bool_t
vm_page_zero_fill(m)
	vm_page_t	m;
{
	VM_PAGE_CHECK(m);

	m->flags &= ~PG_CLEAN;
	pmap_zero_page(VM_PAGE_TO_PHYS(m));
	return (TRUE);
}

/*
 *	vm_page_copy:
 *
 *	Copy one page to another
 */

void
vm_page_copy(src_m, dest_m)
	vm_page_t	src_m;
	vm_page_t	dest_m;
{
	VM_PAGE_CHECK(src_m);
	VM_PAGE_CHECK(dest_m);

	dest_m->flags &= ~PG_CLEAN;
	pmap_copy_page(VM_PAGE_TO_PHYS(src_m), VM_PAGE_TO_PHYS(dest_m));
}

/* page anon management */
vm_page_t
vm_page_anon_alloc(segment, offset, anon)
	vm_segment_t segment;
	vm_offset_t	offset;
	vm_anon_t anon;
{
	vm_page_t	mem;

	mem = vm_page_alloc(segment, offset);

	mem->segment = segment;
	mem->offset = offset;
	mem->anon = anon;
	mem->flags = PG_BUSY|PG_CLEAN|PG_FAKE;
	mem->wire_count = 0;
	if (anon) {
		anon->u.an_page = mem;
		mem->flags = PG_ANON;
	} else {
		if (segment) {
			vm_page_insert(mem, segment, offset);
		}
	}
	return (mem);
}

void
vm_page_anon_free(mem)
	vm_page_t mem;
{
	if (mem->flags & PG_TABLED) {
		vm_page_remove(mem);
	} else if (mem->flags & PG_ANON) {
		mem->flags &= ~PG_ANON;
		mem->anon = NULL;
		return;
	}
	vm_page_free(mem);
}
