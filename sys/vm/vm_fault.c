/*	$NetBSD: uvm_fault.c,v 1.76.4.3 2002/12/10 07:14:41 jmc Exp $	*/

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
 *
 * from: Id: uvm_fault.c,v 1.1.2.23 1998/02/06 05:29:05 chs Exp
 */
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
 *	@(#)vm_fault.c	8.5 (Berkeley) 1/9/95
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
 *	Page fault handling module.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mman.h>

#include <vm/include/vm.h>
#include <vm/include/vm_fault.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>
#include <vm/include/vm_pageout.h>
#include <vm/include/vm_aobject.h>
#include <vm/include/vm_swap.h>

/*
 * local data structures
 */
static struct vm_advice vmadvice[] = {
		{ MADV_NORMAL, 3, 4 },
		{ MADV_RANDOM, 0, 0 },
		{ MADV_SEQUENTIAL, 8, 7},
};

#define VM_MAXRANGE 16 	/* must be MAX() of nback+nforw+1 */

/* fault errors */
#define EFAULTRETRY 	1	/* error fault retry */
#define EFAULTCOPY 		2	/* error fault copy */
#define EFAULTPROT 		3	/* error fault protection */
#define EFAULTRESOURCE	4	/* error fault resource shortage */

/* fault handler check flags */
#define FHDMLOOP		0x01	/* handler active | inactive | busy after main loop */
#define FHDOBJCOPY		0x02	/* handler active | inactive before object copy*/
#define FHDRETRY		0x04	/* handler active | inactive before retry */
#define FHDPMAP			0x08	/* handler active | inactive before pmap  */
#define FHDANON			0x10	/* handler active | inactive after anon  */

/*
 * private prototypes
 */

static int             	vm_fault_pager(struct vm_faultinfo *, bool_t);
static int              vm_fault_object(struct vm_faultinfo *, bool_t);
static int              vm_fault_segment(struct vm_faultinfo *, bool_t);
static int              vm_fault_page(struct vm_faultinfo *, bool_t);
static int             	vm_fault_copy(struct vm_faultinfo *, vm_prot_t, bool_t, bool_t);
static void            	vm_fault_cow(struct vm_faultinfo *, vm_prot_t);
static int             	vm_fault_retry(struct vm_faultinfo *, vm_offset_t, vm_prot_t);
static void            	vm_fault_handler_check(struct vm_faultinfo *, int);
static void            	vm_fault_zerofill(struct vm_faultinfo *);
static int				vm_fault_map_lookup(struct vm_faultinfo *, vm_offset_t, vm_prot_t);
static void            	vm_fault_advice(struct vm_faultinfo *);
static void				vm_fault_amap(struct vm_faultinfo *, vm_anon_t *);
static int				vm_fault_anon(struct vm_faultinfo *, vm_prot_t);
static void 			vm_fault_amapcopy(struct vm_faultinfo *);
static __inline void 	vm_fault_anonflush(vm_anon_t *, int);
static __inline void 	vm_fault_unlockall(struct vm_faultinfo *, vm_amap_t, vm_object_t, vm_anon_t);
static __inline void 	vm_fault_unlockmaps(struct vm_faultinfo *, bool_t);
static __inline bool_t 	vm_fault_lookup(struct vm_faultinfo *, bool_t);
static __inline bool_t 	vm_fault_relock(struct vm_faultinfo *);

/*
 * inline functions
 */

void
vm_fault_free(segment, page)
	vm_segment_t 	segment;
	vm_page_t 		page;
{
	register vm_page_t 	ps;

	if (segment != NULL) {
		SEGMENT_WAKEUP(segment);
		vm_segment_lock_lists();
		ps = vm_page_lookup(segment, segment->offset);
		if (page != NULL) {
			PAGE_WAKEUP(page);
			vm_page_lock_queues();
			if (page == ps) {
				vm_page_free(page);
			} else {
				page = ps;
				vm_page_free(page);
			}
			vm_page_unlock_queues();
		} else {
			vm_segment_free(segment);
		}
		vm_segment_unlock_lists();
	}
}

void
vm_fault_release(segment, page)
	vm_segment_t 	segment;
	vm_page_t 		page;
{
	register vm_page_t 	ps;

	if (segment != NULL) {
		SEGMENT_WAKEUP(segment);
		vm_segment_lock_lists();
		ps = vm_page_lookup(segment, segment->offset);
		if (page != NULL) {
			PAGE_WAKEUP(page);
			vm_page_lock_queues();
			if (page == ps) {
				vm_page_activate(page);
			} else {
				page = ps;
				vm_page_activate(page);
			}
			vm_page_unlock_queues();
		} else {
			vm_segment_activate(segment);
		}
		vm_segment_unlock_lists();
	}
}

static __inline void
unlock_map(vfi)
	struct vm_faultinfo *vfi;
{
	if (vfi->lookup_still_valid) {
		vm_map_lookup_done(vfi->map, vfi->entry);
		vfi->lookup_still_valid = FALSE;
	}
}

static __inline void
unlock_things(vfi)
	struct vm_faultinfo *vfi;
{
	vfi->object->paging_in_progress--;
	vm_object_unlock(vfi->object);
	if (vfi->object != vfi->first_object) {
		vm_object_lock(vfi->first_object);
		vm_fault_free(vfi->first_segment, vfi->first_page);
		vfi->first_object->paging_in_progress--;
		vm_object_unlock(vfi->first_object);
	}
	unlock_map(vfi);
}

static __inline void
unlock_and_deallocate(vfi)
	struct vm_faultinfo *vfi;
{
	unlock_things(vfi);
	vm_object_deallocate(vfi->first_object);
}

static int
vm_fault_pager(vfi, change_wiring)
	struct vm_faultinfo *vfi;
	bool_t		change_wiring;
{
	int rv;

	if (((vfi->object->pager != NULL) && (!change_wiring || vfi->wired)) || (vfi->object == vfi->first_object)) {
		vfi->segment = vm_segment_alloc(vfi->object, vfi->offset);
		if(vfi->segment == NULL) {
			unlock_and_deallocate(vfi);
			vm_wait();
			return (EFAULTRETRY);
		}
		vfi->page = vm_page_alloc(vfi->segment, vfi->segment->offset);
		if(vfi->page == NULL) {
			unlock_and_deallocate(vfi);
			vm_wait();
			return (EFAULTRETRY);
		}
	}
	if (vfi->object->pager != NULL && (!change_wiring || vfi->wired)) {
		vm_object_unlock(vfi->object);

		unlock_map(vfi);
		cnt.v_pageins++;
		rv = vm_pager_get(vfi->object->pager, vfi->page, TRUE);

		vm_object_lock(vfi->object);

		if (rv == VM_PAGER_OK) {
			vfi->segment = vm_segment_lookup(vfi->object, vfi->offset);
			vfi->page = vm_page_lookup(vfi->segment, vfi->segment->offset);
			if (VM_PAGE_TO_PHYS(vfi->page) == VM_SEGMENT_TO_PHYS(vfi->segment)) {
				pmap_clear_modify(VM_PAGE_TO_PHYS(vfi->page));
				return (rv);
			}
		}

		if (rv == VM_PAGER_ERROR || rv == VM_PAGER_BAD) {
			vm_fault_free(vfi->segment, vfi->page);
			unlock_and_deallocate(vfi);
			return (KERN_PROTECTION_FAILURE);
		}

		if (vfi->object != vfi->first_object) {
			vm_fault_free(vfi->segment, vfi->page);
		}
	}
	return (rv);
}

static int
vm_fault_object(vfi, change_wiring)
	struct vm_faultinfo *vfi;
	bool_t		change_wiring;
{
	int error;

	vfi->segment = vm_segment_lookup(vfi->object, vfi->offset);
	if(vfi->segment != NULL) {
		vfi->page = vm_page_lookup(vfi->segment, vfi->segment->offset);
		if(vfi->page != NULL) {
			error = vm_fault_page(vfi, change_wiring);
			if(error != 0) {
				return (error);
			}
		} else {
			error = vm_fault_segment(vfi, change_wiring);
			if(error != 0) {
				return (error);
			}
		}
	} else {
		error = vm_fault_pager(vfi, change_wiring);
	}
	if (vfi->object != vfi->first_object) {
		vfi->first_segment = vfi->segment;
		vfi->first_page = vfi->page;
	}
	return (error);
}

static int
vm_fault_segment(vfi, change_wiring)
	struct vm_faultinfo *vfi;
	bool_t		change_wiring;
{
	if (vfi->segment->flags & SEG_BUSY) {
		SEGMENT_ASSERT_WAIT(vfi->segment, !change_wiring);
		unlock_things(vfi);
		cnt.v_intrans++;
		vm_object_deallocate(vfi->first_object);
		return (EFAULTRETRY);
	}

	vm_segment_lock_lists();
	if (vfi->segment->flags & SEG_INACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_inactive, vfi->segment, segmentq);
		vfi->segment->flags &= ~SEG_INACTIVE;
		cnt.v_segment_active_count--;
	}
	if (vfi->segment->flags & SEG_ACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_active, vfi->segment, segmentq);
		vfi->segment->flags &= ~SEG_ACTIVE;
		cnt.v_segment_active_count--;
	}
	vm_segment_unlock_lists();

	vfi->segment->flags |= SEG_BUSY;
	return (0);
}

