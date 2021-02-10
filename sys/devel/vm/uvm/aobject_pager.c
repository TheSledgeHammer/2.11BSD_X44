/*	$NetBSD: uvm_aobj.c,v 1.18.2.1 1999/04/16 16:27:13 chs Exp $	*/

/*
 * Copyright (c) 1998 Chuck Silvers, Charles D. Cranor and
 *                    Washington University.
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
 * from: Id: uvm_aobj.c,v 1.1.2.5 1998/02/06 05:14:38 chs Exp
 */
/*
 * uvm_aobj.c: anonymous memory uvm_object pager
 *
 * author: Chuck Silvers <chuq@chuq.com>
 * started: Jan-1998
 *
 * - design mostly from Chuck Cranor
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_pager.h>

#include <devel/vm/uvm/uvm.h>
#include <devel/vm/uvm/vm_aobject.h>
#include <devel/sys/malloctypes.h>

/*
 * aobj_pager
 *
 * note that some functions (e.g. put) are handled elsewhere
 */

struct pagerops aobject_pager = {
		uao_init,		/* init */
		uao_alloc,		/* allocate */
		uao_dealloc,	/* disassociate */
		uao_get,		/* get */
		uao_put,		/* put */
		uao_has,		/* has */
		NULL,			/* cluster */
};

/*
 * pager functions
 */

/*
 * uao_init: set up aobj pager subsystem
 *
 * => called at boot time from uvm_pager_init()
 */
static void
uao_init()
{
	static int uao_initialized;

	if (uao_initialized)
		return;
	uao_initialized = TRUE;
}

/*
 * uao_reference: add a ref to an aobj
 *
 * => aobj must be unlocked (we will lock it)
 */
void
uao_reference(uobj)
	struct vm_object *uobj;
{
	/*
 	 * kernel_object already has plenty of references, leave it alone.
 	 */

	if (uobj->ref_count == VM_OBJ_KERN)
		return;

	simple_lock(&uobj->Lock);
	uobj->ref_count++;		/* bump! */
	simple_unlock(&uobj->Lock);
}

/*
 * uao_detach: drop a reference to an aobj
 *
 * => aobj must be unlocked, we will lock it
 */
void
uao_detach(uobj)
	struct vm_object *uobj;
{
	struct vm_aobject *aobj = (struct vm_aobject *)uobj;
	struct vm_page *pg;
	boolean_t busybody;

	/*
 	 * detaching from kernel_object is a noop.
 	 */
	if (uobj->ref_count == VM_OBJ_KERN)
		return;

	simple_lock(&uobj->Lock);

	uobj->ref_count--;					/* drop ref! */
	if (uobj->ref_count) {				/* still more refs? */
		simple_unlock(&uobj->Lock);
		return;
	}

	/*
 	 * remove the aobj from the global list.
 	 */
	simple_lock(&aobject_list_lock);
	LIST_REMOVE(aobj, u_list);
	simple_unlock(&aobject_list_lock);

	/*
 	 * free all the pages that aren't PG_BUSY, mark for release any that are.
 	 */

	busybody = FALSE;
	for (pg = uobj->memq.tqh_first ; pg != NULL ; pg = pg->listq.tqe_next) {

		if (pg->flags & PG_BUSY) {
			pg->flags |= PG_RELEASED;
			busybody = TRUE;
			continue;
		}

		/* zap the mappings, free the swap slot, free the page */
		pmap_page_protect(PMAP_PGARG(pg), VM_PROT_NONE);
		uao_dropswap(&aobj->u_obj, pg->offset >> PAGE_SHIFT);
		vm_page_lock_queues();
		vm_page_free(pg);
		vm_page_unlock_queues();
	}

	/*
 	 * if we found any busy pages, we're done for now.
 	 * mark the aobj for death, releasepg will finish up for us.
 	 */
	if (busybody) {
		aobj->u_flags |= UAO_FLAG_KILLME;
		simple_unlock(&aobj->u_obj.Lock);
		return;
	}

	/*
 	 * finally, free the rest.
 	 */
	uao_free(aobj);
}

