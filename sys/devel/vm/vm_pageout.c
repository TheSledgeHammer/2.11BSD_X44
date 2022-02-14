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
 *	@(#)vm_pageout.c	8.7 (Berkeley) 6/19/95
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
 *	The proverbial page-out daemon.
 */

/*
 * TODO: Areas to be improved.
 * Pageout Daemon Specific
 * - Page clustering with segments
 * - Better utilization of segments
 * - Anonymous memory management (anons)
 *
 * Pageout Daemon Related
 * - Pager: Most tie into the above.
 * - Clustering with segments (Mach/PureDarwin VM implements pager clustering)
 * - Per Segment Load Balancing of Pages
 * 	 - Improves page utilization and distribution
 */

#include <sys/param.h>

#include <devel/vm/include/vm_segment.h>
#include <devel/vm/include/vm_page.h>
#include <vm/include/vm_pageout.h>
#include <devel/vm/include/vm.h>

/*
 *	vm_pageout_scan does the dirty work for the pageout daemon.
 */
void
vm_pageout_scan()
{
	register vm_page_t	m, next;
	register int		page_shortage;
	register int		s;
	register int		pages_freed;
	int					free;
	vm_object_t			object;

}

/*
 * Called with object and page queues locked.
 * If reactivate is TRUE, a pager error causes the page to be
 * put back on the active queue, ow it is left on the inactive queue.
 */
void
vm_pageout_page(page, segment, object)
	vm_page_t 		page;
	vm_segment_t 	segment;
	vm_object_t 	object;
{
	vm_pager_t pager;
	int pageout_status;

	/*
	 * We set the busy bit to cause potential page faults on
	 * this page to block.
	 *
	 * We also set pageout-in-progress to keep the object from
	 * disappearing during pageout.  This guarantees that the
	 * page won't move from the inactive queue.  (However, any
	 * other page on the inactive queue may move!)
	 */
	pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_NONE);
	page->flags |= PG_BUSY;

	/*
	 * Try to collapse the object before making a pager for it.
	 * We must unlock the page queues first.
	 */
	vm_page_unlock_queues();
	vm_segment_unlock_lists();
	if (object->pager == NULL)
		vm_object_collapse(object);

	object->paging_in_progress++;
	vm_object_unlock(object);

	/*
	 * Do a wakeup here in case the following operations block.
	 */
	thread_wakeup(&cnt.v_free_count);

	/*
	 * If there is no pager for the page, use the default pager.
	 * If there is no place to put the page at the moment,
	 * leave it in the laundry and hope that there will be
	 * paging space later.
	 */
	if ((pager = object->pager) == NULL) {
		pager = vm_pager_allocate(PG_DFLT, (caddr_t)0, object->size, VM_PROT_ALL, (vm_offset_t)0);
		if (pager != NULL) {
			vm_object_setpager(object, pager, 0, FALSE);
		}
	}
	pageout_status = pager ? vm_pager_put(pager, page, FALSE) : VM_PAGER_FAIL;

	vm_object_lock(object);
	vm_segment_lock_lists();
	vm_page_lock_queues();

	switch (pageout_status) {
	case VM_PAGER_OK:
	case VM_PAGER_PEND:
		cnt.v_pgpgout++;
		page->flags &= ~PG_LAUNDRY;
		break;
	case VM_PAGER_BAD:
		/*
		 * Page outside of range of object.  Right now we
		 * essentially lose the changes by pretending it
		 * worked.
		 *
		 * XXX dubious, what should we do?
		 */
		page->flags &= ~PG_LAUNDRY;
		page->flags |= PG_CLEAN;
		pmap_clear_modify(VM_PAGE_TO_PHYS(page));
		break;
	case VM_PAGER_AGAIN: {
		extern int lbolt;

		/*
		 * FAIL on a write is interpreted to mean a resource
		 * shortage, so we put pause for awhile and try again.
		 * XXX could get stuck here.
		 */
		vm_page_unlock_queues();
		vm_segment_unlock_lists();
		vm_object_unlock(object);
		(void) tsleep((caddr_t) & lbolt, PZERO | PCATCH, "pageout", 0);
		vm_object_lock(object);
		vm_segment_lock_lists();
		vm_page_lock_queues();
		break;
	}
	case VM_PAGER_FAIL:
	case VM_PAGER_ERROR:
		/*
		 * If page couldn't be paged out, then reactivate
		 * the page so it doesn't clog the inactive list.
		 * (We will try paging out it again later).
		 */
		vm_page_activate(page);
		cnt.v_reactivated++;
		break;
	}

	pmap_clear_reference(VM_PAGE_TO_PHYS(page));

	/*
	 * If the operation is still going, leave the page busy
	 * to block all other accesses.  Also, leave the paging
	 * in progress indicator set so that we don't attempt an
	 * object collapse.
	 */
	if (pageout_status != VM_PAGER_PEND) {
		page->flags &= ~PG_BUSY;
		PAGE_WAKEUP(page);
		object->paging_in_progress--;
	}
}

