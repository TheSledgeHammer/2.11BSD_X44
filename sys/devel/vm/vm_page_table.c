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
 */

#include <sys/user.h>
#include <sys/tree.h>
#include <sys/fnv_hash.h>
#include <sys/malloc.h>
#include <sys/map.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_page.h>
#include "devel/vm/include/vm_segment.h"

struct vm_page_table_hash_root 	vm_pagetable_buckets;
int								vm_pagetable_bucket_count = 0;	/* How big is array? */
int								vm_pagetable_hash_mask;			/* Mask for hash function */

long				pagetable_collapses = 0;
long				pagetable_bypasses  = 0;

extern vm_size_t	pagetable_mask;
extern int			pagetable_shift;

static void	_vm_pagetable_allocate(vm_size_t, vm_page_table_t);

void
vm_set_pagetable_size()
{
	if (cnt.v_pagetable_size == 0)
		cnt.v_pagetable_size = DEFAULT_PAGETABLE_SIZE;
	pagetable_mask = cnt.v_pagetable_size - 1;
	if ((pagetable_mask & cnt.v_pagetable_size) != 0)
		panic("vm_set_pagetable_size: pagetable size not a power of two");
	for (pagetable_shift = 0;; pagetable_shift++)
		if ((1 << pagetable_shift) == cnt.v_pagetable_size)
			break;
}

void
vm_pagetable_init(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register int i;

	if (vm_pagetable_bucket_count == 0) {
		vm_pagetable_bucket_count = 1;
		while (vm_pagetable_bucket_count < atop(*end - *start))
			vm_pagetable_bucket_count <<= 1;
	}

	vm_pagetable_hash_mask = vm_pagetable_bucket_count - 1;

	for(i = 0; i < vm_pagetable_hash_mask; i++) {
		RB_INIT(&vm_pagetable_buckets[i]);
	}
}

vm_page_table_t
vm_pagetable_allocate(size)
	vm_size_t	size;
{
	register vm_page_table_t result;

	result = (vm_page_table_t)malloc((u_long)sizeof(*result), M_VMPGTABLE, M_WAITOK);

	_vm_pagetable_allocate(size, result);

	return (result);
}

static void
_vm_pagetable_allocate(size, pagetable)
	vm_size_t					size;
	register vm_page_table_t 	pagetable;
{
	TAILQ_INIT(&pagetable->pt_pglist);
	vm_pagetable_lock_init(pagetable);
	pagetable->pt_ref_count = 1;
	pagetable->pt_resident_page_count = 0;
	pagetable->pt_size = size;
	pagetable->pt_flags;
	pagetable->pt_paging_in_progress = 0;


	pagetable->pt_paging_offset = 0;
	simple_lock(&vm_pagetable_tree_lock);
	RB_INSERT(pttree, &vm_pagetable_tree, pagetable);
	vm_pagetable_count++;
	simple_unlock(&vm_pagetable_tree_lock);
}

void
vm_pagetable_deallocate(pagetable)
	register vm_page_table_t	pagetable;
{
	vm_page_table_t temp;

	while (pagetable != NULL) {

		vm_pagetable_lock(pagetable);
		if(--(pagetable->pt_ref_count) != 0) {
			vm_pagetable_unlock(pagetable);
			return;
		}

		if (pagetable->pt_flags & OBJ_CANPERSIST) {
			/* cache insertion here */
			vm_pagetable_deactivate_pages(pagetable);
			vm_pagetable_lock(pagetable);

			vm_pagetable_cache_trim();
		}
		vm_pagetable_remove(pagetable);

		temp = pagetable->pt_shadow;
		vm_pagetable_terminate(pagetable);
			/* unlocks and deallocates object */
		pagetable = temp;
	}
}

/*
 *	vm_page_table_reference:
 *
 *	Gets another reference to the given page table.
 */
void
vm_pagetable_reference(pagetable)
	register vm_page_table_t	pagetable;
{
	if (pagetable == NULL)
		return;

	vm_pagetable_lock(pagetable);
	pagetable->pt_ref_count++;
	vm_pagetable_unlock(pagetable);
}