static int
vm_fault_page(vfi, change_wiring)
	struct vm_faultinfo *vfi;
	bool_t		change_wiring;
{
	if (vfi->page->flags & PG_BUSY) {
		PAGE_ASSERT_WAIT(vfi->page, !change_wiring);
		unlock_things(vfi);
		cnt.v_intrans++;
		vm_object_deallocate(vfi->first_object);
		return (EFAULTRETRY);
	}

	vm_page_lock_queues();
	if (vfi->page->flags & PG_INACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_inactive, vfi->page, pageq);
		vfi->page->flags &= ~PG_INACTIVE;
		cnt.v_page_inactive_count--;
		cnt.v_reactivated++;
	}
	if (vfi->page->flags & PG_ACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_active, vfi->page, pageq);
		vfi->page->flags &= ~PG_ACTIVE;
		cnt.v_page_active_count--;
	}
	vm_page_unlock_queues();

	vfi->page->flags |= PG_BUSY;
	return (0);
}

/*
 *	vm_fault:
 *
 *	Handle a page fault occuring at the given address,
 *	requiring the given permissions, in the map specified.
 *	If successful, the page is inserted into the
 *	associated physical map.
 *
 *	NOTE: the given address should be truncated to the
 *	proper page address.
 *
 *	KERN_SUCCESS is returned if the page fault is handled; otherwise,
 *	a standard error specifying why the fault is fatal is returned.
 *
 *
 *	The map in question must be referenced, and remains so.
 *	Caller may hold no locks.
 */
