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
 * private prototypes
 */
static void vm_fault_amapcopy(vm_map_t, vm_map_entry_t, vm_offset_t, unsigned int);
static __inline void vm_fault_anonflush(vm_anon_t *, int);
static __inline void vm_fault_unlockmaps(vm_map_t, bool_t);
static __inline void vm_fault_unlockall(vm_map_t, vm_amap_t, vm_object_t, vm_anon_t);
static __inline bool_t vm_fault_lookup(vm_map_t, vm_map_t, bool_t, unsigned int);
static __inline bool_t vm_fault_relock(vm_map_t, unsigned int);

/*
 * inline functions
 */

/*
 * uvmfault_anonflush: try and deactivate pages in specified anons
 *
 * => does not have to deactivate page if it is busy
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

/*
 * normal functions
 */

/*
 * uvmfault_amapcopy: clear "needs_copy" in a map.
 *
 * => called with VM data structures unlocked (usually, see below)
 * => we get a write lock on the maps and clear needs_copy for a VA
 * => if we are out of RAM we sleep (waiting for more)
 */

static void
vm_fault_amapcopy(vfi, map, entry, size, timestamp)
	struct vm_faultinfo *vfi;
	vm_map_t 	   map;
	vm_map_entry_t entry;
	vm_offset_t 	size;
	unsigned int timestamp;
{
	for (;;) {
		/*
		 * no mapping?  give up.
		 */
		if (vm_fault_lookup(map, size, TRUE, timestamp) == FALSE) {
			return;
		}

		/*
		 * copy if needed.
		 */

		if (entry->needs_copy) {
			vm_amap_copy(map, entry, M_NOWAIT, TRUE, size, size + 1);
		}

		/*
		 * didn't work?  must be out of RAM.   unlock and sleep.
		 */

		if (entry->needs_copy) {
			vm_fault_unlockmaps(map, TRUE);
			VM_WAIT;//vm_wait();
			continue;
		}

		/*
		 * got it!   unlock and return.
		 */

		vm_fault_unlockmaps(map, TRUE);
		return;
	}
	/*NOTREACHED*/
}

/*
 * uvmfault_anonget: get data in an anon into a non-busy, non-released
 * page in that anon.
 *
 * => maps, amap, and anon locked by caller.
 * => if we fail (result != VM_PAGER_OK) we unlock everything.
 * => if we are successful, we return with everything still locked.
 * => we don't move the page on the queues [gets moved later]
 * => if we allocate a new page [we_own], it gets put on the queues.
 *    either way, the result is that the page is on the queues at return time
 * => for pages which are on loan from a uvm_object (and thus are not
 *    owned by the anon): if successful, we return with the owning object
 *    locked.   the caller must unlock this object when it unlocks everything
 *    else.
 */
