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

/*
 * an aobj manages anonymous-memory backed uvm_objects.   in addition
 * to keeping the list of resident pages, it also keeps a list of
 * allocated swap blocks.  depending on the size of the aobj this list
 * of allocated swap blocks is either stored in an array (small objects)
 * or in a hash table (large objects).
 */

/*
 * local structures
 */

/*
 * for hash tables, we break the address space of the aobj into blocks
 * of UAO_SWHASH_CLUSTER_SIZE pages.   we require the cluster size to
 * be a power of two.
 */

#define UAO_SWHASH_CLUSTER_SHIFT 4
#define UAO_SWHASH_CLUSTER_SIZE (1 << UAO_SWHASH_CLUSTER_SHIFT)

/* get the "tag" for this page index */
#define UAO_SWHASH_ELT_TAG(PAGEIDX) 							\
	((PAGEIDX) >> UAO_SWHASH_CLUSTER_SHIFT)

/* given an ELT and a page index, find the swap slot */
#define UAO_SWHASH_ELT_PAGESLOT(ELT, PAGEIDX) 					\
	((ELT)->slots[(PAGEIDX) & (UAO_SWHASH_CLUSTER_SIZE - 1)])

/* given an ELT, return its pageidx base */
#define UAO_SWHASH_ELT_PAGEIDX_BASE(ELT) 						\
	((ELT)->tag << UAO_SWHASH_CLUSTER_SHIFT)

/*
 * the swhash hash function
 */
#define UAO_SWHASH_HASH(AOBJ, PAGEIDX) 							\
	(&(AOBJ)->u_swhash[(((PAGEIDX) >> UAO_SWHASH_CLUSTER_SHIFT) \
			& (AOBJ)->u_swhashmask)])

/*
 * the swhash threshhold determines if we will use an array or a
 * hash table to store the list of allocated swap blocks.
 */

#define UAO_SWHASH_THRESHOLD (UAO_SWHASH_CLUSTER_SIZE * 4)
#define UAO_USES_SWHASH(AOBJ) 									\
	((AOBJ)->u_pages > UAO_SWHASH_THRESHOLD)	/* use hash? */

/*
 * the number of buckets in a swhash, with an upper bound
 */
#define UAO_SWHASH_MAXBUCKETS 256
#define UAO_SWHASH_BUCKETS(AOBJ) 								\
	(min((AOBJ)->u_pages >> UAO_SWHASH_CLUSTER_SHIFT, 			\
			UAO_SWHASH_MAXBUCKETS))

/*
 * uao_swhash_elt: when a hash table is being used, this structure defines
 * the format of an entry in the bucket list.
 */

struct uao_swhash_elt {
	LIST_ENTRY(uao_swhash_elt) 	list;							/* the hash list */
	vaddr_t 					tag;							/* our 'tag' */
	int 						count;							/* our number of active slots */
	int 						slots[UAO_SWHASH_CLUSTER_SIZE];	/* the slots */
};

 /*
  * uao_swhash: the swap hash table structure
  */

 LIST_HEAD(uao_swhash, uao_swhash_elt);

/*
 * aobj_pager
 *
 * note that some functions (e.g. put) are handled elsewhere
 */

struct pagerops aobject_pager = {
		uao_init,		/* init */
		uao_reference,	/* reference */
		uao_detach,		/* detach */
		NULL,			/* fault */
		uao_flush,		/* flush */
		uao_get,		/* get */
		NULL,			/* asyncget */
		NULL,			/* put (done by pagedaemon) */
		NULL,			/* cluster */
		NULL,			/* mk_pcluster */
		vm_shareprot,	/* shareprot */
		NULL,			/* aiodone */
		uao_releasepg	/* releasepg */
};

/*
 * uao_list: global list of active aobjs, locked by uao_list_lock
 */

static LIST_HEAD(aobjlist, vm_aobject) 	uao_list;
static simple_lock_data_t 				uao_list_lock;

/*
 * functions
 */

/*
 * hash table/array related functions
 */

/*
 * uao_find_swhash_elt: find (or create) a hash table entry for a page
 * offset.
 *
 * => the object should be locked by the caller
 */

static struct uao_swhash_elt *
uao_find_swhash_elt(aobj, pageidx, create)
	struct vm_aobject *aobj;
	int pageidx;
	boolean_t create;
{
	struct uao_swhash *swhash;
	struct uao_swhash_elt *elt;
	int page_tag;

	swhash = UAO_SWHASH_HASH(aobj, pageidx); /* first hash to get bucket */
	page_tag = UAO_SWHASH_ELT_TAG(pageidx); /* tag to search for */

	/*
	 * now search the bucket for the requested tag
	 */
	for (elt = swhash->lh_first; elt != NULL; elt = elt->list.le_next) {
		if (elt->tag == page_tag)
			return (elt);
	}

	/* fail now if we are not allowed to create a new entry in the bucket */
	if (!create)
		return NULL;

	/*
	 * allocate a new entry for the bucket and init/insert it in
	 */
	//elt = (struct uao_swhash_elt *)rmalloc(elt, sizeof(struct uao_swhash_elt *));
	LIST_INSERT_HEAD(swhash, elt, list);
	elt->tag = page_tag;
	elt->count = 0;
	memset(elt->slots, 0, sizeof(elt->slots));

	return (elt);
}