/*
 * uao_flush: uh, yea, sure it's flushed.  really!
 */
boolean_t
uao_flush(uobj, start, end, flags)
	struct vm_object *uobj;
	vaddr_t start, end;
	int flags;
{

	/*
 	 * anonymous memory doesn't "flush"
 	 */
	/*
 	 * XXX
 	 * deal with PGO_DEACTIVATE (for madvise(MADV_SEQUENTIAL))
 	 * and PGO_FREE (for msync(MSINVALIDATE))
 	 */
	return TRUE;
}

/*
 * uao_get: fetch me a page
 *
 * we have three cases:
 * 1: page is resident     -> just return the page.
 * 2: page is zero-fill    -> allocate a new page and zero it.
 * 3: page is swapped out  -> fetch the page from swap.
 *
 * cases 1 and 2 can be handled with PGO_LOCKED, case 3 cannot.
 * so, if the "center" page hits case 3 (or any page, with PGO_ALLPAGES),
 * then we will need to return VM_PAGER_UNLOCK.
 *
 * => prefer map unlocked (not required)
 * => object must be locked!  we will _unlock_ it before starting any I/O.
 * => flags: PGO_ALLPAGES: get all of the pages
 *           PGO_LOCKED: fault data structures are locked
 * => NOTE: offset is the offset of pps[0], _NOT_ pps[centeridx]
 * => NOTE: caller must check for released pages!!
 */
