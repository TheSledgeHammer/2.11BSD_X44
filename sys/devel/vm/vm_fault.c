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

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_fault.h>
#include <devel/vm/include/vm_page.h>
#include <devel/vm/include/vm_pageout.h>
#include <devel/vm/include/vm_segment.h>

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
#define FAULTCONT 	0	/* fault continue */
#define FAULTRETRY 	1	/* error fault retry */
#define FAULTCOPY 	2	/* error fault copy */

/*
 * private prototypes
 */

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
		ps = vm_page_lookup(segment, segment->sg_offset);
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
		ps = vm_page_lookup(segment, segment->sg_offset);
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
	register vm_segment_t so;

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
			VM_WAIT;
			return (FAULTRETRY);
		}
		vfi->page = vm_page_alloc(vfi->segment, vfi->segment->sg_offset);
		if(vfi->page == NULL) {
			unlock_and_deallocate(vfi);
			VM_WAIT;
			return (FAULTRETRY);
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
			vfi->page = vm_page_lookup(vfi->segment, vfi->segment->sg_offset);
			if(VM_PAGE_TO_PHYS(vfi->page) == VM_SEGMENT_TO_PHYS(vfi->segment)) {
				pmap_clear_modify(VM_PAGE_TO_PHYS(vfi->page));
				return (rv);
			}
		}

		if (rv == VM_PAGER_ERROR || rv == VM_PAGER_BAD) {
			vm_fault_free_segment_page(vfi->object, vfi->segment, vfi->page);
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
		vfi->page = vm_page_lookup(vfi->segment, vfi->segment->sg_offset);
		if(vfi->page != NULL) {
			error = vm_fault_page(vfi, change_wiring);
			if(error != FAULTCONT) {
				return (error);
			}
		} else {
			error = vm_fault_segment(vfi, change_wiring);
			if(error != FAULTCONT) {
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
	if (vfi->segment->sg_flags & SEG_BUSY) {
		SEGMENT_ASSERT_WAIT(vfi->segment, !change_wiring);
		unlock_things(vfi);
		cnt.v_intrans++;
		vm_object_deallocate(vfi->first_object);
		return (FAULTRETRY);
	}

	vm_segment_lock_lists();
	if (vfi->segment->sg_flags & SEG_INACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_inactive, vfi->segment, sg_list);
		vfi->segment->sg_flags &= ~SEG_INACTIVE;
		cnt.v_segment_active_count--;
	}
	if (vfi->segment->sg_flags & SEG_ACTIVE) {
		CIRCLEQ_REMOVE(&vm_segment_list_active, vfi->segment, sg_list);
		vfi->segment->sg_flags &= ~SEG_ACTIVE;
		cnt.v_segment_active_count--;
	}
	vm_segment_unlock_lists();

	vfi->segment->sg_flags |= SEG_BUSY;
	return (FAULTCONT);
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
		return (FAULTRETRY);
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
	return (FAULTCONT);
}

int
vm_fault(map, vaddr, fault_type, change_wiring)
	vm_map_t	map;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;
	bool_t		change_wiring;
{
	vm_prot_t			prot;
	int					result;
	bool_t				page_exists;
	vm_page_t 			old_page;
	struct vm_faultinfo vfi;

	cnt.v_faults++;

RetryFault: ;
	result = vm_fault_lookup(vfi, vaddr, fault_type);
	if(result != KERN_SUCCESS) {
		return (result);
	}

	vfi->lookup_still_valid = TRUE;
	if (vfi.wired) {
		fault_type = prot;
	}

	vfi.first_segment = NULL;
	vfi.first_page = NULL;

	vm_object_lock(vfi.first_object);

	vfi.first_object->ref_count++;
	vfi.first_object->paging_in_progress++;

	vfi.object = vfi.first_object;
	vfi.offset = vfi.first_offset;

	/*
	 *	See whether this page is resident
	 */
	while (TRUE) {
		if (vm_fault_object(&vfi, change_wiring) == FAULTRETRY) {
			goto RetryFault;
		}
		vm_fault_zerofill(&vfi);
	}
	vm_fault_check_flags(&vfi);

	/* Revise: fault_cow/fault_copy function */
	old_page = vfi.page;
	if (vfi.object != vfi.first_object) {
		if (fault_type & VM_PROT_WRITE) {
			vm_page_copy(vfi.page, vfi.first_page);
			vfi.first_page->flags &= ~PG_FAKE;
			vm_page_lock_queues();
			vm_page_activate(vfi.page);
			vm_page_deactivate(vfi.page);
			pmap_page_protect(VM_PAGE_TO_PHYS(vfi.page), VM_PROT_NONE);
			vm_page_unlock_queues();

			PAGE_WAKEUP(vfi.page);
			vfi.object->paging_in_progress--;
			vm_object_unlock(vfi.object);

			cnt.v_cow_faults++;
			vfi.page = vfi.first_page;
			vfi.object = vfi.first_object;
			vfi.offset = vfi.first_offset;
			vm_object_lock(vfi.object);
			vfi.object->paging_in_progress--;
			vm_object_collapse(vfi.object);
			vfi.object->paging_in_progress++;
		} else {
	    	prot &= ~VM_PROT_WRITE;
	    	vfi.page->flags |= PG_COPYONWRITE;
		}
	}

	if (vfi.page->flags & (PG_ACTIVE|PG_INACTIVE)) {
		panic("vm_fault: active or inactive before copy object handling");
	}

RetryCopy:
	if (vfi.first_object->copy != NULL) {
		vm_object_t copy_object;
		vm_offset_t copy_offset;
		vm_page_t copy_page;

		copy_object = vfi.first_object->copy;

		if ((fault_type & VM_PROT_WRITE) == 0) {
			prot &= ~VM_PROT_WRITE;
			vfi.page->flags |= PG_COPYONWRITE;
		} else {
			if (!vm_object_lock_try(copy_object)) {
				vm_object_unlock(vfi.object);
				/* should spin a bit here... */
				vm_object_lock(vfi.object);
				goto RetryCopy;
			}
			copy_object->ref_count++;
			copy_offset = vfi.first_offset - copy_object->shadow_offset;
			copy_page = vm_page_lookup(copy_object, copy_offset);
			if (page_exists == (copy_page != NULL)) {
				if (copy_page->flags & PG_BUSY) {
					PAGE_ASSERT_WAIT(copy_page, !change_wiring);
					vm_fault_release(vfi.segment, vfi.page);
					copy_object->ref_count--;
					vm_object_unlock(copy_object);
					unlock_things(&vfi);
					vm_object_deallocate(vfi.first_object);
					goto RetryFault;
				}
			}
			if (!page_exists) {

			}
		}
	}

	vm_object_unlock(vfi.object);

	pmap_enter(map->pmap, vaddr, VM_PAGE_TO_PHYS(vfi.page), prot, vfi.wired);

	vm_object_lock(vfi.object);
	vm_page_lock_queues();
	if (change_wiring) {
		if (vfi.wired) {
			vm_page_wire(vfi.page);
		} else {
			vm_page_unwire(vfi.page);
		}
	} else {
		vm_page_activate(vfi.page);
	}
	vm_page_unlock_queues();

	SEGMENT_WAKEUP(vfi.segment);
	PAGE_WAKEUP(vfi.page);
	unlock_and_deallocate(&vfi);
	return (KERN_SUCCESS);
}

void
vm_fault_check_flags(vfi)
	struct vm_faultinfo *vfi;
{
	int segflags, pgflags;

	segflags = (SEG_ACTIVE | SEG_INACTIVE | SEG_BUSY);
	pgflags = (PG_ACTIVE | PG_INACTIVE | PG_BUSY);

	if (((vfi->segment->sg_flags & segflags) != SEG_BUSY) || ((vfi->page->flags & pgflags) != PG_BUSY)) {
		panic("vm_fault_check_flags: active, inactive or !busy after main loop");
	}
}

void
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
			vm_segment_zero_fill(vfi->segment, vfi->page);
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
vm_fault_lookup(vfi, vaddr, fault_type)
	struct vm_faultinfo *vfi;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;
{
	vm_prot_t		prot;
	bool_t			su;
	int 			result;

	result = vm_map_lookup(&vfi->orig_map, vaddr, fault_type, &vfi->orig_entry, &vfi->first_object, &vfi->first_offset, &prot, &vfi->wired, &su);
	if (result == KERN_SUCCESS) {
		vfi->map = vfi->orig_map;
		vfi->entry = vfi->orig_entry;
		vfi->mapv = vfi->map->timestamp;
	}

	return (result);
}

/* uvm /anons related */
static bool_t 	vm_fault_lookup(struct vm_faultinfo *, bool_t);
static bool_t 	vm_fault_relock(struct vm_faultinfo *);
static void 	vm_fault_unlockall(struct vm_faultinfo *, vm_amap_t, vm_object_t, vm_anon_t);
static void 	vm_fault_unlockmaps(struct vm_faultinfo *, bool_t);

int
vm_fault_anonget(vfi, amap, anon)
	struct vm_faultinfo *vfi;
	vm_amap_t amap;
	vm_anon_t anon;
{
	bool_t we_own;	/* we own anon's page? */
	bool_t locked;	/* did we relock? */
	vm_page_t page;
	int result;

	while (1) {
		we_own = FALSE;
		page = anon->u.an_page;
		if (page) {
			if (page->segment) {

			}
		}
	} /* while (1) */
}

/*
 * uvmfault_unlockmaps: unlock the maps
 */

static __inline void
vm_fault_unlockmaps(vfi, write_locked)
	struct vm_faultinfo *vfi;
	boolean_t write_locked;
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
