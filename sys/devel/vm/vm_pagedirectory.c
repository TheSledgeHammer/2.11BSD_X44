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

struct vm_pagedirectory_hash_root 	vm_pagedirectory_buckets;
int									vm_pagedirectory_bucket_count = 0;	/* How big is array? */
int									vm_pagedirectory_hash_mask;			/* Mask for hash function */

long								pagedirectory_collapses = 0;
long								pagedirectory_bypasses  = 0;

extern vm_size_t					pagedirectory_mask;
extern int							pagedirectory_shift;

static void	_vm_pagedirectory_allocate(vm_size_t, vm_pagedirectory_t);

void
vm_set_pagedirectory_size()
{
	if (cnt.v_pagedirectory_size == 0)
		cnt.v_pagedirectory_size = DEFAULT_PAGEDIRECTORY_SIZE;
	pagedirectory_mask = cnt.v_pagedirectory_size - 1;
	if ((pagedirectory_mask & cnt.v_pagedirectory_size) != 0)
		panic("vm_set_pagedirectory_size: pagedirectory size not a power of two");
	for (pagedirectory_shift = 0;; pagedirectory_shift++)
		if ((1 << pagedirectory_shift) == cnt.v_pagedirectory_size)
			break;
}

void
vm_pagedirectory_init(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register int i;

	if (vm_pagedirectory_bucket_count == 0) {
		vm_pagedirectory_bucket_count = 1;
		while (vm_pagedirectory_bucket_count < atop(*end - *start))
			vm_pagedirectory_bucket_count <<= 1;
	}

	vm_pagedirectory_hash_mask = vm_pagedirectory_bucket_count - 1;

	for(i = 0; i < vm_pagedirectory_hash_mask; i++) {
		RB_INIT(&vm_pagedirectory_buckets[i]);
	}
}

vm_pagedirectory_t
vm_pagedirectory_allocate(size)
	vm_size_t	size;
{
	register vm_pagedirectory_t result;

	result = (vm_pagedirectory_t)malloc((u_long)sizeof(*result), M_VMPDTABLE, M_WAITOK);

	_vm_pagedirectory_allocate(size, result);

	return (result);
}

static void
_vm_pagedirectory_allocate(size, pagedirectory)
	vm_size_t						size;
	register vm_pagedirectory_t 	pagedirectory;
{
	TAILQ_INIT(&pagedirectory->pd_pglist);
	vm_pagetable_lock_init(pagedirectory);
	pagedirectory->pd_ref_count = 1;
	pagedirectory->pd_resident_page_count = 0;
	pagedirectory->pd_size = size;
	pagedirectory->pd_flags;
	pagedirectory->pd_paging_in_progress = 0;


	pagedirectory->pd_paging_offset = 0;
	simple_lock(&vm_pagedirectory_tree_lock);
	RB_INSERT(pttree, &vm_pagedirectory_tree, pagedirectory);
	vm_pagedirectory_count++;
	simple_unlock(&vm_pagedirectory_tree_lock);
}

void
vm_pagedirectory_deallocate(pagedirectory)
	register vm_pagedirectory_t	pagedirectory;
{
	vm_pagedirectory_t temp;

	while (pagedirectory != NULL) {

		vm_pagedirectory_lock(pagedirectory);
		if(--(pagedirectory->pd_ref_count) != 0) {
			vm_pagedirectory_unlock(pagedirectory);
			return;
		}

		if (pagedirectory->pd_flags & OBJ_CANPERSIST) {
			/* cache insertion here */
			vm_pagedirectory_deactivate_pages(pagedirectory);
			vm_pagedirectory_lock(pagedirectory);

			vm_pagedirectory_cache_trim();
		}
		vm_pagedirectory_remove(pagedirectory);

		temp = pagedirectory->pd_shadow;
		vm_pagedirectory_terminate(pagedirectory);
			/* unlocks and deallocates object */
		pagedirectory = temp;
	}
}

/*
 *	vm_page_table_reference:
 *
 *	Gets another reference to the given page table.
 */
void
vm_pagedirectory_reference(pagedirectory)
	register vm_pagedirectory_t	pagedirectory;
{
	if (pagedirectory == NULL)
		return;

	vm_pagedirectory_lock(pagedirectory);
	pagedirectory->pd_ref_count++;
	vm_pagedirectory_unlock(pagedirectory);
}

boolean_t
vm_pagedirectory_page_clean(pagedirectory, start, end, syncio, de_queue)
	register vm_pagedirectory_t	pagedirectory;
	register vm_offset_t		start;
	register vm_offset_t		end;
	boolean_t					syncio;
	boolean_t					de_queue;
{
	register vm_page_t p;
	int onqueue;
	boolean_t noerror = TRUE;

	if (pagedirectory == NULL)
		return (TRUE);

	return (noerror);
}

void
vm_pagedirectory_deactivate_pages(pagedirectory)
	register vm_pagedirectory_t	pagedirectory;
{
	register vm_page_t	p, next;

	for (p = TAILQ_FIRST(pagedirectory->pd_pglist); p != NULL; p = next) {
		next = TAILQ_NEXT(p, listq);
		vm_page_lock_queues();
		vm_page_deactivate(p);
		vm_page_unlock_queues();
	}
}

/*
 *	Set the specified object's pager to the specified pager.
 */