int
vm_fault_anonget(map, amap, anon)
	vm_map_t  map;
	vm_amap_t amap;
	vm_anon_t anon;
{
	bool_t we_own;	/* we own anon's page? */
	bool_t locked;	/* did we relock? */
	vm_map_entry_t entry;
	vm_page_t pg;
	int error;

	error = 0;

	if (anon->u.an_page) {
		curproc->p_stats->p_ru.ru_minflt++;
	} else {
		curproc->p_stats->p_ru.ru_majflt++;
	}

	/*
	 * loop until we get it, or fail.
	 */

	for (;;) {
		we_own = FALSE;		/* TRUE if we set PG_BUSY on a page */
		pg = anon->u.an_page;

		/*
		 * page there?   make sure it is not busy/released.
		 */

		if (pg) {

			/*
			 * at this point, if the page has a uobject [meaning
			 * we have it on loan], then that uobject is locked
			 * by us!   if the page is busy, we drop all the
			 * locks (including uobject) and try again.
			 */

			if ((pg->flags & PG_BUSY) == 0) {
				return (VM_PAGER_OK);
			}
			pg->flags |= PG_WANTED;
			cnt.v_fltpgwait++;

			/*
			 * the last unlock must be an atomic unlock+wait on
			 * the owner of page
			 */

			if (pg->object) {	/* owner is uobject ? */
				vm_fault_unlockall(map, amap, NULL, anon);
			} else {
				/* anon owns page */
				vm_fault_unlockall(map, amap, NULL, NULL);
			}
		} else {

			/*
			 * no page, we must try and bring it in.
			 */

			pg = uvm_pagealloc(NULL, 0, anon, 0);
			if (pg == NULL) {		/* out of RAM.  */
				vm_fault_unlockall(map, amap, NULL, anon);
				cnt.v_fltnoram++;
				uvm_wait("flt_noram1");
			} else {
				/* we set the PG_BUSY bit */
				we_own = TRUE;
				vm_fault_unlockall(map, amap, NULL, anon);

				/*
				 * we are passing a PG_BUSY+PG_FAKE+PG_CLEAN
				 * page into the uvm_swap_get function with
				 * all data structures unlocked.  note that
				 * it is ok to read an_swslot here because
				 * we hold PG_BUSY on the page.
				 */
				cnt.v_pageins++;
				error = vm_swap_get(pg, anon->an_swslot, PGO_SYNCIO); /* fix; non-existent */

				/*
				 * we clean up after the i/o below in the
				 * "we_own" case
				 */
			}
		}

		/*
		 * now relock and try again
		 */

		locked = vm_fault_relock(map, map->timestamp);
		if (locked && amap != NULL) {
			amap_lock(amap);
		}
		if (locked || we_own) {
			simple_lock(&anon->an_lock);
		}

		/*
		 * if we own the page (i.e. we set PG_BUSY), then we need
		 * to clean up after the I/O. there are three cases to
		 * consider:
		 *   [1] page released during I/O: free anon and ReFault.
		 *   [2] I/O not OK.   free the page and cause the fault
		 *       to fail.
		 *   [3] I/O OK!   activate the page and sync with the
		 *       non-we_own case (i.e. drop anon lock if not locked).
		 */

		if (we_own) {
			if (pg->flags & PG_WANTED) {
				wakeup(pg);
			}
			if (error) {

				/*
				 * remove the swap slot from the anon
				 * and mark the anon as having no real slot.
				 * don't free the swap slot, thus preventing
				 * it from being used again.
				 */

				if (anon->an_swslot > 0) {
					vm_swap_markbad(anon->an_swslot, 1);
				}
				anon->an_swslot = SWSLOT_BAD;

				if ((pg->flags & PG_RELEASED) != 0) {
					goto released;
				}

				/*
				 * note: page was never !PG_BUSY, so it
				 * can't be mapped and thus no need to
				 * pmap_page_protect it...
				 */

				uvm_lock_pageq();
				uvm_pagefree(pg);
				uvm_unlock_pageq();

				if (locked) {
					vm_fault_unlockall(map, amap, NULL, anon);
				} else {
					simple_unlock(&anon->an_lock);
				}
				return error;
			}

			if ((pg->flags & PG_RELEASED) != 0) {
released:
				KASSERT(anon->an_ref == 0);

				/*
				 * released while we unlocked amap.
				 */

				if (locked) {
					vm_fault_unlockall(map, amap, NULL, NULL);
				}

				vm_anon_release(anon);

				if (error) {
					return (error);
				}
				return (ERESTART);
			}

			/*
			 * we've successfully read the page, activate it.
			 */

			vm_page_lock_queues();
			vm_page_activate(pg);
			vm_page_unlock_queues();
			pg->flags &= ~(PG_WANTED|PG_BUSY|PG_FAKE);
			if (!locked) {
				simple_unlock(&anon->an_lock);
			}
		}

		/*
		 * we were not able to relock.   restart fault.
		 */

		if (!locked) {
			return (ERESTART);
		}

		/*
		 * verify no one has touched the amap and moved the anon on us.
		 */

		if (map != NULL && vm_map_lookup_entry(map, map->size, &entry)) {
			if(vm_amap_lookup(&entry->aref, map->size - entry->start) != anon) {
				vm_fault_unlockall(map, amap, NULL, anon);
				return (ERESTART);
			}
		}

		/*
		 * try it again!
		 */

		cnt.v_fltanretry++;
		continue;
	}
	/*NOTREACHED*/
}