int
vm_fault(map, vaddr, fault_type, change_wiring)
	vm_map_t	map;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;
	bool_t		change_wiring;
{
	vm_anon_t			anons;
	int					error, result;
	bool_t				page_exists;
	struct vm_faultinfo vfi;

	vfi.anon = NULL;

	cnt.v_faults++;

RetryFault: ;

	/*
	 *	Find the backing store object and offset into
	 *	it to begin the search.
	 */

	vfi.orig_map = map;
	vfi.orig_rvaddr = trunc_page(vaddr);
	vfi.orig_size = PAGE_SIZE;				/* can't get any smaller than this */

	result = vm_fault_map_lookup(&vfi, vaddr, fault_type);
	if (result != KERN_SUCCESS && vm_fault_lookup(&vfi, FALSE) == FALSE) {
		return (result);
	}

	vfi.lookup_still_valid = TRUE;

	if ((vfi.entry->protection & fault_type) != fault_type) {
		vm_fault_unlockmaps(&vfi, FALSE);
		return (KERN_PROTECTION_FAILURE);
	}

	if (vfi.wired) {
		fault_type = vfi.prot;
	}

	if (vfi.entry->needs_copy) {
		if ((fault_type & VM_PROT_WRITE) || (vfi.entry->object.vm_object == NULL)) {
			vm_fault_unlockmaps(&vfi, FALSE);
			vm_fault_amapcopy(&vfi);
			goto RetryFault;
		} else {
			vfi.prot &= ~VM_PROT_WRITE;
		}
	}

	vfi.first_segment = NULL;
	vfi.first_page = NULL;

   	/*
	 *	Make a reference to this object to
	 *	prevent its disposal while we are messing with
	 *	it.  Once we have the reference, the map is free
	 *	to be diddled.  Since objects reference their
	 *	shadows (and copies), they will stay around as well.
	 */

	vm_object_lock(vfi.first_object);

	vfi.first_object->ref_count++;
	vfi.first_object->paging_in_progress++;

	/*
	 *	INVARIANTS (through entire routine):
	 *
	 *	1)	At all times, we must either have the object
	 *		lock or a busy page in some object to prevent
	 *		some other thread from trying to bring in
	 *		the same page.
	 *
	 *		Note that we cannot hold any locks during the
	 *		pager access or when waiting for memory, so
	 *		we use a busy page then.
	 *
	 *		Note also that we aren't as concerned about
	 *		more than one thead attempting to pager_data_unlock
	 *		the same page at once, so we don't hold the page
	 *		as busy then, but do record the highest unlock
	 *		value so far.  [Unlock requests may also be delivered
	 *		out of order.]
	 *
	 *	2)	Once we have a busy page, we must remove it from
	 *		the pageout queues, so that the pageout daemon
	 *		will not grab it away.
	 *
	 *	3)	To prevent another thread from racing us down the
	 *		shadow chain and entering a new page in the top
	 *		object before we do, we must keep a busy page in
	 *		the top object while following the shadow chain.
	 *
	 *	4)	We must increment paging_in_progress on any object
	 *		for which we have a busy page, to prevent
	 *		vm_object_collapse from removing the busy page
	 *		without our noticing.
	 */

	/*
	 *	Search for the page at object/offset.
	 */

	vfi.amap = vfi.entry->aref.ar_amap;
	vfi.object = vfi.first_object;
	vfi.offset = vfi.first_offset;

	if(vfi.amap && vfi.object == NULL) {
		vm_fault_unlockmaps(&vfi, FALSE);
		return (KERN_PROTECTION_FAILURE);
	}
	vm_fault_advice(&vfi);
	vm_fault_amap(&vfi, &anons);

	/*
	 *	See whether this page is resident
	 */
	while (TRUE) {
		error = vm_fault_object(&vfi, change_wiring);
		if (error == (0 | VM_PAGER_OK)) {
		    break;
		} else if (error == (VM_PAGER_ERROR | VM_PAGER_BAD)) {
		  return (KERN_PROTECTION_FAILURE);
		} else {
		    goto RetryFault;
		}
		vm_fault_zerofill(&vfi);
	}
	vm_fault_handler_check(&vfi, FHDMLOOP);
	vm_fault_cow(&vfi, fault_type);
	vm_fault_handler_check(&vfi, FHDOBJCOPY);

	vfi.anon = &anons[vfi.centeridx];

	error = vm_fault_anonget(&vfi, vfi.amap, vfi.anon);
	switch (error) {
	case 0:
		break;
		
        case EFAULTRETRY:
	        goto RetryFault;
	}

	error = vm_fault_anon(&vfi, fault_type);
	switch (error) {
	case 0:
		break;
		
	case EFAULTRESOURCE:
	        return (KERN_RESOURCE_SHORTAGE);
	        
        case EFAULTRETRY:
	        goto RetryFault;
	}
	vm_fault_handler_check(&vfi, FHDANON);

RetryCopy:
	error = vm_fault_copy(&vfi, fault_type, change_wiring, page_exists);
	switch (error) {
	case 0:
		break;

	case EFAULTCOPY:
		goto RetryCopy;

	case EFAULTRETRY:
		goto RetryFault;
	}

	vm_fault_handler_check(&vfi, FHDRETRY);
	if (vm_fault_retry(&vfi, vaddr, fault_type) == EFAULTRETRY) {
		goto RetryFault;
	}

	vm_fault_handler_check(&vfi, FHDPMAP);
	vm_object_unlock(vfi.object);

	pmap_enter(vfi.map->pmap, vaddr, VM_PAGE_TO_PHYS(vfi.page), vfi.prot, vfi.wired);

	vm_object_lock(vfi.object);

	vm_page_lock_queues();
	if (change_wiring) {
		if (vfi.wired) {
			vm_page_wire(vfi.page);
			vm_segment_wire(vfi.segment);
			if (vfi.page->flags & PG_AOBJ) {
				vfi.page->flags &= ~(PG_CLEAN);
				vm_aobject_dropswap(vfi.object, vfi.page->offset >> PAGE_SHIFT);
			}
		} else {
			vm_page_unwire(vfi.page);
			vm_segment_unwire(vfi.segment);
		}
	} else {
		vm_page_activate(vfi.page);
		vm_segment_activate(vfi.segment);
	}
	vm_page_unlock_queues();

	if (vfi.segment->flags & SEG_WANTED) {
		SEGMENT_WAKEUP(vfi.segment);
	}
	if (vfi.page->flags & PG_WANTED) {
		PAGE_WAKEUP(vfi.page);
	}
	vm_fault_unlockall(&vfi, vfi.amap, vfi.object, NULL);
	unlock_and_deallocate(&vfi);

	return (KERN_SUCCESS);
}