static int
uao_get(uobj, offset, pps, npagesp, centeridx, access_type, advice, flags)
	struct vm_object *uobj;
	vaddr_t offset;
	struct vm_page **pps;
	int *npagesp;
	int centeridx, advice, flags;
	vm_prot_t access_type;
{
	struct vm_aobject *aobj = (struct vm_aobject *)uobj;
	vaddr_t current_offset;
	vm_page_t ptmp;
	int lcv, gotpages, maxpages, swslot, rv;
	boolean_t done;

	/*
 	 * get number of pages
 	 */

	maxpages = *npagesp;

	/*
 	 * step 1: handled the case where fault data structures are locked.
 	 */

	if (flags & PGO_LOCKED) {

		/*
 		 * step 1a: get pages that are already resident.   only do
		 * this if the data structures are locked (i.e. the first
		 * time through).
 		 */

		done = TRUE;	/* be optimistic */
		gotpages = 0;	/* # of pages we got so far */

		for (lcv = 0, current_offset = offset ; lcv < maxpages ;
		    lcv++, current_offset += PAGE_SIZE) {
			/* do we care about this page?  if not, skip it */
			if (pps[lcv] == PGO_DONTCARE)
				continue;

			ptmp = vm_page_lookup(uobj, current_offset);

			/*
 			 * if page is new, attempt to allocate the page, then
			 * zero-fill it.
 			 */
			if (ptmp == NULL && uao_find_swslot(aobj,
			    current_offset >> PAGE_SHIFT) == 0) {
				ptmp = vm_page_alloc(uobj, current_offset, NULL, 0);
				if (ptmp) {
					/* new page */
					ptmp->flags &= ~(PG_BUSY|PG_FAKE);
					ptmp->pqflags |= PQ_AOBJ;
					//UVM_PAGE_OWN(ptmp, NULL);
					vm_pagezero(ptmp);
				}
			}

			/*
			 * to be useful must get a non-busy, non-released page
			 */
			if (ptmp == NULL ||
			    (ptmp->flags & (PG_BUSY|PG_RELEASED)) != 0) {
				if (lcv == centeridx ||
				    (flags & PGO_ALLPAGES) != 0)
					/* need to do a wait or I/O! */
					done = FALSE;
					continue;
			}

			/*
			 * useful page: busy/lock it and plug it in our
			 * result array
			 */
			/* caller must un-busy this page */
			ptmp->flags |= PG_BUSY;
			//UVM_PAGE_OWN(ptmp, "uao_get1");
			pps[lcv] = ptmp;
			gotpages++;

		}	/* "for" lcv loop */

		/*
 		 * step 1b: now we've either done everything needed or we
		 * to unlock and do some waiting or I/O.
 		 */

		*npagesp = gotpages;
		if (done)
			/* bingo! */
			return (VM_PAGER_OK);
		else
			/* EEK!   Need to unlock and I/O */
			return (VM_PAGER_UNLOCK);
	}

	/*
 	 * step 2: get non-resident or busy pages.
 	 * object is locked.   data structures are unlocked.
 	 */

	for (lcv = 0, current_offset = offset ; lcv < maxpages ;
	    lcv++, current_offset += PAGE_SIZE) {
		/*
		 * - skip over pages we've already gotten or don't want
		 * - skip over pages we don't _have_ to get
		 */
		if (pps[lcv] != NULL ||
		    (lcv != centeridx && (flags & PGO_ALLPAGES) == 0))
			continue;

		/*
 		 * we have yet to locate the current page (pps[lcv]).   we
		 * first look for a page that is already at the current offset.
		 * if we find a page, we check to see if it is busy or
		 * released.  if that is the case, then we sleep on the page
		 * until it is no longer busy or released and repeat the lookup.
		 * if the page we found is neither busy nor released, then we
		 * busy it (so we own it) and plug it into pps[lcv].   this
		 * 'break's the following while loop and indicates we are
		 * ready to move on to the next page in the "lcv" loop above.
 		 *
 		 * if we exit the while loop with pps[lcv] still set to NULL,
		 * then it means that we allocated a new busy/fake/clean page
		 * ptmp in the object and we need to do I/O to fill in the data.
 		 */

		/* top of "pps" while loop */
		while (pps[lcv] == NULL) {
			/* look for a resident page */
			ptmp = vm_page_lookup(uobj, current_offset);

			/* not resident?   allocate one now (if we can) */
			if (ptmp == NULL) {

				ptmp = vm_page_alloc(uobj, current_offset, NULL, 0);

				/* out of RAM? */
				if (ptmp == NULL) {
					simple_unlock(&uobj->Lock);
					vm_wait("uao_getpage");
					simple_lock(&uobj->Lock);
					/* goto top of pps while loop */
					continue;
				}

				/*
				 * safe with PQ's unlocked: because we just
				 * alloc'd the page
				 */
				ptmp->pqflags |= PQ_AOBJ;

				/*
				 * got new page ready for I/O.  break pps while
				 * loop.  pps[lcv] is still NULL.
				 */
				break;
			}

			/* page is there, see if we need to wait on it */
			if ((ptmp->flags & (PG_BUSY|PG_RELEASED)) != 0) {
				ptmp->flags |= PG_WANTED;
				//UVM_UNLOCK_AND_WAIT(ptmp, &uobj->Lock, 0, "uao_get", 0);
				simple_lock(&uobj->Lock);
				continue;	/* goto top of pps while loop */
			}

			/*
 			 * if we get here then the page has become resident and
			 * unbusy between steps 1 and 2.  we busy it now (so we
			 * own it) and set pps[lcv] (so that we exit the while
			 * loop).
 			 */
			/* we own it, caller must un-busy */
			ptmp->flags |= PG_BUSY;
			//UVM_PAGE_OWN(ptmp, "uao_get2");
			pps[lcv] = ptmp;
		}

		/*
 		 * if we own the valid page at the correct offset, pps[lcv] will
 		 * point to it.   nothing more to do except go to the next page.
 		 */
		if (pps[lcv])
			continue;			/* next lcv */

		/*
 		 * we have a "fake/busy/clean" page that we just allocated.
 		 * do the needed "i/o", either reading from swap or zeroing.
 		 */
		swslot = uao_find_swslot(aobj, current_offset >> PAGE_SHIFT);

		/*
 		 * just zero the page if there's nothing in swap.
 		 */
		if (swslot == 0) {
			/*
			 * page hasn't existed before, just zero it.
			 */
			vm_page_zero(ptmp);
		} else {

			/*
			 * page in the swapped-out page.
			 * unlock object for i/o, relock when done.
			 */
			simple_unlock(&uobj->Lock);
			rv = uvm_swap_get(ptmp, swslot, PGO_SYNCIO);
			simple_lock(&uobj->Lock);

			/*
			 * I/O done.  check for errors.
			 */
			if (rv != VM_PAGER_OK) {
				if (ptmp->flags & PG_WANTED)
					/* object lock still held */
					thread_wakeup(ptmp);
				ptmp->flags &= ~(PG_WANTED|PG_BUSY);
				//UVM_PAGE_OWN(ptmp, NULL);
				uvm_lock_pageq();
				vm_page_free(ptmp);
				uvm_unlock_pageq();
				simple_unlock(&uobj->Lock);
				return (rv);
			}
		}

		/*
 		 * we got the page!   clear the fake flag (indicates valid
		 * data now in page) and plug into our result array.   note
		 * that page is still busy.
 		 *
 		 * it is the callers job to:
 		 * => check if the page is released
 		 * => unbusy the page
 		 * => activate the page
 		 */

		ptmp->flags &= ~PG_FAKE;		/* data is valid ... */
		pmap_clear_modify(PMAP_PGARG(ptmp));	/* ... and clean */
		pps[lcv] = ptmp;

	}	/* lcv loop */

	/*
 	 * finally, unlock object and return.
 	 */

	simple_unlock(&uobj->Lock);
	return(VM_PAGER_OK);
}