int
vm_fault(map, vaddr, fault_type, change_wiring)
	vm_map_t	map;
	vm_offset_t	vaddr;
	vm_prot_t	fault_type;
	bool_t		change_wiring;
{
	vm_object_t				first_object;
	vm_offset_t				first_offset;
	vm_map_entry_t			entry;
	vm_amap_t 				amap;
	vm_anon_t 				anons_store[VM_MAXRANGE], *anons, anon, oanon;
	register vm_object_t	object;
	register vm_offset_t	offset;
	register vm_segment_t	seg;
	register vm_page_t		m;
	vm_segment_t			first_seg;
	vm_page_t				first_m;
	vm_prot_t				prot;
	int						result;
	bool_t					wired;
	bool_t					su;
	bool_t					lookup_still_valid;
	bool_t					page_exists;
	vm_page_t				old_m;
	vm_object_t				next_object;

	struct vm_faultinfo 	vfi;
	vm_page_t 				pg;

	anon = NULL;
	pg = NULL;

	vfi.orig_map = map;
	vfi.orig_rvaddr = truc_page(vaddr);
	vfi.orig_size = PAGE_SIZE;

	result = vm_map_lookup(&map, vaddr, fault_type, &entry, &first_object, &first_offset, &prot, &wired, &su);
	if(result != KERN_SUCCESS) {
		return (result);
	}

	if(vm_fault_lookup(map, vaddr, FALSE, map->timestamp) == FALSE) {
		return (EFAULT);
	}

	lookup_still_valid = TRUE;

	if (wired) {
		fault_type = prot;
	}
	first_seg = NULL;
	first_m = NULL;

	vm_object_lock(first_object);

	first_object->ref_count++;
	first_object->paging_in_progress++;

	object = first_object;
	offset = first_offset;

	seg = vm_segment_lookup(object, offset);
	while (TRUE) {
		m = vm_page_lookup(seg, offset);
	}

	if (((object->pager != NULL) && (!change_wiring || wired))
			|| (object == first_object)) {
		m = vm_page_alloc(object, offset);
		if (m == NULL) {
			UNLOCK_AND_DEALLOCATE;
			VM_WAIT;
			goto RetryFault;
		}
	}
}

/*
 * uvmfault_unlockmaps: unlock the maps
 */
static __inline void
vm_fault_unlockmaps(map, write_locked)
	vm_map_t map;
	bool_t write_locked;
{
	/*
	 * ufi can be NULL when this isn't really a fault,
	 * but merely paging in anon data.
	 */
	if (map == NULL) {
		return;
	}
	if (write_locked) {
		vm_map_unlock(map);
	} else {
		vm_map_unlock_read(map);
	}
}

/*
 * uvmfault_unlockall: unlock everything passed in.
 *
 * => maps must be read-locked (not write-locked).
 */

static __inline void
vm_fault_unlockall(map, amap, obj, anon)
	vm_map_t map;
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
	vm_fault_unlockmaps(map, FALSE);
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
vm_fault_lookup(orig_map, orig_size, write_lock, timestamp)
	vm_map_t orig_map;
	vm_size_t orig_size;
	bool_t write_lock;
	unsigned int timestamp;
{
	vm_map_t map, tmpmap;
	vm_map_entry_t entry;
	vm_size_t size;

	map = orig_map;
	size = orig_size;

	/*
	 * keep going down levels until we are done.   note that there can
	 * only be two levels so we won't loop very long.
	 */
	while (1) {

		/*
		 * lock map
		 */
		if (write_lock) {
			vm_map_lock(map);
		} else {
			vm_map_lock_read(map);
		}

		/*
		 * lookup
		 */
		if (!vm_map_lookup_entry(map, orig_size, &entry)) {
			return (FALSE);
		}

		/*
		 * reduce size if necessary
		 */
		if(entry->end - orig_size < size) {
			size = entry->end - orig_size;
		}

		/*
		 * submap?    replace map with the submap and lookup again.
		 * note: VAs in submaps must match VAs in main map.
		 */
		if(entry->is_sub_map) {
			tmpmap = entry->object.sub_map;
			if (write_lock) {
				vm_map_lock(map);
			} else {
				vm_map_lock_read(map);
			}
			map = tmpmap;
			continue;
		}

		/*
		 * got it!
		 */
		timestamp = map->timestamp;
		return( TRUE);
	} /* while loop */

	/*NOTREACHED*/
}

/*
 * uvmfault_relock: attempt to relock the same version of the map
 *
 * => fault data structures should be unlocked before calling.
 * => if a success (TRUE) maps will be locked after call.
 */
static __inline bool_t
vm_fault_relock(map, timestamp)
	vm_map_t map;
	unsigned int timestamp;
{
	/*
	 * ufi can be NULL when this isn't really a fault,
	 * but merely paging in anon data.
	 */

	if (map == NULL) {
		return (TRUE);
	}

	/*
	 * relock map.   fail if version mismatch (in which case nothing
	 * gets locked).
	 */

	vm_map_lock_read(map);
	if (timestamp != map->timestamp) {
		vm_map_unlock_read(map);
		return (FALSE);
	}
	return (TRUE);
}