void
vm_pagetable_terminate(pagetable)
	register vm_page_table_t	pagetable;
{
	register vm_page_t			p;

}


boolean_t
vm_pagetable_page_clean(pagetable, start, end, syncio, de_queue)
	register vm_page_table_t	pagetable;
	register vm_offset_t		start;
	register vm_offset_t		end;
	boolean_t					syncio;
	boolean_t					de_queue;
{
	register vm_page_t p;
	register vm_pager_t pgr = vm_page_table_getpager(pagetable);
	int onqueue;
	boolean_t noerror = TRUE;

	if (pagetable == NULL)
		return (TRUE);

	return (noerror);
}

void
vm_pagetable_deactivate_pages(pagetable)
	register vm_page_table_t	pagetable;
{
	register vm_page_t	p, next;

	for (p = TAILQ_FIRST(pagetable->pt_pglist); p != NULL; p = next) {
		next = TAILQ_NEXT(p, listq);
		vm_page_lock_queues();
		vm_page_deactivate(p);
		vm_page_unlock_queues();
	}
}

void
vm_pagetable_pmap_copy(pagetable, start, end)
	register vm_page_table_t	pagetable;
	register vm_offset_t		start;
	register vm_offset_t		end;
{
	register vm_page_t	p;

	if (pagetable == NULL)
		return;

	vm_pagetable_lock(pagetable);
	for (p = TAILQ_FIRST(pagetable->pt_pglist); p != NULL; p = TAILQ_NEXT(p, listq)) {
		if ((start <= p->offset) && (p->offset < end)) {
			pmap_page_protect(VM_PAGE_TO_PHYS(p), VM_PROT_READ);
			p->flags |= PG_COPYONWRITE;
		}
	}
	vm_pagetable_unlock(pagetable);
}

void
vm_pagetable_pmap_remove(pagetable, start, end)
	register vm_page_table_t	pagetable;
	register vm_offset_t		start;
	register vm_offset_t		end;
{
	register vm_page_t			p;

	if(pagetable == NULL) {
		return;
	}

	vm_pagetable_lock(pagetable);
	for(p = TAILQ_FIRST(pagetable->pt_pglist); p != NULL; p = TAILQ_NEXT(p, listq)) {
		if ((start <= p->offset) && (p->offset < end)) {
			pmap_page_protect(VM_PAGE_TO_PHYS(p), VM_PROT_NONE);
		}
	}
	vm_pagetable_unlock(pagetable);
}

vm_pager_t
vm_pagetable_getpager(pagetable)
	vm_page_table_t	pagetable;
{
	register vm_segment_t segment;

	vm_pagetable_lock(pagetable);
	segment = pagetable->pt_segment;
	if (segment != NULL) {
		vm_pagetable_unlock(pagetable);
		return (vm_segment_getpager(segment));
	}
	vm_pagetable_unlock(pagetable);
	return (NULL);
}

/*
 *	Set the specified object's pager to the specified pager.
 */
void
vm_pagetable_setpager(pagetable, read_only)
	vm_page_table_t	pagetable;
	boolean_t		read_only;
{
	register vm_segment_t segment;

	vm_pagetable_lock(pagetable);
	segment = pagetable->pt_segment;
	if(segment != NULL) {
		vm_segment_setpager(segment, read_only);
	}
	vm_pagetable_unlock(pagetable);
}


/*
 *	vm_object_shadow:
 *
 *	Create a new object which is backed by the
 *	specified existing object range.  The source
 *	object reference is deallocated.
 *
 *	The new object and offset into that object
 *	are returned in the source parameters.
 */