static int
vm_fault_copy(vfi, fault_type, change_wiring, page_exists)
	struct vm_faultinfo *vfi;
	vm_prot_t fault_type;
	bool_t	change_wiring, page_exists;
{
	vm_object_t 	copy_object;
	vm_offset_t 	copy_offset;
	vm_segment_t 	copy_segment;
	vm_page_t 		copy_page, old_page;

	old_page = vfi->page;
	if (vfi->first_object->copy != NULL) {
		copy_object = vfi->first_object->copy;
		if ((fault_type & VM_PROT_WRITE) == 0) {
			vfi->prot &= ~VM_PROT_WRITE;
			vfi->page->flags |= PG_COPYONWRITE;
		} else {
			if (!vm_object_lock_try(copy_object)) {
				vm_object_unlock(vfi->object);
				/* should spin a bit here... */
				vm_object_lock(vfi->object);
				return (EFAULTCOPY);
			}
			copy_object->ref_count++;
			copy_offset = vfi->first_offset - copy_object->shadow_offset;
			copy_segment = vm_segment_lookup(copy_object, copy_offset);
			if(copy_segment != NULL) {
				if(copy_segment->flags & SEG_BUSY) {
					SEGMENT_ASSERT_WAIT(copy_segment, !change_wiring);
					copy_page = vm_page_lookup(copy_segment, copy_segment->offset);
					if (page_exists == (copy_page != NULL)) {
						if (copy_page->flags & PG_BUSY) {
							PAGE_ASSERT_WAIT(copy_page, !change_wiring);
							vm_fault_release(vfi->segment, vfi->page);
							copy_object->ref_count--;
							vm_object_unlock(copy_object);
							unlock_things(&vfi);
							vm_object_deallocate(vfi->first_object);
							return (EFAULTRETRY);
						}
					}
					if (!page_exists) {
						copy_page = vm_page_alloc(copy_segment, copy_segment->offset);
						if(copy_page == NULL) {
							vm_fault_release(vfi->segment, vfi->page);
							copy_object->ref_count--;
							vm_object_unlock(copy_object);
							unlock_and_deallocate(&vfi);
							vm_wait();
							return (EFAULTRETRY);
						}

						if (copy_object->pager != NULL) {
							vm_object_unlock(vfi->object);
							vm_object_unlock(copy_object);
							unlock_map(vfi);
							page_exists = vm_pager_has_page(copy_object->pager,
									(copy_offset + copy_object->paging_offset));
							vm_object_lock(copy_object);
							if (copy_object->shadow != vfi->object ||
												    copy_object->ref_count == 1) {
								vm_fault_free(copy_segment, copy_page);
								vm_object_unlock(copy_object);
								vm_object_deallocate(copy_object);
								vm_object_lock(vfi->object);
								return (EFAULTCOPY);
							}

							vm_object_lock(vfi->object);

							if (page_exists) {
								vm_fault_free(copy_segment, copy_page);
							}
						}
					}
					if (!page_exists) {
						vm_page_copy(vfi->page, copy_page);
						copy_page->flags &= ~PG_FAKE;
						vm_page_lock_queues();
						pmap_page_protect(VM_PAGE_TO_PHYS(old_page), VM_PROT_NONE);
						copy_page->flags &= ~PG_CLEAN;
						vm_page_activate(copy_page);
						vm_page_unlock_queues();
						PAGE_WAKEUP(copy_page);
					}
					copy_object->ref_count--;
					vm_object_unlock(copy_object);
					vfi->page->flags &= ~PG_COPYONWRITE;
				}
			}
		}
	}
	return (0);
}

static void
vm_fault_cow(vfi, fault_type)
	struct vm_faultinfo *vfi;
	vm_prot_t fault_type;
{
	vm_page_t old_page;

	old_page = vfi->page;
	if (vfi->object != vfi->first_object) {
		if (fault_type & VM_PROT_WRITE) {
			if ((vfi->first_segment != NULL && !TAILQ_EMPTY(&vfi->first_segment->memq)) ||
					(vfi->first_segment == NULL && TAILQ_EMPTY(&vfi->first_segment->memq))) {
				vm_segment_copy(vfi->segment, vfi->first_segment);
				vfi->first_page = vm_page_lookup(vfi->first_segment, vfi->first_segment->offset);
				if (vfi->first_page) {
					vfi->first_page->flags &= ~PG_FAKE;
				}
				vm_segment_lock_lists();
				vm_segment_activate(vfi->segment);
				vm_segment_deactivate(vfi->segment);
				pmap_page_protect(VM_SEGMENT_TO_PHYS(vfi->segment), VM_PROT_NONE);
				vm_segment_unlock_lists();
			} else {
				vm_page_copy(vfi->page, vfi->first_page);
				vfi->first_page->flags &= ~PG_FAKE;
				vm_page_lock_queues();
				vm_page_activate(vfi->page);
				vm_page_deactivate(vfi->page);
				pmap_page_protect(VM_PAGE_TO_PHYS(vfi->page), VM_PROT_NONE);
				vm_page_unlock_queues();
			}

			SEGMENT_WAKEUP(vfi->segment);
			PAGE_WAKEUP(vfi->page);
			vfi->object->paging_in_progress--;
			vm_object_unlock(vfi->object);

			cnt.v_cow_faults++;

			vfi->page = vfi->first_page;
			vfi->segment = vfi->first_segment;
			vfi->object = vfi->first_object;
			vfi->offset = vfi->first_offset;
			vm_object_lock(vfi->object);
			vfi->object->paging_in_progress--;
			vm_object_collapse(vfi->object);
			vfi->object->paging_in_progress++;
		} else {
			vfi->prot &= ~VM_PROT_WRITE;
			vfi->page->flags |= PG_COPYONWRITE;
		}
	}
}

static int
vm_fault_retry(vfi, vaddr, fault_type)
	struct vm_faultinfo *vfi;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;
{
	vm_object_t	retry_object;
	vm_offset_t	retry_offset;
	vm_prot_t	retry_prot;
	int 		result;

	if (!vfi->lookup_still_valid) {
		vm_object_unlock(vfi->object);
		result = vm_map_lookup(&vfi->map, vaddr, fault_type, &vfi->entry,
				&retry_object, &retry_offset, &vfi->prot, &vfi->wired,
				&vfi->su);
		vm_object_lock(vfi->object);

		if (result != KERN_SUCCESS) {
			vm_fault_release(vfi->segment, vfi->page);
			unlock_and_deallocate(vfi);
			return (result);
		}

		vfi->lookup_still_valid = TRUE;

		if ((retry_object != vfi->first_object)
				|| (retry_offset != vfi->first_offset)) {
			vm_fault_release(vfi->segment, vfi->page);
			unlock_and_deallocate(vfi);
			return (EFAULTRETRY);
		}
		vfi->prot &= retry_prot;
		if (vfi->page->flags & PG_COPYONWRITE) {
			vfi->prot &= ~VM_PROT_WRITE;
		}
	}

	if (vfi->prot & VM_PROT_WRITE) {
		vfi->page->flags &= ~PG_COPYONWRITE;
	}

