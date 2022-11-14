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
#define EFAULTRETRY 	1	/* error fault retry */
#define EFAULTCOPY 		2	/* error fault copy */

/* fault handler check flags */
#define FHDMLOOP		0x01	/* handler active | inactive | busy after main loop */
#define FHDOBJCOPY		0x02	/* handler active | inactive before object copy*/
#define FHDRETRY		0x04	/* handler active | inactive before retry */
#define FHDPMAP			0x08	/* handler active | inactive before pmap  */

/*
 * private prototypes
 */
static int				vm_fault_map_lookup(struct vm_faultinfo *, vm_offset_t, vm_prot_t);
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
			return (EFAULTRETRY);
		}
		vfi->page = vm_page_alloc(vfi->segment, vfi->segment->sg_offset);
		if(vfi->page == NULL) {
			unlock_and_deallocate(vfi);
			VM_WAIT;
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
			vfi->page = vm_page_lookup(vfi->segment, vfi->segment->sg_offset);
			if(VM_PAGE_TO_PHYS(vfi->page) == VM_SEGMENT_TO_PHYS(vfi->segment)) {
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
		vfi->page = vm_page_lookup(vfi->segment, vfi->segment->sg_offset);
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
	if (vfi->segment->sg_flags & SEG_BUSY) {
		SEGMENT_ASSERT_WAIT(vfi->segment, !change_wiring);
		unlock_things(vfi);
		cnt.v_intrans++;
		vm_object_deallocate(vfi->first_object);
		return (EFAULTRETRY);
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

int
vm_fault(map, vaddr, fault_type, change_wiring)
	vm_map_t	map;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;
	bool_t		change_wiring;
{
	vm_page_t 			old_page;
	int					error, result;
	bool_t				page_exists;
	struct vm_faultinfo vfi;

	//vfi.anon = NULL;

	cnt.v_faults++;

RetryFault: ;

	vfi.orig_map = map;
	vfi.orig_rvaddr = trunc_page(vaddr);
	vfi.orig_size = PAGE_SIZE;				/* can't get any smaller than this */

	result = vm_fault_map_lookup(&vfi, vaddr, fault_type);
	if (result != KERN_SUCCESS && vm_fault_lookup(&vfi, FALSE) == FALSE) {
		return (result);
	}

	vfi.lookup_still_valid = TRUE;
#ifdef notyet
	if ((vfi.entry->protection & fault_type) != fault_type) {
		vm_fault_unlockmaps(&vfi, FALSE);
		return (KERN_PROTECTION_FAILURE);
	}
#endif
	if (vfi.wired) {
		fault_type = vfi.prot;
	}
#ifdef notyet
	if (vfi.entry->needs_copy) {
		if ((fault_type & VM_PROT_WRITE) || (vfi.entry->object.vm_object == NULL)) {
			vm_fault_unlockmaps(&vfi, FALSE);
			vm_fault_amapcopy(&vfi);
			goto RetryFault;
		} else {
			vfi.prot &= ~VM_PROT_WRITE;
		}
	}
#endif
	vfi.first_segment = NULL;
	vfi.first_page = NULL;

	vm_object_lock(vfi.first_object);

	vfi.first_object->ref_count++;
	vfi.first_object->paging_in_progress++;

	//vfi.amap = vfi.entry->aref.ar_amap;
	vfi.object = vfi.first_object;
	vfi.offset = vfi.first_offset;

	/*
	 *	See whether this page is resident
	 */
	while (TRUE) {
		if (vm_fault_object(&vfi, change_wiring) == EFAULTRETRY) {
			goto RetryFault;
		}
		vm_fault_zerofill(&vfi);
	}
	vm_fault_handler_check(&vfi, FHDMLOOP);
	old_page = vfi->page;
	vm_fault_cow(&vfi, fault_type, old_page);
	vm_fault_handler_check(&vfi, FHDOBJCOPY);

RetryCopy:
	error = vm_fault_copy(&vfi, fault_type, change_wiring, page_exists, old_page);
	switch (error) {
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
#ifdef notyet
			if (vfi.page->flags & PG_AOBJ) {
				vfi.page->flags &= ~(PG_CLEAN);
				vm_aobject_dropswap(vfi.object, vfi.page->offset >> PAGE_SHIFT);
			}
#endif
		} else {
			vm_page_unwire(vfi.page);
		}
	} else {
		vm_page_activate(vfi.page);
	}
	vm_page_unlock_queues();

	if (vfi.segment->sg_flags & SEG_WANTED) {
		SEGMENT_WAKEUP(vfi.segment);
	}
	if (vfi.page->flags & PG_WANTED) {
		PAGE_WAKEUP(vfi.page);
	}
#ifdef notyet
	vm_fault_unlockall(&vfi, &vfi.amap, &vfi.object, NULL);
#endif
	unlock_and_deallocate(&vfi);

	return (KERN_SUCCESS);
}

/* TODO: better segment & page workings */
int
vm_fault_copy(vfi, fault_type, change_wiring, page_exists, old_page)
	struct vm_faultinfo *vfi;
	vm_prot_t fault_type;
	bool_t	change_wiring, page_exists;
	vm_page_t old_page;
{
	vm_object_t 	copy_object;
	vm_offset_t 	copy_offset;
	vm_segment_t 	copy_segment;
	vm_page_t 		copy_page;

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
				if(copy_segment->sg_flags & SEG_BUSY) {
					SEGMENT_ASSERT_WAIT(copy_segment, !change_wiring);
					copy_page = vm_page_lookup(copy_segment, copy_segment->sg_offset);
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
						copy_page = vm_page_alloc(copy_segment, copy_segment->sg_offset);
						if(copy_page == NULL) {
							vm_fault_release(vfi->segment, vfi->page);
							copy_object->ref_count--;
							vm_object_unlock(copy_object);
							unlock_and_deallocate(&vfi);
							VM_WAIT;
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

void
vm_fault_cow(vfi, fault_type, old_page)
	struct vm_faultinfo *vfi;
	vm_prot_t fault_type;
	vm_page_t old_page;
{
	if (vfi->object != vfi->first_object) {
		if (fault_type & VM_PROT_WRITE) {
			if ((vfi->first_segment != NULL && !TAILQ_EMPTY(vfi->first_segment->sg_memq)) ||
					(vfi->first_segment == NULL && TAILQ_EMPTY(vfi->first_segment->sg_memq))) {
				vm_segment_copy(vfi->segment, vfi->first_segment);
				vfi->first_page = vm_page_lookup(vfi->first_segment, vfi->first_segment->sg_offset);
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

int
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

void
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
		sgflag = segment->sg_flags & (SEG_ACTIVE | SEG_INACTIVE | SEG_BUSY);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE | PG_BUSY);
		if((sgflag != SEG_BUSY) || (pgflag != PG_BUSY)) {
			panic("vm_fault_handler_check: active, inactive or !busy after main loop");
		}
		break;

	case FHDOBJCOPY:
		sgflag = segment->sg_flags & (SEG_ACTIVE | SEG_INACTIVE);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE);
		if (sgflag || pgflag) {
			panic("vm_fault_handler_check: active or inactive before copy object handling");
		}
		break;

	case FHDRETRY:
		sgflag = segment->sg_flags & (SEG_ACTIVE | SEG_INACTIVE);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE);
		if (sgflag || pgflag) {
			panic("vm_fault_handler_check: active or inactive before retrying lookup");
		}
		break;

	case FHDPMAP:
		sgflag = segment->sg_flags & (SEG_ACTIVE | SEG_INACTIVE);
		pgflag = page->flags & (PG_ACTIVE | PG_INACTIVE);
		if (sgflag || pgflag) {
			panic("vm_fault_handler_check: active or inactive before pmap_enter");
		}
		break;
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
	//vm_amap_add ?
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
			VM_WAIT;
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
			if ((segment->sg_flags & SEG_BUSY) == 0) {
				return (0);
			}

			segment->sg_flags |= SEG_WANTED;

			if (page) {
				if ((page->flags & PG_BUSY) == 0) {
					return (0);
				}
				page->flags |= PG_WANTED;

				if(segment->sg_object) {
					vm_fault_unlockall(vfi, amap, NULL, anon);
				} else {
					vm_fault_unlockall(vfi, amap, NULL, NULL);
				}
			} else {
				page = vm_page_anon_alloc(segment, segment->sg_offset, anon);
				if(page == NULL) {
					vm_fault_unlockall(vfi, amap, NULL, anon);
					VM_WAIT;
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
				VM_WAIT;
			} else {
				page = vm_page_anon_alloc(segment, segment->sg_offset, anon);
				if (page == NULL) {
					vm_fault_unlockall(vfi, amap, NULL, anon);
					VM_WAIT;
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
			if (segment->sg_flags & SEG_WANTED) {
				wakeup(segment);
			}
			if (page->flags & PG_WANTED) {
				wakeup(page);
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
			return (ERESTART);
		}
		if (vfi != NULL && vm_amap_lookup(&vfi->entry->aref, vfi->orig_rvaddr - vfi->entry->start) != anon) {
			vm_fault_unlockall(vfi, amap, NULL, anon);
			return (ERESTART);
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