/*
 * uao_releasepg: handle released page in an aobj
 *
 * => "pg" is a PG_BUSY [caller owns it], PG_RELEASED page that we need
 *      to dispose of.
 * => caller must handle PG_WANTED case
 * => called with page's object locked, pageq's unlocked
 * => returns TRUE if page's object is still alive, FALSE if we
 *      killed the page's object.    if we return TRUE, then we
 *      return with the object locked.
 * => if (nextpgp != NULL) => we return pageq.tqe_next here, and return
 *                              with the page queues locked [for pagedaemon]
 * => if (nextpgp == NULL) => we return with page queues unlocked [normal case]
 * => we kill the aobj if it is not referenced and we are suppose to
 *      kill it ("KILLME").
 */

static boolean_t
uao_releasepg(pg, nextpgp)
	struct vm_page *pg;
	struct vm_page **nextpgp;	/* OUT */
{
	struct vm_aobject *aobj = (struct vm_aobject *) pg->object;

#ifdef DIAGNOSTIC
	if ((pg->flags & PG_RELEASED) == 0)
		panic("uao_releasepg: page not released!");
#endif

	/*
 	 * dispose of the page [caller handles PG_WANTED] and swap slot.
 	 */
	pmap_page_protect(PMAP_PGARG(pg), VM_PROT_NONE);
	uao_dropswap(&aobj->u_obj, pg->offset >> PAGE_SHIFT);
	vm_page_lock_queues();
	if (nextpgp)
		*nextpgp = pg->pageq.tqe_next; /* next page for daemon */
	uvm_pagefree(pg);
	if (!nextpgp)
		vm_page_unlock_queues(); /* keep locked for daemon */

	/*
	 * if we're not killing the object, we're done.
	 */
	if ((aobj->u_flags & UAO_FLAG_KILLME) == 0)
		return TRUE;

#ifdef DIAGNOSTIC
	if (aobj->u_obj.ref_count)
		panic("vm_km_releasepg: kill flag set on referenced object!");
#endif

	/*
	 * if there are still pages in the object, we're done for now.
	 */
	if (aobj->u_obj.resident_page_count != 0)
		return TRUE;

#ifdef DIAGNOSTIC
	if (aobj->u_obj.memq.tqh_first)
		panic("vn_releasepg: pages in object with npages == 0");
#endif

	/*
	 * finally, free the rest.
	 */
	uao_free(aobj);

	return FALSE;
}

/*
 * uao_dropswap:  release any swap resources from this aobj page.
 *
 * => aobj must be locked or have a reference count of 0.
 */

void
uao_dropswap(uobj, pageidx)
	struct vm_object *uobj;
	int pageidx;
{
	int slot;

	slot = uao_set_swslot(uobj, pageidx, 0);
	if (slot) {
		vm_swap_free(slot, 1);
	}
}