	return (0);
}

static void
vm_fault_handler_check(vfi, flag)
	struct vm_faultinfo *vfi;
	int flag;
{
	vm_segment_t 	segment;
	vm_page_t 		page;
	int sgflag, pgflag;

	segment = vfi->segment;
	page = vfi->page;

	switch (flag) {
	case FHDMLOOP:
		sgflag = segment->flags & (SEG_ACTIVE | SEG_INACTIVE | SEG_BUSY);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE | PG_BUSY);
		if((sgflag != SEG_BUSY) || (pgflag != PG_BUSY)) {
			panic("vm_fault_handler_check: active, inactive or !busy after main loop");
		}
		break;

	case FHDOBJCOPY:
		sgflag = segment->flags & (SEG_ACTIVE | SEG_INACTIVE);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE);
		if (sgflag || pgflag) {
			panic("vm_fault_handler_check: active or inactive before copy object handling");
		}
		break;

	case FHDRETRY:
		sgflag = segment->flags & (SEG_ACTIVE | SEG_INACTIVE);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE);
		if (sgflag || pgflag) {
			panic("vm_fault_handler_check: active or inactive before retrying lookup");
		}
		break;

	case FHDANON:
		sgflag = segment->flags & (SEG_ACTIVE | SEG_INACTIVE | SEG_BUSY);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE | PG_BUSY);
		if ((sgflag != SEG_BUSY) || (pgflag != PG_BUSY)) {
			panic("vm_fault_handler_check: active or inactive or !busy after anon");
		}
		break;

	case FHDPMAP:
		sgflag = segment->flags & (SEG_ACTIVE | SEG_INACTIVE);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE);
		if (sgflag || pgflag) {
			panic("vm_fault_handler_check: active or inactive before pmap_enter");
		}
		break;
	}
}

static void
vm_fault_zerofill(vfi)
	struct vm_faultinfo *vfi;
{
	vm_object_t next_object;

	vfi->offset += vfi->object->shadow_offset;
	next_object = vfi->object->shadow;
	if (next_object == NULL) {
		if (vfi->object != vfi->first_object) {
			vfi->object->paging_in_progress--;
			vm_object_unlock(vfi->object);

			vfi->object = vfi->first_object;
			vfi->offset = vfi->first_offset;
			vfi->segment = vfi->first_segment;
			vfi->page = vfi->first_page;
			vm_object_lock(vfi->object);
		}
		if (vfi->first_segment == NULL) {
			vfi->first_segment = NULL;
			vm_segment_zero_fill(vfi->segment);
		} else {
			if (vfi->first_segment != NULL && vfi->first_page == NULL) {
				vfi->first_page = NULL;
				vm_page_zero_fill(vfi->page);
			}
		}
		cnt.v_zfod++;
		vfi->page->flags &= ~PG_FAKE;
		return;
	} else {
		vm_object_lock(next_object);
		if (vfi->object != vfi->first_object) {
			vfi->object->paging_in_progress--;
		}
		vm_object_unlock(vfi->object);
		vfi->object = next_object;
		vfi->object->paging_in_progress++;
	}
}

static int
vm_fault_map_lookup(vfi, vaddr, fault_type)
	struct vm_faultinfo *vfi;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;
{
	int  result;
	result = vm_map_lookup(&vfi->orig_map, vaddr, fault_type, &vfi->orig_entry, &vfi->first_object, &vfi->first_offset, &vfi->prot, &vfi->wired, &vfi->su);
	if (result == KERN_SUCCESS) {
		vfi->map = vfi->orig_map;
		vfi->entry = vfi->orig_entry;
		vfi->mapv = vfi->map->timestamp;
	}
	return (result);
}

/*
 * amap, anon & aobject related
 */
static void
vm_fault_advice(vfi)
	struct vm_faultinfo *vfi;
{
	KASSERT(vmadvice[vfi->entry->advice].advice == vfi->entry->advice);
	vfi->nback = min(vmadvice[vfi->entry->advice].nback, (vfi->orig_rvaddr - vfi->entry->start) >> PAGE_SHIFT);
	vfi->startva = vfi->orig_rvaddr - (vfi->nback << PAGE_SHIFT);
	vfi->nforw = min(vmadvice[vfi->entry->advice].nforw, ((vfi->entry->end - vfi->orig_rvaddr) >> PAGE_SHIFT) - 1);
	vfi->npages = vfi->nback + vfi->nforw + 1;
    vfi->nsegments = num_segments(vfi->npages);
    vfi->centeridx = vfi->nback;
}

static void
vm_fault_amap(vfi, anons)
	struct vm_faultinfo *vfi;
	vm_anon_t	*anons;
{
	vm_anon_t anons_store[VM_MAXRANGE];
	vm_page_t pages[VM_MAXRANGE];
	vm_offset_t start, end;
	int lcv;

	if (vfi->amap) {
		amap_lock(vfi->amap);
		anons = anons_store;
		vm_amap_lookups(&vfi->entry->aref, vfi->startva - vfi->entry->start, anons, vfi->npages);
	} else {
		anons = NULL;
	}

	/*
	 * for MADV_SEQUENTIAL mappings we want to deactivate the back pages
	 * now and then forget about them (for the rest of the fault).
	 */
	if (vfi->entry->advice == MADV_SEQUENTIAL && vfi->nback != 0) {
		if (vfi->amap) {
			vm_fault_anonflush(anons, vfi->nback);
		}
		/* flush object? */
		if (vfi->object) {
			start = (vfi->startva - vfi->entry->start) + vfi->entry->offset;
			end = start + (vfi->nback << PAGE_SHIFT);
			simple_lock(&vfi->object->Lock);
			(void)vm_object_segment_page_clean(vfi->object, start, end, TRUE, TRUE);
		}
		if (vfi->amap) {
			anons += vfi->nback;
		}
		vfi->startva += (vfi->nback << PAGE_SHIFT);
		vfi->npages -= vfi->nback;
		vfi->nback = vfi->centeridx = 0;
	}

	vfi->currva = vfi->startva;
	for (lcv = 0 ; lcv < vfi->npages ; lcv++, vfi->currva += PAGE_SIZE) {
		vfi->anon = anons[lcv];
	}

	pmap_update();
}

