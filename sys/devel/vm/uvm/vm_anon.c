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
#include <vm/include/vm_kern.h>

#include <devel/vm/uvm/uvm.h>
#include <vm/include/vm.h>
//#include <vm/include/vm_swap.h>

/*
 * allocate anons
 */
void
vm_anon_init()
{
	struct vm_anon *anon;

	int nanon = cnt.v_free_count - (cnt.v_free_count / 16); /* XXXCDC ??? */
	int lcv;

	/*
	 * Allocate the initial anons.
	 */
	anon = (struct vm_anon *)kmem_alloc(kernel_map, sizeof(*anon) * nanon);
	if (anon == NULL) {
		printf("vm_anon_init: can not allocate %d anons\n", nanon);
		panic("vm_anon_init");
	}

	memset(anon, 0, sizeof(*anon) * nanon);
	anon->u.an_free = NULL;
	cnt.v_free_count = cnt.v_anon_free_count = nanon;
	for (lcv = 0 ; lcv < nanon ; lcv++) {
		anon[lcv].u.an_nxt = anon->u.an_free;
		anon->u.an_free = &anon[lcv];
	}
	simple_lock_init(&anon->u.an_freelock);
}

/*
 * add some more anons to the free pool.  called when we add
 * more swap space.
 */
void
vm_anon_add(pages)
	int	pages;
{
	struct vm_anon *anon;
	int lcv;

	anon = (struct vm_anon *)kmem_alloc(kernel_map, sizeof(*anon) * pages);

	/* XXX Should wait for VM to free up. */
	if (anon == NULL) {
		printf("vm_anon_add: can not allocate %d anons\n", pages);
		panic("vm_anon_add");
	}

	simple_lock(&anon->u.an_freelock);
	memset(anon, 0, sizeof(*anon) * pages);
	cnt.v_kernel_anons += pages;
	cnt.v_anon_free_count += pages;
	for (lcv = 0; lcv < pages; lcv++) {
		simple_lock_init(&anon->an_lock);
		anon[lcv].u.an_nxt = anon->u.an_free;
		anon->u.an_free = &anon[lcv];
	}
	simple_unlock(&anon->u.an_freelock);
}

/*
 * allocate an anon
 */