void
vm_pagetable_shadow(pagetable, offset, length)
	vm_page_table_t	*pagetable;	/* IN/OUT */
	vm_offset_t		*offset;	/* IN/OUT */
	vm_size_t		length;
{
	register vm_page_table_t	source;
	register vm_page_table_t	result;

	source = *pagetable;

	/*
	 *	Allocate a new object with the given length
	 */

	if ((result = vm_pagetable_allocate(length)) == NULL)
		panic("vm_object_shadow: no object for shadowing");

	/*
	 *	The new object shadows the source object, adding
	 *	a reference to it.  Our caller changes his reference
	 *	to point to the new object, removing a reference to
	 *	the source object.  Net result: no change of reference
	 *	count.
	 */
	result->pt_shadow = source;

	/*
	 *	Store the offset into the source object,
	 *	and fix up the offset into the new object.
	 */

	result->pt_shadow_offset = *offset;

	/*
	 *	Return the new things
	 */

	*offset = 0;
	*pagetable = result;
}

unsigned long
vm_pagetable_hash(segment, offset)
    vm_segment_t    segment;
    vm_offset_t     offset;
{
    Fnv32_t hash1 = fnv_32_buf(&segment, (sizeof(&segment) + offset)&vm_pagetable_hash_mask, FNV1_32_INIT)%vm_pagetable_hash_mask;
    Fnv32_t hash2 = (((unsigned long)segment+(unsigned long)offset)&vm_pagetable_hash_mask);
    return (hash1^hash2);
}