static int
vm_fault_anon(vfi, fault_type)
	struct vm_faultinfo *vfi;
	vm_prot_t fault_type;
{
	vm_anon_t 		oanon;

	if ((fault_type & VM_PROT_WRITE) != 0 && vfi->anon->an_ref > 1) {
		cnt.v_flt_acow++;
		oanon = vfi->anon;
		vfi->anon = vm_anon_alloc();
		if (vfi->anon) {
			vfi->segment = vm_segment_anon_alloc(vfi->object, vfi->offset, vfi->anon);
			if (vfi->segment) {
				vfi->page = vm_page_anon_alloc(vfi->segment, vfi->segment->offset, vfi->anon);
			}
		}
		if (vfi->anon == NULL || vfi->segment == NULL || vfi->page == NULL) {
			if (vfi->anon) {
				vm_anon_free(vfi->anon);
			}
			vm_fault_unlockall(vfi, vfi->amap, vfi->object, oanon);
			if (vfi->anon == NULL || cnt.v_swpgonly == cnt.v_swpages) {
				cnt.v_fltnoanon++;
				return (KERN_RESOURCE_SHORTAGE);
			}
			cnt.v_fltnoram++;
			vm_wait();
			return (EFAULTRETRY);
		}

		vm_segment_copy(oanon->u.an_segment, vfi->segment);
		vm_segment_lock_lists();
		vm_segment_activate(vfi->segment);
		 vfi->segment->flags &= ~SEG_BUSY;
		vm_page_copy(oanon->u.an_page, vfi->page);
		vm_page_lock_queues();
		vm_page_activate(vfi->page);
		vfi->page->flags &= ~(PG_BUSY|PG_FAKE);
		vm_page_unlock_queues();
		vm_segment_unlock_lists();
		vm_amap_add(&vfi->entry->aref, vfi->orig_rvaddr - vfi->entry->start, vfi->anon, 1);
		oanon->an_ref--;
	} else {
		cnt.v_flt_anon++;
		oanon = vfi->anon;
		vfi->segment = vfi->anon->u.an_segment;
		vfi->page = vfi->anon->u.an_page;
		if (vfi->anon->an_ref > 1) {
			vfi->prot = vfi->prot & ~VM_PROT_WRITE;
		}
	}
	vm_fault_unlockall(vfi, vfi->amap, vfi->object, oanon);
	return (0);
}

static __inline void
vm_fault_anonflush(anons, n)
	vm_anon_t *anons;
	int n;
{
	int lcv;
	vm_segment_t sg;
	vm_page_t pg;

	for (lcv = 0 ; lcv < n ; lcv++) {
		if (anons[lcv] == NULL) {
			continue;
		}
		simple_lock(&anons[lcv]->an_lock);
		sg = anons[lcv]->u.an_segment;
		pg = anons[lcv]->u.an_page;
		if (pg && (pg->flags & PG_BUSY) == 0) {
			vm_page_lock_queues();
			if (pg->wire_count == 0) {
				pmap_clear_reference(VM_PAGE_TO_PHYS(pg));
				vm_page_deactivate(pg);
			}
			vm_page_unlock_queues();
		}
		simple_unlock(&anons[lcv]->an_lock);
	}
}

static void
vm_fault_amapcopy(vfi)
	struct vm_faultinfo *vfi;
{
	for (;;) {
		if (vm_fault_lookup(vfi, TRUE) == FALSE) {
			return;
		}

		if (vfi->entry->needs_copy) {
			vm_amap_copy(vfi->map, vfi->entry, M_NOWAIT, TRUE, vfi->orig_rvaddr, vfi->orig_rvaddr + 1);
		}

		if (vfi->entry->needs_copy) {
			vm_fault_unlockmaps(vfi, TRUE);
			vm_wait();
			continue;
		}

		vm_fault_unlockmaps(vfi, TRUE);
		return;
	}
	/*NOTREACHED*/
}

int
vm_fault_anonget(vfi, amap, anon)
	struct vm_faultinfo *vfi;
	vm_amap_t amap;
	vm_anon_t anon;
{
	bool_t 			we_own;	/* we own anon's page? */
	bool_t 			locked;	/* did we relock? */
	vm_segment_t 	segment;
	vm_page_t 	 	page;
	int error;

	error = 0;
	cnt.v_fltanget++;

	for (;;) {
		we_own = FALSE;
		segment = anon->u.an_segment;
		page = anon->u.an_page;

		if(segment) {
			if ((segment->flags & SEG_BUSY) == 0) {
				return (0);
			}

			segment->flags |= SEG_WANTED;

			if (page) {
				if ((page->flags & PG_BUSY) == 0) {
					return (0);
				}
				page->flags |= PG_WANTED;

				if(segment->object) {
					vm_fault_unlockall(vfi, amap, NULL, anon);
				} else {
					vm_fault_unlockall(vfi, amap, NULL, NULL);
				}
			} else {
				page = vm_page_anon_alloc(segment, segment->offset, anon);
				if(page == NULL) {
					vm_fault_unlockall(vfi, amap, NULL, anon);
					vm_wait();
				} else {
					we_own = TRUE;
					vm_fault_unlockall(vfi, amap, NULL, anon);
					cnt.v_pageins++;
					error = vm_pager_get(vfi->object->pager, page, TRUE);
				}
			}
		} else {
			segment = vm_segment_anon_alloc(vfi->object, vfi->offset, anon);
			if(segment == NULL) {
				vm_fault_unlockall(vfi, amap, NULL, anon);
				vm_wait();
			} else {
				page = vm_page_anon_alloc(segment, segment->offset, anon);
				if (page == NULL) {
					vm_fault_unlockall(vfi, amap, NULL, anon);
					vm_wait();
				} else {
					we_own = TRUE;
					vm_fault_unlockall(vfi, amap, NULL, anon);
					cnt.v_pageins++;
					error = vm_pager_get(vfi->object->pager, page, TRUE);
				}
			}
		}
		locked = vm_fault_relock(vfi);
		if (locked && amap != NULL) {
			amap_lock(amap);
		}
		if (locked || we_own) {
			simple_lock(&anon->an_lock);
		}
		if (we_own) {
			if (segment->flags & SEG_WANTED) {
				wakeup(segment);
			}
			if (page->flags & PG_WANTED) {
				wakeup(page);
			}
			segment->flags &= ~(SEG_WANTED|SEG_BUSY);
			page->flags &= ~(PG_WANTED|PG_BUSY|PG_FAKE);

			if (page->flags & PG_RELEASED) {
				pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_NONE);
				simple_unlock(&anon->an_lock);
				vm_anon_free(anon);
				if (locked) {
					vm_fault_unlockall(vfi, amap, NULL, NULL);
				}
			}

			if (error) {
				anon->u.an_segment = NULL;
				anon->u.an_page = NULL;

				vm_swap_markbad(anon->an_swslot, 1);
				anon->an_swslot = SWSLOT_BAD;

				vm_segment_lock_lists();
				if (page) {
					vm_page_lock_queues();
					vm_page_anon_free(page);
					vm_page_unlock_queues();
				} else {
					vm_segment_anon_free(segment);
				}
				vm_segment_unlock_lists();

				if (locked) {
					vm_fault_unlockall(vfi, amap, NULL, anon);
				} else {
					simple_unlock(&anon->an_lock);
				}
				return (error);
			}

			if (segment) {
				vm_segment_lock_lists();
				vm_segment_activate(segment);
				vm_segment_unlock_lists();
			}
			if (page) {
				vm_page_lock_queues();
				vm_page_activate(page);
				vm_page_unlock_queues();
			}
			if (!locked) {
				simple_unlock(&anon->an_lock);
			}
		}
		if (!locked) {
			return (EFAULTRETRY);
		}
		if (vfi != NULL && vm_amap_lookup(&vfi->entry->aref, vfi->orig_rvaddr - vfi->entry->start) != anon) {
			vm_fault_unlockall(vfi, amap, NULL, anon);
			return (EFAULTRETRY);
		}
		cnt.v_fltanretry++;

		continue;
	} /* while (1) */
}