#define PAGEOUTABLE(p) \
	((((p)->flags & (PG_INACTIVE|PG_CLEAN|PG_LAUNDRY)) == \
	  (PG_INACTIVE|PG_LAUNDRY)) && !pmap_is_referenced(VM_PAGE_TO_PHYS(p)))

/*
 * Attempt to pageout as many contiguous (to ``m'') dirty pages as possible
 * from ``object''.  Using information returned from the pager, we assemble
 * a sorted list of contiguous dirty pages and feed them to the pager in one
 * chunk.  Called with paging queues and object locked.  Also, object must
 * already have a pager.
 */
void
vm_pageout_cluster(page, segment, object)
	vm_page_t page;
	vm_segment_t segment;
	vm_object_t object;
{
	vm_offset_t offset, loff, hoff;
	vm_page_t plist[MAXPOCLUSTER], *plistp, p;

	/*
	 * Determine the range of pages that can be part of a cluster
	 * for this object/offset.  If it is only our single page, just
	 * do it normally.
	 */
	vm_pager_cluster(object->pager, page->offset, &loff, &hoff);
	if (hoff - loff == PAGE_SIZE) {
		vm_pageout_segment(page, segment, object);
		return;
	}

	plistp = plist;

	/*
	 * Target page is always part of the cluster.
	 */
	pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_NONE);
	page->flags |= PG_BUSY;
	plistp[atop(page->offset - loff)] = page;
	count = 1;

	/*
	 * Backup from the given page til we find one not fulfilling
	 * the pageout criteria or we hit the lower bound for the
	 * cluster.  For each page determined to be part of the
	 * cluster, unmap it and busy it out so it won't change.
	 */
	ix = atop(page->offset - loff);
	offset = page->offset;
	while (offset > loff && count < MAXPOCLUSTER - 1) {
		p = vm_page_lookup(segment, offset - PAGE_SIZE);
		if (p == NULL || !PAGEOUTABLE(p))
			break;
		pmap_page_protect(VM_PAGE_TO_PHYS(p), VM_PROT_NONE);
		p->flags |= PG_BUSY;
		plistp[--ix] = p;
		offset -= PAGE_SIZE;
		count++;
	}
	plistp += atop(offset - loff);
	loff = offset;

	/*
	 * Now do the same moving forward from the target.
	 */
	ix = atop(page->offset - loff) + 1;
	offset = page->offset + PAGE_SIZE;
	while (offset < hoff && count < MAXPOCLUSTER) {
		p = vm_page_lookup(segment, offset);
		if (p == NULL || !PAGEOUTABLE(p))
			break;
		pmap_page_protect(VM_PAGE_TO_PHYS(p), VM_PROT_NONE);
		p->flags |= PG_BUSY;
		plistp[ix++] = p;
		offset += PAGE_SIZE;
		count++;
	}
	hoff = offset;

	/*
	 * Pageout the page.
	 * Unlock everything and do a wakeup prior to the pager call
	 * in case it blocks.
	 */
	vm_page_unlock_queues();
	vm_segment_unlock_lists();
	object->paging_in_progress++;
	vm_object_unlock(object);
again:
	thread_wakeup(&cnt.v_free_count);
	postatus = vm_pager_put_pages(object->pager, plistp, count, FALSE);

	if (postatus == VM_PAGER_AGAIN) {
		extern int lbolt;

		(void) tsleep((caddr_t) & lbolt, PZERO | PCATCH, "pageout", 0);
		goto again;
	} else if (postatus == VM_PAGER_BAD)
		panic("vm_pageout_cluster: VM_PAGER_BAD");
	vm_object_lock(object);
	vm_page_lock_queues();

	/*
	 * Loop through the affected pages, reflecting the outcome of
	 * the operation.
	 */
	for (ix = 0; ix < count; ix++) {
		p = *plistp++;
		switch (postatus) {
		case VM_PAGER_OK:
		case VM_PAGER_PEND:
			cnt.v_pgpgout++;
			p->flags &= ~PG_LAUNDRY;
			break;
		case VM_PAGER_FAIL:
		case VM_PAGER_ERROR:
			/*
			 * Pageout failed, reactivate the target page so it
			 * doesn't clog the inactive list.  Other pages are
			 * left as they are.
			 */
			if (p == page) {
				vm_page_activate(p);
				cnt.v_reactivated++;
			}
			break;
		}
		pmap_clear_reference(VM_PAGE_TO_PHYS(p));
		/*
		 * If the operation is still going, leave the page busy
		 * to block all other accesses.
		 */
		if (postatus != VM_PAGER_PEND) {
			p->flags &= ~PG_BUSY;
			PAGE_WAKEUP(p);

		}
	}
	/*
	 * If the operation is still going, leave the paging in progress
	 * indicator set so that we don't attempt an object collapse.
	 */
	if (postatus != VM_PAGER_PEND)
		object->paging_in_progress--;
}
