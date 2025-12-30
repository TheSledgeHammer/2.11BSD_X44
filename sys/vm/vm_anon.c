/*	$NetBSD: uvm_anon.c,v 1.2 1999/03/26 17:34:15 chs Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * uvm_anon.c: uvm anon ops
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vmmeter.h>

#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>
#include <vm/include/vm_kern.h>

#include <vm/include/vm.h>
#include <vm/include/vm_swap.h>
#include <vm/include/vm_fault.h>

vm_anon_t vm_anon_allocate(int);
static void vm_anon_release_segment(vm_anon_t, vm_segment_t);
static void vm_anon_release_page(vm_anon_t, vm_segment_t, vm_page_t);
static void vm_anon_free_segment(vm_anon_t, vm_segment_t);
static void vm_anon_free_page(vm_anon_t, vm_segment_t, vm_page_t);

/*
 * initialize anons
 */
void
vm_anon_init(void)
{
	vm_anon_t anon;
	int nanon;
	int lcv;

	nanon = cnt.v_page_free_count - (cnt.v_page_free_count / 16); /* XXXCDC ??? */
	simple_lock_init(&anon->u.an_freelock, "anon_freelock");

	/*
	 * Allocate the initial anons.
	 */
	anon = vm_anon_allocate(nanon);

	bzero(anon, sizeof(*anon) * nanon);
	cnt.v_nanon = cnt.v_nfreeanon = nanon;

	anon->u.an_free = NULL;
	for (lcv = 0 ; lcv < nanon ; lcv++) {
		anon[lcv].u.an_nxt = anon->u.an_free;
		anon->u.an_free = &anon[lcv];
	}
}

/*
 * allocate anons
 */
vm_anon_t
vm_anon_allocate(nanon)
	int nanon;
{
	register vm_anon_t result;
	vm_size_t totsize;

	totsize = sizeof(*result) * nanon;
	result = (vm_anon_t)kmem_alloc(kernel_map, totsize);

	return (result);
}

/*
 * add some more anons to the free pool.  called when we add
 * more swap space.
 */
void
vm_anon_add(pages)
	int	pages;
{
	vm_anon_t anon;
	int lcv;

	anon = vm_anon_allocate(pages);

	simple_lock(&anon->u.an_freelock);
	bzero(anon, sizeof(*anon) * pages);
	cnt.v_nanon += pages;
	cnt.v_nfreeanon += pages;
	for (lcv = 0; lcv < pages; lcv++) {
		simple_lock_init(&anon->an_lock, "anon_lock");
		anon[lcv].u.an_nxt = anon->u.an_free;
		anon->u.an_free = &anon[lcv];
	}
	simple_unlock(&anon->u.an_freelock);
}

/*
 * allocate an anon
 */
vm_anon_t
vm_anon_alloc(void)
{
	vm_anon_t anon;

	simple_lock(&anon->u.an_freelock);
	anon = anon->u.an_free;
	if (anon) {
		anon->u.an_free = anon->u.an_nxt;
		cnt.v_nfreeanon--;
		anon->an_ref = 1;
		anon->u.an_segment = NULL;
		anon->u.an_page = NULL;
		anon->an_swslot = 0;
	}
	simple_unlock(&anon->u.an_freelock);

	return (anon);
}

/*
 * uvm_anfree: free a single anon structure
 *
 * => caller must remove anon from its amap before calling (if it was in
 *	an amap).
 * => anon must be unlocked and have a zero reference count.
 * => we may lock the pageq's.
 */
void
vm_anon_free(anon)
	vm_anon_t 		anon;
{
	vm_segment_t 	segment;
	vm_page_t 		page;

	KASSERT(anon->an_ref == 0);
	/*
	 * get segment & page
	 */
	segment = anon->u.an_segment;
	page = anon->u.an_page;

	/*
	 * if we have a resident page, we must dispose of it before freeing
	 * the anon.
	 */
	if (segment) {
		 if (page) {
				/*
				 * if the page is busy, mark it as PG_RELEASED
				 * so that uvm_anon_release will release it later.
				 */
			 vm_anon_free_page(anon, segment, page);
		 } else {
			 vm_anon_free_segment(anon, segment);
		 }
	}
	if (anon->an_swslot != 0) {
		/* this page is no longer only in swap. */
		simple_lock(&swap_data_lock);
		KASSERT(cnt.v_swpgonly > 0);
		cnt.v_swpgonly--;
		simple_unlock(&swap_data_lock);
	}

	/*
	 * free any swap resources.
	 */
	vm_anon_dropswap(anon);

	/*
	 * now that we've stripped the data areas from the anon, free the anon
	 * itself!
	 */
	if (anon->u.an_segment != NULL) {
		 vm_anon_free_segment(anon, anon->u.an_segment);
	}
	if (anon->u.an_page != NULL) {
		 vm_anon_free_page(anon, anon->u.an_segment, anon->u.an_page);
	}
	if (anon->an_swslot != 0) {
		anon->an_swslot = 0;
	}

	simple_lock(&anon->u.an_freelock);
	anon->u.an_nxt = anon->u.an_free;
	anon->u.an_free = anon;
	cnt.v_nfreeanon++;
	simple_unlock(&anon->u.an_freelock);
}

/*
 * uvm_anon_dropswap:  release any swap resources from this anon.
 *
 * => anon must be locked or have a reference count of 0.
 */