/*
 * uvmfault_unlockmaps: unlock the maps
 */
static __inline void
vm_fault_unlockmaps(vfi, write_locked)
	struct vm_faultinfo *vfi;
	bool_t write_locked;
{
	/*
	 * vfi can be NULL when this isn't really a fault,
	 * but merely paging in anon data.
	 */

	if (vfi == NULL) {
		return;
	}

	if (write_locked) {
		vm_map_unlock(vfi->map);
	} else {
		vm_map_unlock_read(vfi->map);
	}
}

/*
 * uvmfault_unlockall: unlock everything passed in.
 *
 * => maps must be read-locked (not write-locked).
 */

static __inline void
vm_fault_unlockall(vfi, amap, obj, anon)
	struct vm_faultinfo *vfi;
	vm_amap_t amap;
	vm_object_t obj;
	vm_anon_t anon;
{
	if (anon) {
		simple_unlock(&anon->an_lock);
	}
	if (obj) {
		simple_unlock(&obj->Lock);
	}
	if (amap) {
		amap_unlock(amap);
	}
	vm_fault_unlockmaps(vfi, FALSE);
}

/*
 * uvmfault_lookup: lookup a virtual address in a map
 *
 * => caller must provide a uvm_faultinfo structure with the IN
 *	params properly filled in
 * => we will lookup the map entry (handling submaps) as we go
 * => if the lookup is a success we will return with the maps locked
 * => if "write_lock" is TRUE, we write_lock the map, otherwise we only
 *	get a read lock.
 * => note that submaps can only appear in the kernel and they are
 *	required to use the same virtual addresses as the map they
 *	are referenced by (thus address translation between the main
 *	map and the submap is unnecessary).
 */
static __inline bool_t
vm_fault_lookup(vfi, write_lock)
	struct vm_faultinfo *vfi;
	bool_t write_lock;
{
	vm_map_t tmpmap;

	/*
	 * init vfi values for lookup.
	 */

	vfi->map = vfi->orig_map;
	vfi->size = vfi->orig_size;

	/*
	 * keep going down levels until we are done.   note that there can
	 * only be two levels so we won't loop very long.
	 */
	while (1) {
		/*
		 * lock map
		 */
		if (write_lock) {
			vm_map_lock(vfi->map);
		} else {
			vm_map_lock_read(vfi->map);
		}

		/*
		 * lookup
		 */
		if (!vm_map_lookup_entry(vfi->map, vfi->orig_rvaddr, &vfi->entry)) {
			vm_fault_unlockmaps(vfi, write_lock);
			return (FALSE);
		}

		/*
		 * reduce size if necessary
		 */
		if (vfi->entry->end - vfi->orig_rvaddr < vfi->size) {
			vfi->size = vfi->entry->end - vfi->orig_rvaddr;
		}

		/*
		 * submap?    replace map with the submap and lookup again.
		 * note: VAs in submaps must match VAs in main map.
		 */
		if(vfi->entry->is_sub_map) {
			tmpmap = vfi->entry->object.sub_map;
			if (write_lock) {
				vm_map_unlock(vfi->map);
			} else {
				vm_map_unlock_read(vfi->map);
			}
			vfi->map = tmpmap;
			continue;
		}

		/*
		 * got it!
		 */
		vfi->mapv = vfi->map->timestamp;
		return (TRUE);
	} /* while loop */
}

/*
 * uvmfault_relock: attempt to relock the same version of the map
 *
 * => fault data structures should be unlocked before calling.
 * => if a success (TRUE) maps will be locked after call.
 */
static __inline bool_t
vm_fault_relock(vfi)
	struct vm_faultinfo *vfi;
{
	/*
	 * ufi can be NULL when this isn't really a fault,
	 * but merely paging in anon data.
	 */

	if (vfi == NULL) {
		return (TRUE);
	}

	cnt.v_fltrelck++;

	/*
	 * relock map.   fail if version mismatch (in which case nothing
	 * gets locked).
	 */

	vm_map_lock_read(vfi->map);
	if (vfi->mapv != vfi->map->timestamp) {
		vm_map_unlock_read(vfi->map);
		return (FALSE);
	}
	cnt.v_fltrelckok++;
	return (TRUE);
}