/*
 * uao_find_swslot: find the swap slot number for an aobj/pageidx
 *
 * => object must be locked by caller
 */
__inline static int
uao_find_swslot(aobj, pageidx)
	struct vm_aobject *aobj;
	int pageidx;
{

	/*
	 * if noswap flag is set, then we never return a slot
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP)
		return (0);

	/*
	 * if hashing, look in hash table.
	 */

	if (UAO_USES_SWHASH(aobj)) {
		struct uao_swhash_elt *elt = uao_find_swhash_elt(aobj, pageidx, FALSE);

		if (elt)
			return (UAO_SWHASH_ELT_PAGESLOT(elt, pageidx));
		else
			return (NULL);
	}

	/*
	 * otherwise, look in the array
	 */
	return (aobj->u_swslots[pageidx]);
}

/*
 * uao_set_swslot: set the swap slot for a page in an aobj.
 *
 * => setting a slot to zero frees the slot
 * => object must be locked by caller
 */
int
uao_set_swslot(uobj, pageidx, slot)
	struct vm_object *uobj;
	int pageidx, slot;
{
	struct vm_aobject *aobj = (struct vm_aobject *)uobj;
	int oldslot;

	/*
	 * if noswap flag is set, then we can't set a slot
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP) {

		if (slot == 0)
			return (0); /* a clear is ok */

		/* but a set is not */
		printf("uao_set_swslot: uobj = %p\n", uobj);
		panic("uao_set_swslot: attempt to set a slot on a NOSWAP object");
	}

	/*
	 * are we using a hash table?  if so, add it in the hash.
	 */

	if (UAO_USES_SWHASH(aobj)) {
		/*
		 * Avoid allocating an entry just to free it again if
		 * the page had not swap slot in the first place, and
		 * we are freeing.
		 */
		struct uao_swhash_elt *elt = uao_find_swhash_elt(aobj, pageidx,
				slot ? TRUE : FALSE);
		if (elt == NULL) {
#ifdef DIAGNOSTIC
			if (slot)
				panic("uao_set_swslot: didn't create elt");
#endif
			return (0);
		}

		oldslot = UAO_SWHASH_ELT_PAGESLOT(elt, pageidx);
		UAO_SWHASH_ELT_PAGESLOT(elt, pageidx) = slot;

		/*
		 * now adjust the elt's reference counter and free it if we've
		 * dropped it to zero.
		 */

		/* an allocation? */
		if (slot) {
			if (oldslot == 0)
				elt->count++;
		} else { /* freeing slot ... */
			if (oldslot) /* to be safe */
				elt->count--;

			if (elt->count == 0) {
				LIST_REMOVE(elt, list);
				pool_put(&uao_swhash_elt_pool, elt);
			}
		}

	} else {
		/* we are using an array */
		oldslot = aobj->u_swslots[pageidx];
		aobj->u_swslots[pageidx] = slot;
	}
	return (oldslot);
}

/*
 * end of hash/array functions
 */

/*
 * uao_free: free all resources held by an aobj, and then free the aobj
 *
 * => the aobj should be dead
 */
static void
uao_free(aobj)
	struct vm_aobject *aobj;
{

	if (UAO_USES_SWHASH(aobj)) {
		int i, hashbuckets = aobj->u_swhashmask + 1;

		/*
		 * free the swslots from each hash bucket,
		 * then the hash bucket, and finally the hash table itself.
		 */
		for (i = 0; i < hashbuckets; i++) {
			struct uao_swhash_elt *elt, *next;

			for (elt = aobj->u_swhash[i].lh_first; elt != NULL; elt = next) {
				int j;

				for (j = 0; j < UAO_SWHASH_CLUSTER_SIZE; j++)
				{
					int slot = elt->slots[j];

					if (slot) {
						vm_swap_free(slot, 1);

						/*
						 * this page is no longer
						 * only in swap.
						 */
						simple_lock(&uvm.swap_data_lock);
						uvmexp.swpgonly--;
						simple_unlock(&uvm.swap_data_lock);
					}
				}

				next = elt->list.le_next;
				pool_put(&uao_swhash_elt_pool, elt);
			}
		}
		FREE(aobj->u_swhash, M_VMAOBJ);
	} else {
		int i;

		/*
		 * free the array
		 */

		for (i = 0; i < aobj->u_pages; i++)
		{
			int slot = aobj->u_swslots[i];

			if (slot) {
				uvm_swap_free(slot, 1);

				/* this page is no longer only in swap. */
				simple_lock(&uvm.swap_data_lock);
				uvmexp.swpgonly--;
				simple_unlock(&uvm.swap_data_lock);
			}
		}
		FREE(aobj->u_swslots, M_VMAOBJ);
	}