struct vm_anon *
vm_analloc()
{
	struct vm_anon *a;

	simple_lock(&a->u->an_freelock);
	a = a->u.an_free;
	if (a) {
		a->u.an_free = a->u.an_nxt;
		cnt.v_anon_free_count--;
		a->an_ref = 1;
		a->an_swslot = 0;
		a->u.an_page = NULL;		/* so we can free quickly */
	}
	simple_unlock(&a->u.an_freelock);
	return(a);
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
vm_anfree(anon)
	struct vm_anon *anon;
{
	struct vm_page *pg;

	/*
	 * get page
	 */

	pg = anon->u.an_page;

	/*
	 * if there is a resident page and it is loaned, then anon may not
	 * own it.   call out to uvm_anon_lockpage() to ensure the real owner
 	 * of the page has been identified and locked.
	 */

	if (pg && pg->loan_count)
		pg = vm_anon_lockloanpg(anon);

	/*
	 * if we have a resident page, we must dispose of it before freeing
	 * the anon.
	 */

	if (pg) {

		/*
		 * if the page is owned by a uobject (now locked), then we must
		 * kill the loan on the page rather than free it.
		 */

		if (pg->object) {

			/* kill loan */
			vm_page_lock_queues();
#ifdef DIAGNOSTIC
			if (pg->loan_count < 1)
				panic("vm_anfree: obj owned page "
				      "with no loan count");
#endif
			pg->loan_count--;
			pg->anon = NULL;
			vm_page_unlock_queues();
			simple_unlock(&pg->object->Lock);

		} else {

			/*
			 * page has no uobject, so we must be the owner of it.
			 *
			 * if page is busy then we just mark it as released
			 * (who ever has it busy must check for this when they
			 * wake up).    if the page is not busy then we can
			 * free it now.
			 */

			if ((pg->flags & PG_BUSY) != 0) {
				/* tell them to dump it when done */
				pg->flags |= PG_RELEASED;
				return;
			}

			pmap_page_protect(PMAP_PGARG(pg), VM_PROT_NONE);
			vm_page_lock_queues();		/* lock out pagedaemon */
			vm_page_free(pg);			/* bye bye */
			vm_page_unlock_queues();	/* free the daemon */
		}
	}

	/*
	 * free any swap resources.
	 */
	vm_anon_dropswap(anon);

	/*
	 * now that we've stripped the data areas from the anon, free the anon
	 * itself!
	 */
	simple_lock(&anon->u.an_freelock);
	anon->u.an_nxt = anon->u.an_free;
	anon->u.an_free = anon;
	cnt.v_anon_free_count++;
	simple_unlock(&anon->u.an_freelock);
}

/*
 * uvm_anon_dropswap:  release any swap resources from this anon.
 *
 * => anon must be locked or have a reference count of 0.
 */
void
vm_anon_dropswap(anon)
	struct vm_anon *anon;
{
	if (anon->an_swslot == 0) {
		return;
	}

	vm_swap_free(anon->an_swslot, 1);
	anon->an_swslot = 0;

	if (anon->u.an_page == NULL) {
		/* this page is no longer only in swap. */
		simple_lock(&uvm.swap_data_lock);
		vmexp.swpgonly--;
		simple_unlock(&uvm.swap_data_lock);
	}
}

/*
 * vm_anon_lockloanpg: given a locked anon, lock its resident page
 *
 * => anon is locked by caller
 * => on return: anon is locked
 *		 if there is a resident page:
 *			if it has a uobject, it is locked by us
 *			if it is ownerless, we take over as owner
 *		 we return the resident page (it can change during
 *		 this function)
 * => note that the only time an anon has an ownerless resident page
 *	is if the page was loaned from a uvm_object and the uvm_object
 *	disowned it
 * => this only needs to be called when you want to do an operation
 *	on an anon's resident page and that page has a non-zero loan
 *	count.
 */
struct vm_page *
vm_anon_lockloanpg(anon)
	struct vm_anon *anon;
{
	struct vm_page *pg;
	boolean_t locked = FALSE;

	/*
	 * loop while we have a resident page that has a non-zero loan count.
	 * if we successfully get our lock, we will "break" the loop.
	 * note that the test for pg->loan_count is not protected -- this
	 * may produce false positive results.   note that a false positive
	 * result may cause us to do more work than we need to, but it will
	 * not produce an incorrect result.
	 */

	while (((pg = anon->u.an_page) != NULL) && pg->loan_count != 0) {

		/*
		 * quickly check to see if the page has an object before
		 * bothering to lock the page queues.   this may also produce
		 * a false positive result, but that's ok because we do a real
		 * check after that.
		 *
		 * XXX: quick check -- worth it?   need volatile?
		 */

		if (pg->object) {

			vm_page_lock_queues();
			if (pg->object) {	/* the "real" check */
				locked =
				    simple_lock_try(&pg->object->Lock);
			} else {
				/* object disowned before we got PQ lock */
				locked = TRUE;
			}
			vm_page_unlock_queues();

			/*
			 * if we didn't get a lock (try lock failed), then we
			 * toggle our anon lock and try again
			 */

			if (!locked) {
				simple_unlock(&anon->an_lock);
				/*
				 * someone locking the object has a chance to
				 * lock us right now
				 */
				simple_lock(&anon->an_lock);
				continue;		/* start over */
			}
		}

		/*
		 * if page is un-owned [i.e. the object dropped its ownership],
		 * then we can take over as owner!
		 */

		if (pg->object == NULL && (pg->pqflags & PQ_ANON) == 0) {
			vm_page_lock_queues();
			pg->pqflags |= PQ_ANON;		/* take ownership... */
			pg->loan_count--;			/* ... and drop our loan */
			vm_page_unlock_queues();
		}

		/*
		 * we did it!   break the loop
		 */
		break;
	}

	/*
	 * done!
	 */

	return (pg);
}
