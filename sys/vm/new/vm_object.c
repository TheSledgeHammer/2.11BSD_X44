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
 *	@(#)vm_object.c	8.7 (Berkeley) 5/11/95
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
 *	Virtual memory object module.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/fnv_hash.h>

#include <vm/include/vm.h>
#include <vm/include/vm_segment.h>
#include <vm/include/vm_page.h>

/*
 *	vm_object_segment_page_clean
 *
 *	Clean all dirty pages or dirty segments in the specified range of object.
 *	If syncio is TRUE, page cleaning is done synchronously.
 *	If de_queue is TRUE, pages are removed from any paging queue
 *	they were on, otherwise they are left on whatever queue they
 *	were on before the cleaning operation began.
 *
 *	Odd semantics: if start == end, we clean everything.
 *
 *	The object must be locked.
 *
 *	Returns TRUE if all was well, FALSE if there was a pager error
 *	somewhere.  We attempt to clean (and dequeue) all pages regardless
 *	of where an error occurs.
 */
bool_t
vm_object_segment_page_clean(object, start, end, syncio, de_queue)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
	bool_t					syncio, de_queue;
{
	register vm_segment_t	segment;
	register vm_page_t		page;
	int 					onqueue;
	bool_t 					noerror, ismod;

	noerror = TRUE;

	if (object == NULL) {
		return (TRUE);
	}

	/*
	 * If it is an internal object and there is no pager, attempt to
	 * allocate one.  Note that vm_object_collapse may relocate one
	 * from a collapsed object so we must recheck afterward.
	 */
	if ((object->flags & OBJ_INTERNAL) && object->pager == NULL) {
		vm_object_collapse(object);
		if (object->pager == NULL) {
			vm_pager_t pager;

			vm_object_unlock(object);
			pager = vm_pager_allocate(PG_DFLT, (caddr_t)0, object->size, VM_PROT_ALL, (vm_offset_t)0);
			if (pager) {
				vm_object_setpager(object, pager, 0, FALSE);
			}
			vm_object_lock(object);
		}
	}

	if (object->pager == NULL) {
		return (TRUE);
	}

again:

	/*
	 * Wait until the pageout daemon is through with the object.
	 */
	while (object->paging_in_progress) {
		vm_object_sleep(object, object, FALSE);
		vm_object_lock(object);
	}
	/*
	 * Loop through the object segment list cleaning as necessary.
	 */
	CIRCLEQ_FOREACH(segment, object->seglist, listq) {
		if ((start == end || (segment->offset >= start && segment->offset < end))) {
			/*
			 * Check if the segment page list is empty.
			 */
			if (!TAILQ_EMPTY(segment->memq)) {
				/*
				 * Loop through the segment page list cleaning as necessary.
				 */
				TAILQ_FOREACH(page, segment->memq, listq) {
					if (page->segment == segment && !(page->flags & PG_FICTITIOUS)) {
						ismod = pmap_is_modified(VM_PAGE_TO_PHYS(page));
						if ((page->flags & PG_CLEAN) && ismod) {
							page->flags &= ~PG_CLEAN;
						}
						/*
						 * Remove the page from any paging queue.
						 * This needs to be done if either we have been
						 * explicitly asked to do so or it is about to
						 * be cleaned (see comment below).
						 */
						if (de_queue || !(page->flags & PG_CLEAN)) {
							vm_page_lock_queues();
							if (page->flags & PG_ACTIVE) {
								TAILQ_REMOVE(&vm_page_queue_active, page, pageq);
								page->flags &= ~PG_ACTIVE;
								cnt.v_page_active_count--;
								onqueue = 1;
							} else if (page->flags & PG_INACTIVE) {
								TAILQ_REMOVE(&vm_page_queue_inactive, page, pageq);
								page->flags &= ~PG_INACTIVE;
								cnt.v_page_inactive_count--;
								onqueue = -1;
							} else {
								onqueue = 0;
							}
							vm_page_unlock_queues();
						}
						/*
						 * To ensure the state of the page doesn't change
						 * during the clean operation we do two things.
						 * First we set the busy bit and write-protect all
						 * mappings to ensure that write accesses to the
						 * page block (in vm_fault).  Second, we remove
						 * the page from any paging queue to foil the
						 * pageout daemon (vm_pageout_scan).
						 */
						pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_READ);
						if (!(page->flags & PG_CLEAN)) {
							page->flags |= PG_BUSY;
							object->paging_in_progress++;
							vm_object_unlock(object);
							/*
							 * XXX if put fails we mark the page as
							 * clean to avoid an infinite loop.
							 * Will loose changes to the page.
							 */
							if (vm_pager_put(object->pager, page, syncio)) {
								printf("%s: pager_put error\n vm_object_page_clean");
								page->flags |= PG_CLEAN;
								noerror = FALSE;
							}
							vm_object_lock(object);
							object->paging_in_progress--;
							if (!de_queue && onqueue) {
								vm_page_lock_queues();
								if (onqueue > 0) {
									vm_page_activate(page);
								} else {
									vm_page_deactivate(page);
								}
								vm_page_unlock_queues();
							}
							page->flags &= ~PG_BUSY;
							PAGE_WAKEUP(page);
							goto again;
						}
					}
				}
			} else {
				ismod = pmap_is_modified(VM_SEGMENT_TO_PHYS(segment));
				if ((segment->flags & SEG_CLEAN) && ismod) {
					segment->flags &= ~SEG_CLEAN;
				}
				/*
				 * Remove the segment from any paging queue.
				 * This needs to be done if either we have been
				 * explicitly asked to do so or it is about to
				 * be cleaned (see comment below).
				 */
				if (de_queue || !(segment->flags & SEG_CLEAN)) {
					vm_segment_lock_lists();
					if (segment->flags & SEG_ACTIVE) {
						CIRCLEQ_REMOVE(&vm_segment_list_active, segment, segmentq);
						segment->flags &= ~SEG_ACTIVE;
						cnt.v_segment_active_count--;
						onqueue = 1;
					} else if (segment->flags & SEG_INACTIVE) {
						CIRCLEQ_REMOVE(&vm_segment_list_inactive, segment, segmentq);
						segment->flags &= ~SEG_INACTIVE;
						cnt.v_segment_inactive_count--;
						onqueue = -1;
					} else {
						onqueue = 0;
					}
					vm_segment_unlock_lists();
				}

				/* protect entire segment & allocate new pages */
				pmap_page_protect(VM_SEGMENT_TO_PHYS(segment), VM_PROT_READ);
//				page = vm_page_alloc(segment, segment->sg_offset);
//				vm_page_insert(segment, page, page->offset);

				if (!(segment->flags & SEG_CLEAN)) {
					segment->flags |= SEG_BUSY;
					/*
					 * Pager access should not be required for segments.
					 * As empty pages are unmodified pages,
					 * which are usually clean. If pages aren't empty,
					 * they should have already been dealt with before getting here.
					 */
					if (!de_queue && onqueue) {
						vm_segment_lock_lists();
						if (onqueue > 0) {
							vm_segment_activate(segment);
						} else {
							vm_segment_deactivate(segment);
						}
						vm_segment_unlock_lists();
					}
					segment->flags &= ~SEG_BUSY;
					SEGMENT_WAKEUP(segment);
					goto again;
				}
			}
		}
	}
	return (noerror);
}

void
vm_object_segment_page_remove(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_segment_t 	segment;
	register vm_page_t		page;

	if (object == NULL) {
		return;
	}

	CIRCLEQ_FOREACH(segment, &object->seglist, listq) {
		if ((start <= segment->offset) && (segment->offset < end)) {
			if (!TAILQ_EMPTY(segment->memq)) {
				TAILQ_FOREACH(page, &segment->memq, listq) {
					if (page->segment == segment) {
						pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_NONE);
						vm_page_lock_queues();
						vm_page_free(page);
						vm_page_unlock_queues();
					}
				}
			} else {
				pmap_page_protect(VM_SEGMENT_TO_PHYS(segment), VM_PROT_NONE);
				vm_segment_lock_lists();
				vm_segment_free(segment);
				vm_segment_unlock_lists();
			}
		}
	}
}