void
vm_pagedirectory_setpager(pagedirectory, read_only)
	vm_pagedirectory_t	pagedirectory;
	boolean_t			read_only;
{
	register vm_segment_t segment;

	vm_pagedirectory_lock(pagedirectory);
	segment = pagedirectory->pd_segment;
	if(segment != NULL) {
		vm_segment_setpager(segment, read_only);
	}
	vm_pagedirectory_unlock(pagedirectory);
}

unsigned long
vm_pagedirectory_hash(segment, offset)
    vm_segment_t    segment;
    vm_offset_t     offset;
{
    Fnv32_t hash1 = fnv_32_buf(&segment, (sizeof(&segment) + offset)&vm_pagedirectory_hash_mask, FNV1_32_INIT)%vm_pagedirectory_hash_mask;
    Fnv32_t hash2 = (((unsigned long)segment+(unsigned long)offset)&vm_pagedirectory_hash_mask);
    return (hash1^hash2);
}

int
vm_pagedirectory_rb_compare(pt1, pt2)
	vm_pagedirectory_t pt1, pt2;
{
	if(pt1->pd_offset < pt2->pd_offset) {
		return (-1);
	} else if(pt1->pd_offset > pt2->pd_offset) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(pdtree, vm_pagedirectory, pd_tree, vm_pagedirectory_rb_compare);
RB_GENERATE(pdtree, vm_pagedirectory, pd_tree, vm_pagedirectory_rb_compare);
RB_PROTOTYPE(vm_pagedirectory_hash_root, vm_pagedirectory_hash_entry, pde_hlinks, vm_pagedirectory_rb_compare);
RB_GENERATE(vm_pagedirectory_hash_root, vm_pagedirectory_hash_entry, pde_hlinks, vm_pagedirectory_rb_compare);

/* insert page directory into segment */
void
vm_pagedirectory_enter(pagedirectory, segment, offset)
	register vm_pagedirectory_t 	pagedirectory;
	register vm_segment_t			segment;
	register vm_offset_t			offset;
{
	struct vm_pagedirectory_hash_root *bucket;

	pagedirectory->pd_segment = segment;
	pagedirectory->pd_offset = offset;

	bucket = &vm_pagedirectory_buckets[vm_pagedirectory_hash(segment, offset)];

	RB_INSERT(vm_pagedirectory_hash_root, bucket, pagedirectory);
}

/* lookup page table from segment/offset pair */
vm_pagedirectory_t
vm_pagedirectory_lookup(segment, offset)
	register vm_segment_t	segment;
	register vm_offset_t	offset;
{
	struct vm_pagedirectory_hash_root *bucket;
	register vm_pagedirectory_t 		pagedirectory;

	bucket = &vm_pagedirectory_buckets[vm_pagedirectory_hash(segment, offset)];

	for (pagedirectory = RB_FIRST(vm_pagedirectory_hash_root, bucket); pagedirectory != NULL; pagedirectory = RB_NEXT(vm_pagedirectory_hash_root, bucket, pagedirectory)) {
		if ((RB_FIND(vm_pagedirectory_hash_root, bucket, pagedirectory)->pde_pdtable == pagedirectory)) {
			pagedirectory = RB_FIND(vm_pagedirectory_hash_root, bucket, pagedirectory)->pde_pdtable;
			if(pagedirectory->pd_segment == segment && pagedirectory->pd_offset == offset) {
				return (pagedirectory);
			}
		}
	}
	return (NULL);
}

/* remove page table from segment */
void
vm_pagedirectory_remove(pagedirectory)
	register vm_pagedirectory_t pagedirectory;
{
	struct vm_pagedirectory_hash_root *bucket = &vm_pagedirectory_buckets[vm_pagedirectory_hash(pagedirectory->pd_segment, pagedirectory->pd_offset)];

	if(bucket) {
		RB_REMOVE(vm_pagedirectory_hash_root, bucket, pagedirectory);
	}
}

/*
 *	vm_object_cache_clear removes all objects from the cache.
 *
 */
void
vm_pagedirectory_cache_clear()
{
	register vm_pagedirectory_t	pagedirectory;

	/*
	 *	Remove each object in the cache by scanning down the
	 *	list of cached objects.
	 */
	vm_pagedirectory_cache_lock();
	while ((pagedirectory = TAILQ_FIRST(vm_pagedirectory_cache_list)) != NULL) {
		vm_pagedirectory_cache_unlock();

		/*
		 * Note: it is important that we use vm_object_lookup
		 * to gain a reference, and not vm_object_reference, because
		 * the logic for removing an object from the cache lies in
		 * lookup.
		 */
		if (pagedirectory != vm_pagedirectory_lookup(pagedirectory->pd_segment, pagedirectory->pd_offset))
			panic("vm_pagedirectory_cache_clear: I'm sooo confused.");
		pager_cache(vm_pagedirectory_getpager(pagedirectory), FALSE);

		vm_pagedirectory_cache_lock();
	}
	vm_pagedirectory_cache_unlock();
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
vm_pagedirectory_page_remove(pagedirectory, start, end)
	register vm_pagedirectory_t	pagedirectory;
	register vm_offset_t		start;
	register vm_offset_t		end;
{
	register vm_page_t	p, next;

	if (pagedirectory == NULL)
		return;

	for (p = TAILQ_FIRST(pagedirectory->pd_pglist); p != NULL; p = next) {
		next = TAILQ_NEXT(p, listq);
		if ((start <= p->offset) && (p->offset < end)) {
			pmap_page_protect(VM_PAGE_TO_PHYS(p), VM_PROT_NONE);
			vm_page_lock_queues();
			vm_page_free(p);
			vm_page_unlock_queues();
		}
	}
}