/*
 *	vm_fault_wire:
 *
 *	Wire down a range of virtual addresses in a map.
 */
int
vm_fault_wire(map, start, end)
	vm_map_t	map;
	vm_offset_t	start, end;
{
	register vm_offset_t	va;
	register pmap_t		pmap;
	int			rv;

	pmap = vm_map_pmap(map);

	/*
	 *	Inform the physical mapping system that the
	 *	range of addresses may not fault, so that
	 *	page tables and such can be locked down as well.
	 */

	pmap_pageable(pmap, start, end, FALSE);

	/*
	 *	We simulate a fault to get the page and enter it
	 *	in the physical map.
	 */

	for (va = start; va < end; va += PAGE_SIZE) {
		rv = vm_fault(map, va, VM_PROT_NONE, TRUE);
		if (rv) {
			if (va != start) {
				vm_fault_unwire(map, start, va);
			}
			return (rv);
		}
	}
	return (KERN_SUCCESS);
}


/*
 *	vm_fault_unwire:
 *
 *	Unwire a range of virtual addresses in a map.
 */
void
vm_fault_unwire(map, start, end)
	vm_map_t	map;
	vm_offset_t	start, end;
{

	register vm_offset_t	va, pa;
	register pmap_t		pmap;

	pmap = vm_map_pmap(map);


	/*
	 *	Since the pages are wired down, we must be able to
	 *	get their mappings from the physical map system.
	 */

	vm_page_lock_queues();

	for (va = start; va < end; va += PAGE_SIZE) {
		pa = pmap_extract(pmap, va);
		if (pa == (vm_offset_t) 0) {
			panic("unwire: page not in pmap");
		}
		pmap_change_wiring(pmap, va, FALSE);
		vm_page_unwire(PHYS_TO_VM_PAGE(pa));
	}
	vm_page_unlock_queues();

	/*
	 *	Inform the physical mapping system that the range
	 *	of addresses may fault, so that page tables and
	 *	such may be unwired themselves.
	 */

	pmap_pageable(pmap, start, end, TRUE);

}

/*
 *	Routine:
 *		vm_fault_copy_entry
 *	Function:
 *		Copy all of the pages from a wired-down map entry to another.
 *
 *	In/out conditions:
 *		The source and destination maps must be locked for write.
 *		The source map entry must be wired down (or be a sharing map
 *		entry corresponding to a main map entry that is wired down).
 */
void
vm_fault_copy_entry(dst_map, src_map, dst_entry, src_entry)
	vm_map_t		dst_map;
	vm_map_t		src_map;
	vm_map_entry_t	dst_entry;
	vm_map_entry_t	src_entry;
{
	vm_object_t		dst_object, src_object;
	vm_segment_t	dst_segment, src_segment;
	vm_page_t		dst_page, src_page;
	vm_offset_t		vaddr, dst_offset, src_offset;
	vm_prot_t		prot;

#ifdef	lint
	src_map++;
#endif

	src_object = src_entry->object.vm_object;
	src_offset = src_entry->offset;

	/*
	 *	Create the top-level object for the destination entry.
	 *	(Doesn't actually shadow anything - we copy the pages
	 *	directly.)
	 */
	dst_object = vm_object_allocate((vm_size_t) (dst_entry->end - dst_entry->start));

	dst_entry->object.vm_object = dst_object;
	dst_entry->offset = 0;

	prot  = dst_entry->max_protection;

	/*
	 *	Loop through all of the pages in the entry's range, copying
	 *	each one from the source object (it should be there) to the
	 *	destination object.
	 */
	for (vaddr = dst_entry->start, dst_offset = 0; vaddr < dst_entry->end; vaddr += PAGE_SIZE, dst_offset += PAGE_SIZE) {
		vm_object_lock(dst_object);
retry:
		dst_segment = vm_segment_alloc(dst_object, round_segment(dst_offset));
		if (dst_segment == NULL) {
			vm_object_unlock(dst_object);
			vm_wait();
			vm_object_lock(dst_object);
			goto retry;
		}

		/*
		 *	Find the page in the source object, and copy it in.
		 *	(Because the source is wired down, the page will be
		 *	in memory.)
		 */
		vm_object_lock(src_object);
		src_segment = vm_segment_lookup(src_object, round_segment(dst_offset) + src_offset);
		if (src_segment == NULL) {
			panic("vm_fault_copy_wired: segment missing");
		}

		/* look through segment for page. */
		dst_page = vm_page_lookup(dst_segment, dst_offset);
		if (dst_page != NULL) {
			/* page found; copy pages */
			src_page = vm_page_lookup(src_segment, dst_offset);
			if (src_page == NULL) {
				panic("vm_fault_copy_wired: page missing");
			}

			vm_page_copy(src_page, dst_page);

			/*
			 *	Enter it in the pmap...
			 */
			vm_object_unlock(src_object);
			vm_object_unlock(dst_object);

			pmap_enter(dst_map->pmap, vaddr, VM_PAGE_TO_PHYS(dst_page), prot, FALSE);


			/*
			 *	Mark it no longer busy, and put it on the active list.
			 */
			vm_object_lock(dst_object);
			vm_segment_lock_lists();
			vm_page_lock_queues();
			vm_page_activate(dst_page);
			vm_page_unlock_queues();
			PAGE_WAKEUP(dst_page);
			vm_segment_unlock_lists();
			vm_object_unlock(dst_object);
		} else {
			/* page not found in segment, copy the segment */

			vm_segment_copy(src_segment, dst_segment);

			/*
			 *	Enter it in the pmap...
			 */
			vm_object_unlock(src_object);
			vm_object_unlock(dst_object);

			pmap_enter(dst_map->pmap, vaddr, VM_SEGMENT_TO_PHYS(dst_segment), prot, FALSE);

			/*
			 *	Mark it no longer busy, and put it on the active list.
			 */
			vm_object_lock(dst_object);
			vm_segment_lock_lists();
			vm_segment_activate(dst_segment);
			vm_segment_unlock_lists();
			SEGMENT_WAKEUP(dst_segment);
			vm_object_unlock(dst_object);
		}
	}
}