	/*
	 * finally free the aobj itself
	 */
	pool_put(&uvm_aobj_pool, aobj);
}

/*
 * pager functions
 */

/*
 * uao_create: create an aobj of the given size and return its uvm_object.
 *
 * => for normal use, flags are always zero
 * => for the kernel object, the flags are:
 *	UAO_FLAG_KERNOBJ - allocate the kernel object (can only happen once)
 *	UAO_FLAG_KERNSWAP - enable swapping of kernel object ("           ")
 */
struct vm_object *
uao_create(size, flags)
	vm_size_t size;
	int flags;
{
	static struct vm_aobject kernel_object_store;	/* home of kernel_object */
	static int kobj_alloced = 0;					/* not allocated yet */
	int pages = round_page(size) >> PAGE_SHIFT;
	struct vm_aobject *aobj;

	/*
 	* malloc a new aobj unless we are asked for the kernel object
 	*/
	if (flags & UAO_FLAG_KERNOBJ) {		/* want kernel object? */
		if (kobj_alloced)
			panic("uao_create: kernel object already allocated");

		/*
		 * XXXTHORPEJ: Need to call this now, so the pool gets
		 * initialized!
		 */
		uao_init();

		aobj = &kernel_object_store;
		aobj->u_pages = pages;
		aobj->u_flags = UAO_FLAG_NOSWAP;	/* no swap to start */
		/* we are special, we never die */
		aobj->u_obj.ref_count = VM_OBJ_KERN;
		kobj_alloced = UAO_FLAG_KERNOBJ;
	} else if (flags & UAO_FLAG_KERNSWAP) {
		aobj = &kernel_object_store;
		if (kobj_alloced != UAO_FLAG_KERNOBJ)
		    panic("uao_create: asked to enable swap on kernel object");
		kobj_alloced = UAO_FLAG_KERNSWAP;
	} else {	/* normal object */
		aobj = pool_get(&uvm_aobj_pool, PR_WAITOK);
		aobj->u_pages = pages;
		aobj->u_flags = 0;		/* normal object */
		aobj->u_obj.ref_count = 1;	/* start with 1 reference */
	}

	/*
 	 * allocate hash/array if necessary
 	 *
 	 * note: in the KERNSWAP case no need to worry about locking since
 	 * we are still booting we should be the only thread around.
 	 */
	if (flags == 0 || (flags & UAO_FLAG_KERNSWAP) != 0) {
		int mflags = (flags & UAO_FLAG_KERNSWAP) != 0 ?
		    M_NOWAIT : M_WAITOK;

		/* allocate hash table or array depending on object size */
			if (UAO_USES_SWHASH(aobj)) {
			aobj->u_swhash = hashinit(UAO_SWHASH_BUCKETS(aobj), M_VMAOBJ, mflags, &aobj->u_swhashmask);
			if (aobj->u_swhash == NULL)
				panic("uao_create: hashinit swhash failed");
		} else {
			MALLOC(aobj->u_swslots, int *, pages * sizeof(int), M_VMAOBJ, mflags);
			if (aobj->u_swslots == NULL)
				panic("uao_create: malloc swslots failed");
			memset(aobj->u_swslots, 0, pages * sizeof(int));
		}

		if (flags) {
			aobj->u_flags &= ~UAO_FLAG_NOSWAP; /* clear noswap */
			return(&aobj->u_obj);
			/* done! */
		}
	}

	/*
 	 * init aobj fields
 	 */
	simple_lock_init(&aobj->u_obj.Lock);
	aobj->u_obj.pager->pg_ops = &aobject_pager;
	TAILQ_INIT(&aobj->u_obj.memq);
	aobj->u_obj.resident_page_count = 0;

	/*
 	 * now that aobj is ready, add it to the global list
 	 * XXXCHS: uao_init hasn't been called'd in the KERNOBJ case,
	 * do we really need the kernel object on this list anyway?
 	 */
	simple_lock(&uao_list_lock);
	LIST_INSERT_HEAD(&uao_list, aobj, u_list);
	simple_unlock(&uao_list_lock);

	/*
 	 * done!
 	 */
	return (&aobj->u_obj);
}

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

	LIST_INIT(&uao_list);
	simple_lock_init(&uao_list_lock);
	struct uao_swhash_elt *elt;
	 struct vm_aobject *aobj;
	/*
	 * NOTE: Pages fror this pool must not come from a pageable
	 * kernel map!
	 */
	MALLOC(elt, struct uao_swhash_elt *, sizeof(struct uao_swhash_elt *), M_VMAOBJ, M_WAITOK);
	MALLOC(aobj, struct vm_aobject *, sizeof(struct vm_aobject *), M_VMAOBJ, M_WAITOK);
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
	simple_lock(&uao_list_lock);
	LIST_REMOVE(aobj, u_list);
	simple_unlock(&uao_list_lock);

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
		uvm_swap_free(slot, 1);
	}
}