void
vm_anon_dropswap(anon)
	vm_anon_t anon;
{
	if (anon->an_swslot == 0) {
		return;
	}

	vm_swap_free(anon->an_swslot, 1);
	anon->an_swslot = 0;
}

void
vm_anon_release(anon)
	vm_anon_t 		anon;
{
	vm_segment_t 	segment;
	vm_page_t 		page;

	segment = anon->u.an_segment;
	page = anon->u.an_page;

	if (page) {
		vm_anon_release_page(anon, segment, page);
	}
	vm_anon_release_segment(anon, segment);
	if (anon->u.an_segment != NULL) {
		 vm_anon_free_segment(anon, anon->u.an_segment);
	}
	if (anon->u.an_page != NULL) {
		 vm_anon_free_page(anon, anon->u.an_segment, anon->u.an_page);
	}
	vm_anon_free(anon);
}

bool_t
vm_anon_pagein(amap, anon)
	vm_amap_t amap;
	vm_anon_t anon;
{
	vm_page_t page;
	vm_segment_t segment;
	vm_object_t object;
	int rv;

	//KASSERT(anon->an_lock == amap->am_lock);

	/* locked: anon */
	rv = vm_fault_anonget(NULL, amap, anon);
	/*
	 * if rv == 0, anon is still locked, else anon
	 * is unlocked
	 */
	switch (rv) {
	case 0:
		break;

	case ERESTART:
		return (FALSE);

	default:
		return (TRUE);
	}
	/*
	 * ok, we've got the page now.
	 * mark it as dirty, clear its swslot and un-busy it.
	 */

	segment = anon->u.an_segment;
	page = anon->u.an_page;
	object = segment->object;
	if (anon->an_swslot > 0) {
		vm_swap_free(anon->an_swslot, 1);
	}
	anon->an_swslot = 0;
	page->flags &= ~(PG_CLEAN);

	/*
	 * deactivate the page (to put it on a page queue)
	 */

	pmap_clear_reference(VM_PAGE_TO_PHYS(page));
	vm_page_lock_queues();
	if (page->wire_count == 0) {
		vm_page_deactivate(page);
	}
	vm_page_unlock_queues();
	if (page->flags & PG_WANTED) {
		wakeup(page);
		page->flags &= ~(PG_WANTED);
	}

	/*
	 * unlock the anon and we're done.
	 */

	simple_unlock(&anon->an_lock);
	if (object) {
		simple_unlock(&object->Lock);
	}
	return (FALSE);
}

static void
vm_anon_release_segment(anon, segment)
	vm_anon_t 		anon;
	vm_segment_t 	segment;
{
	KASSERT(segment != NULL);
	KASSERT((segment->flags & SEG_RELEASED) != 0);
	KASSERT((segment->flags & SEG_BUSY) != 0);
	KASSERT(segment->object == NULL);
	KASSERT(segment->anon == anon);

	vm_segment_lock_lists();
	vm_segment_anon_free(segment);
	vm_segment_unlock_lists();
}

static void
vm_anon_release_page(anon, segment, page)
	vm_anon_t 		anon;
	vm_segment_t 	segment;
	vm_page_t 		page;
{
	KASSERT(page != NULL);
	KASSERT((page->flags & PG_RELEASED) != 0);
	KASSERT((page->flags & PG_BUSY) != 0);
	KASSERT(page->segment == segment);
	KASSERT(page->anon == anon);

	vm_page_lock_queues();
	vm_page_anon_free(page);
	vm_page_unlock_queues();
}

static void
vm_anon_free_segment(anon, segment)
    vm_anon_t 		anon;
	vm_segment_t 	segment;
{
	vm_segment_t asg;

	asg = anon->u.an_segment;

	if (asg == segment) {
		if (segment->object) {
			vm_segment_lock_lists();
			segment->anon = NULL;
			vm_segment_unlock_lists();
			//simple_lock(&segment->object->Lock);
		} else {

			KASSERT((segment->flags & SEG_RELEASED) == 0);
			simple_lock(&anon->an_lock);
			pmap_page_protect(VM_SEGMENT_TO_PHYS(segment), VM_PROT_NONE);

			if ((segment->flags & SEG_BUSY) != 0) {
				/* tell them to dump it when done */
				segment->flags |= SEG_RELEASED;
				simple_unlock(&anon->an_lock);
				return;
			}
			vm_segment_lock_lists();
			vm_segment_anon_free(segment);
			vm_segment_unlock_lists();
			simple_unlock(&anon->an_lock);
		}
	}
}

static void
vm_anon_free_page(anon, segment, page)
	vm_anon_t 		anon;
	vm_segment_t 	segment;
	vm_page_t 		page;
{
	vm_page_t 		apg;

	KASSERT(page->segment == segment);

	apg = anon->u.an_page;
	if (apg == page) {
		if (page->segment) {
			vm_page_lock_queues();
			page->anon = NULL;
			vm_page_unlock_queues();
		} else {
			KASSERT((page->flags & PG_RELEASED) == 0);
			simple_lock(&anon->an_lock);
			pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_NONE);

			if ((page->flags & PG_BUSY) != 0) {
				/* tell them to dump it when done */
				page->flags |= PG_RELEASED;
				simple_unlock(&anon->an_lock);
				return;
			}
			vm_page_lock_queues();
			vm_page_anon_free(page);
			vm_page_unlock_queues();
			simple_unlock(&anon->an_lock);
		}
	}
}