int
vm_pagetable_rb_compare(pt1, pt2)
	vm_page_table_t pt1, pt2;
{
	if(pt1->pt_offset < pt2->pt_offset) {
		return (-1);
	} else if(pt1->pt_offset > pt2->pt_offset) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(pttree, vm_page_table, pt_tree, vm_pagetable_rb_compare);
RB_GENERATE(pttree, vm_page_table, pt_tree, vm_pagetable_rb_compare);
RB_PROTOTYPE(vm_page_table_hash_root, vm_page_table_hash_entry, pte_hlinks, vm_pagetable_rb_compare);
RB_GENERATE(vm_page_table_hash_root, vm_page_table_hash_entry, pte_hlinks, vm_pagetable_rb_compare);

/* insert page table into segment */
void
vm_pagetable_enter(pagetable, segment, offset)
	register vm_page_table_t 	pagetable;
	register vm_segment_t		segment;
	register vm_offset_t		offset;
{
	struct vm_page_table_hash_root *bucket;

	pagetable->pt_segment = segment;
	pagetable->pt_offset = offset;

	bucket = &vm_pagetable_buckets[vm_pagetable_hash(segment, offset)];

	RB_INSERT(vm_page_table_hash_root, bucket, pagetable);
}

/* lookup page table from segment/offset pair */
vm_page_table_t
vm_pagetable_lookup(segment, offset)
	register vm_segment_t	segment;
	register vm_offset_t	offset;
{
	struct vm_page_table_hash_root *bucket;
	register vm_page_table_t 		pagetable;

	bucket = &vm_pagetable_buckets[vm_pagetable_hash(segment, offset)];

	for (pagetable = RB_FIRST(vm_page_table_hash_root, bucket); pagetable != NULL; pagetable = RB_NEXT(vm_page_table_hash_root, bucket, pagetable)) {
		if ((RB_FIND(vm_page_table_hash_root, bucket, pagetable)->pte_pgtable == pagetable)) {
			pagetable = RB_FIND(vm_page_table_hash_root, bucket, pagetable)->pte_pgtable;
			if(pagetable->pt_segment == segment && pagetable->pt_offset == offset) {
				return (pagetable);
			}
		}
	}
	return (NULL);
}

/* remove page table from segment */
void
vm_pagetable_remove(pagetable)
	register vm_page_table_t pagetable;
{
	struct vm_page_table_hash_root *bucket = &vm_pagetable_buckets[vm_pagetable_hash(pagetable->pt_segment, pagetable->pt_offset)];

	if(bucket) {
		RB_REMOVE(vm_page_table_hash_root, bucket, pagetable);
	}
}

/*
 *	vm_object_cache_clear removes all objects from the cache.
 *
 */
void
vm_pagetable_cache_clear()
{
	register vm_page_table_t	pagetable;

	/*
	 *	Remove each object in the cache by scanning down the
	 *	list of cached objects.
	 */
	vm_pagetable_cache_lock();
	while ((pagetable = TAILQ_FIRST(vm_pagetable_cache_list)) != NULL) {
		vm_pagetable_cache_unlock();

		/*
		 * Note: it is important that we use vm_object_lookup
		 * to gain a reference, and not vm_object_reference, because
		 * the logic for removing an object from the cache lies in
		 * lookup.
		 */
		if (pagetable != vm_pagetable_lookup(pagetable->pt_segment, pagetable->pt_offset))
			panic("vm_pagetable_cache_clear: I'm sooo confused.");
		pager_cache(vm_pagetable_getpager(pagetable), FALSE);

		vm_pagetable_cache_lock();
	}
	vm_pagetable_cache_unlock();
}

boolean_t	vm_pagetable_collapse_allowed = TRUE;
/*
 *	vm_object_collapse:
 *
 *	Collapse an object with the object backing it.
 *	Pages in the backing object are moved into the
 *	parent, and the backing object is deallocated.
 *
 *	Requires that the object be locked and the page
 *	queues be unlocked.
 *
 */
void
vm_pagetable_collapse(pagetable)
	register vm_page_table_t	pagetable;
{
	register vm_page_table_t	backing_pagetable;
	register vm_offset_t		backing_offset;
	register vm_size_t			size;
	register vm_offset_t		new_offset;
	register vm_page_t			p, pp;

	if (!vm_pagetable_collapse_allowed)
			return;

	while (TRUE) {
		if (pagetable == NULL || pagetable->pt_paging_in_progress != 0|| pagetable->pager != NULL)
			return;

		/*
		 *		There is a backing object, and
		 */

		if ((backing_pagetable = pagetable->pt_shadow) == NULL)
			return;

		vm_pagetable_lock(backing_pagetable);
		/*
		 *	...
		 *		The backing object is not read_only,
		 *		and no pages in the backing object are
		 *		currently being paged out.
		 *		The backing object is internal.
		 */

		if ((backing_pagetable->pt_flags & OBJ_INTERNAL) == 0
				|| backing_pagetable->pt_paging_in_progress != 0) {
			vm_pagetable_unlock(backing_pagetable);
			return;
		}

		/*
		 *	The backing object can't be a copy-object:
		 *	the shadow_offset for the copy-object must stay
		 *	as 0.  Furthermore (for the 'we have all the
		 *	pages' case), if we bypass backing_object and
		 *	just shadow the next object in the chain, old
		 *	pages from that object would then have to be copied
		 *	BOTH into the (former) backing_object and into the
		 *	parent object.
		 */
		if (backing_pagetable->pt_shadow != NULL
				&& backing_pagetable->pt_shadow->pt_copy != NULL) {
			vm_pagetable_unlock(backing_pagetable);
			return;
		}

		/*
		 *	We know that we can either collapse the backing
		 *	object (if the parent is the only reference to
		 *	it) or (perhaps) remove the parent's reference
		 *	to it.
		 */

		backing_offset = pagetable->pt_shadow_offset;
		size = pagetable->pt_size;

		/*
		 *	If there is exactly one reference to the backing
		 *	object, we can collapse it into the parent.
		 */

		if (backing_pagetable->pt_ref_count == 1) {

			/*
			 *	We can collapse the backing object.
			 *
			 *	Move all in-memory pages from backing_object
			 *	to the parent.  Pages that have been paged out
			 *	will be overwritten by any of the parent's
			 *	pages that shadow them.
			 */

			while ((p = TAILQ_FIRST(backing_pagetable->pt_pglist)) != NULL) {
				new_offset = (p->offset - backing_offset);

				/*
				 *	If the parent has a page here, or if
				 *	this page falls outside the parent,
				 *	dispose of it.
				 *
				 *	Otherwise, move it as planned.
				 */

				if (p->offset < backing_offset || new_offset >= size) {
					vm_page_lock_queues();
					vm_page_free(p);
					vm_page_unlock_queues();
				} else {
					pp = vm_page_lookup(object, new_offset);
					if (pp != NULL && !(pp->flags & PG_FAKE)) {
						vm_page_lock_queues();
						vm_page_free(p);
						vm_page_unlock_queues();
					} else {
						if (pp) {
							/* may be someone waiting for it */
							PAGE_WAKEUP(pp);
							vm_page_lock_queues();
							vm_page_free(pp);
							vm_page_unlock_queues();
						}
						vm_page_rename(p, object, new_offset);
					}
				}
			}

			/*
			 *	Move the pager from backing_object to object.
			 *
			 *	XXX We're only using part of the paging space
			 *	for keeps now... we ought to discard the
			 *	unused portion.
			 */

			if (backing_pagetable->pager) {
				pagetable->pager = backing_pagetable->pager;
				pagetable->pt_paging_offset = backing_offset
						+ backing_pagetable->pt_paging_offset;
				backing_pagetable->pager = NULL;
			}

			/*
			 *	Object now shadows whatever backing_object did.
			 *	Note that the reference to backing_object->shadow
			 *	moves from within backing_object to within object.
			 */

			pagetable->pt_shadow = backing_pagetable->pt_shadow;
			pagetable->pt_shadow_offset += backing_pagetable->pt_shadow_offset;
			if (pagetable->pt_shadow != NULL && pagetable->pt_shadow->pt_copy != NULL) {
				panic("vm_pagetable_collapse: we collapsed a copy-object!");
			}
			/*
			 *	Discard backing_object.
			 *
			 *	Since the backing object has no pages, no
			 *	pager left, and no object references within it,
			 *	all that is necessary is to dispose of it.
			 */

			vm_pagetable_unlock(backing_pagetable);

			simple_lock(&vm_pagetable_tree_lock);
			RB_REMOVE(pttree, &vm_pagetable_tree_lock, backing_pagetable);
			vm_pagetable_count--;
			simple_unlock(&vm_pagetable_tree_lock);

			free((caddr_t) backing_pagetable, M_VMOBJ);

			pagetable_collapses++;
		} else {
			/*
			 *	If all of the pages in the backing object are
			 *	shadowed by the parent object, the parent
			 *	object no longer has to shadow the backing
			 *	object; it can shadow the next one in the
			 *	chain.
			 *
			 *	The backing object must not be paged out - we'd
			 *	have to check all of the paged-out pages, as
			 *	well.
			 */

			if (backing_pagetable->pager != NULL) {
				vm_pagetable_unlock(backing_pagetable);
				return;
			}

			/*
			 *	Should have a check for a 'small' number
			 *	of pages here.
			 */

			for (p = TAILQ_FIRST(backing_pagetable->pt_pglist); p != NULL;
					p = p->listq.tqe_next) {
				new_offset = (p->offset - backing_offset);

				/*
				 *	If the parent has a page here, or if
				 *	this page falls outside the parent,
				 *	keep going.
				 *
				 *	Otherwise, the backing_object must be
				 *	left in the chain.
				 */

				if (p->offset >= backing_offset && new_offset < size
						&& ((pp = vm_page_lookup(object, new_offset)) == NULL
								|| (pp->flags & PG_FAKE))) {
					/*
					 *	Page still needed.
					 *	Can't go any further.
					 */
					vm_pagetable_unlock(backing_pagetable);
					return;
				}
			}

			/*
			 *	Make the parent shadow the next object
			 *	in the chain.  Deallocating backing_object
			 *	will not remove it, since its reference
			 *	count is at least 2.
			 */

			pagetable->pt_shadow = backing_pagetable->pt_shadow;
			vm_object_reference(pagetable->pt_shadow);
			pagetable->pt_shadow_offset += backing_pagetable->pt_shadow_offset;

			/*
			 *	Backing object might have had a copy pointer
			 *	to us.  If it did, clear it.
			 */
			if (backing_pagetable->pt_copy == pagetable) {
				backing_pagetable->pt_copy = NULL;
			}

			/*	Drop the reference count on backing_object.
			 *	Since its ref_count was at least 2, it
			 *	will not vanish; so we don't need to call
			 *	vm_object_deallocate.
			 */
			backing_pagetable->pt_ref_count--;
			vm_pagetable_unlock(backing_pagetable);

			object_bypasses++;

		}

		/*
		 *	Try again with this object's new backing object.
		 */
	}
}

/*
 *	vm_pagetable_page_remove: [internal]
 *
 *	Removes all physical pages in the specified
 *	object range from the object's list of pages.
 *
 *	The object must be locked.
 */
void
vm_pagetable_page_remove(pagetable, start, end)
	register vm_page_table_t	pagetable;
	register vm_offset_t		start;
	register vm_offset_t		end;
{
	register vm_page_t	p, next;

	if (pagetable == NULL)
		return;

	for (p = TAILQ_FIRST(pagetable->pt_pglist); p != NULL; p = next) {
		next = TAILQ_NEXT(p, listq);
		if ((start <= p->offset) && (p->offset < end)) {
			pmap_page_protect(VM_PAGE_TO_PHYS(p), VM_PROT_NONE);
			vm_page_lock_queues();
			vm_page_free(p);
			vm_page_unlock_queues();
		}
	}
}

/*
 *	Routine:	vm_pagetable_coalesce
 *	Function:	Coalesces two objects backing up adjoining
 *			regions of memory into a single object.
 *
 *	returns TRUE if objects were combined.
 *
 *	NOTE:	Only works at the moment if the second object is NULL -
 *		if it's not, which object do we lock first?
 *
 *	Parameters:
 *		prev_object	First object to coalesce
 *		prev_offset	Offset into prev_object
 *		next_object	Second object into coalesce
 *		next_offset	Offset into next_object
 *
 *		prev_size	Size of reference to prev_object
 *		next_size	Size of reference to next_object
 *
 *	Conditions:
 *	The object must *not* be locked.
 */
boolean_t
vm_pagetable_coalesce(prev_pagetable, next_pagetable, prev_offset, next_offset, prev_size, next_size)
	register vm_page_table_t	prev_pagetable;
	vm_page_table_t				next_pagetable;
	vm_offset_t					prev_offset, next_offset;
	vm_size_t					prev_size, next_size;
{
	vm_size_t	newsize;

#ifdef	lint
	next_offset++;
#endif

	if (next_pagetable != NULL) {
		return (FALSE);
	}

	if (prev_pagetable == NULL) {
		return (TRUE);
	}

	vm_pagetable_lock(prev_pagetable);

	/*
	 *	Try to collapse the object first
	 */
	vm_pagetable_collapse(prev_pagetable);

	/*
	 *	Can't coalesce if:
	 *	. more than one reference
	 *	. paged out
	 *	. shadows another object
	 *	. has a copy elsewhere
	 *	(any of which mean that the pages not mapped to
	 *	prev_entry may be in use anyway)
	 */

	if (prev_pagetable->pt_ref_count > 1||
	vm_pagetable_getpager(prev_pagetable) != NULL ||
	prev_pagetable->pt_shadow != NULL ||
	prev_pagetable->pt_copy != NULL) {
		vm_pagetable_unlock(prev_pagetable);
		return (FALSE);
	}

	/*
	 *	Remove any pages that may still be in the object from
	 *	a previous deallocation.
	 */

	vm_pagetable_page_remove(prev_pagetable, prev_offset + prev_size, prev_offset + prev_size + next_size);

	/*
	 *	Extend the object if necessary.
	 */
	newsize = prev_offset + prev_size + next_size;
	if (newsize > prev_pagetable->pt_size)
		prev_pagetable->pt_size = newsize;

	vm_pagetable_unlock(prev_pagetable);
	return (TRUE);
}
