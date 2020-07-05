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
#include <sys/user.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_page.h>
#include <devel/vm/include/vm_kern.h>
#include <devel/vm/include/vm_aobject.h>

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
	struct uvm_aobj *aobj;
	int pageidx;
	boolean_t create;
{
	struct uao_swhash *swhash;
	struct uao_swhash_elt *elt;
	vm_offset_t page_tag;

	swhash = UAO_SWHASH_HASH(aobj, pageidx);
	page_tag = UAO_SWHASH_ELT_TAG(pageidx);

	/*
	 * now search the bucket for the requested tag
	 */

	LIST_FOREACH(elt, swhash, list) {
		if (elt->tag == page_tag) {
			return elt;
		}
	}
	if (!create) {
		return NULL;
	}

	/*
	 * allocate a new entry for the bucket and init/insert it in
	 */

	elt = pool_get(&uao_swhash_elt_pool, PR_NOWAIT);
	if (elt == NULL) {
		return NULL;
	}
	LIST_INSERT_HEAD(swhash, elt, list);
	elt->tag = page_tag;
	elt->count = 0;
	memset(elt->slots, 0, sizeof(elt->slots));
	return elt;
}

/*
 * uao_find_swslot: find the swap slot number for an aobj/pageidx
 *
 * => object must be locked by caller
 */

int
uao_find_swslot(uobj, pageidx)
	struct uvm_object *uobj;
	int pageidx;
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct uao_swhash_elt *elt;

	/*
	 * if noswap flag is set, then we never return a slot
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP)
		return(0);

	/*
	 * if hashing, look in hash table.
	 */

	if (UAO_USES_SWHASH(aobj)) {
		elt = uao_find_swhash_elt(aobj, pageidx, FALSE);
		if (elt)
			return(UAO_SWHASH_ELT_PAGESLOT(elt, pageidx));
		else
			return(0);
	}

	/*
	 * otherwise, look in the array
	 */

	return(aobj->u_swslots[pageidx]);
}

/*
 * uao_set_swslot: set the swap slot for a page in an aobj.
 *
 * => setting a slot to zero frees the slot
 * => object must be locked by caller
 * => we return the old slot number, or -1 if we failed to allocate
 *    memory to record the new slot number
 */

int
uao_set_swslot(uobj, pageidx, slot)
	struct uvm_object *uobj;
	int pageidx, slot;
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct uao_swhash_elt *elt;
	int oldslot;
	//UVMHIST_FUNC("uao_set_swslot"); UVMHIST_CALLED(pdhist);
	//UVMHIST_LOG(pdhist, "aobj %p pageidx %d slot %d", aobj, pageidx, slot, 0);

	/*
	 * if noswap flag is set, then we can't set a non-zero slot.
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP) {
		if (slot == 0)
			return(0);

		printf("uao_set_swslot: uobj = %p\n", uobj);
		panic("uao_set_swslot: NOSWAP object");
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

		elt = uao_find_swhash_elt(aobj, pageidx, slot != 0);
		if (elt == NULL) {
			return slot ? -1 : 0;
		}

		oldslot = UAO_SWHASH_ELT_PAGESLOT(elt, pageidx);
		UAO_SWHASH_ELT_PAGESLOT(elt, pageidx) = slot;

		/*
		 * now adjust the elt's reference counter and free it if we've
		 * dropped it to zero.
		 */

		if (slot) {
			if (oldslot == 0)
				elt->count++;
		} else {
			if (oldslot)
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
	struct uvm_aobj *aobj;
{
	int swpgonlydelta = 0;

	simple_unlock(&aobj->u_obj.vmobjlock);
	if (UAO_USES_SWHASH(aobj)) {
		int i, hashbuckets = aobj->u_swhashmask + 1;

		/*
		 * free the swslots from each hash bucket,
		 * then the hash bucket, and finally the hash table itself.
		 */

		for (i = 0; i < hashbuckets; i++) {
			struct uao_swhash_elt *elt, *next;

			for (elt = LIST_FIRST(&aobj->u_swhash[i]);
			     elt != NULL;
			     elt = next) {
				int j;

				for (j = 0; j < UAO_SWHASH_CLUSTER_SIZE; j++) {
					int slot = elt->slots[j];

					if (slot > 0) {
						uvm_swap_free(slot, 1);
						swpgonlydelta++;
					}
				}

				next = LIST_NEXT(elt, list);
				pool_put(&uao_swhash_elt_pool, elt);
			}
		}
		free(aobj->u_swhash, M_UVMAOBJ);
	} else {
		int i;

		/*
		 * free the array
		 */

		for (i = 0; i < aobj->u_pages; i++) {
			int slot = aobj->u_swslots[i];

			if (slot > 0) {
				uvm_swap_free(slot, 1);
				swpgonlydelta++;
			}
		}
		free(aobj->u_swslots, M_UVMAOBJ);
	}

	/*
	 * finally free the aobj itself
	 */

	pool_put(&uvm_aobj_pool, aobj);

	/*
	 * adjust the counter of pages only in swap for all
	 * the swap slots we've freed.
	 */

	if (swpgonlydelta > 0) {
		simple_lock(&vm.swap_data_lock);
		KASSERT(uvmexp.swpgonly >= swpgonlydelta);
		vmexp.swpgonly -= swpgonlydelta;
		simple_unlock(&vm.swap_data_lock);
	}
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
	static struct vm_aobject kernel_object_store;
	static int kobj_alloced = 0;
	int pages = round_page(size) >> PAGE_SHIFT;
	struct uvm_aobj *aobj;

	/*
	 * malloc a new aobj unless we are asked for the kernel object
	 */

	if (flags & UAO_FLAG_KERNOBJ) {
		KASSERT(!kobj_alloced);
		aobj = &kernel_object_store;
		aobj->u_pages = pages;
		aobj->u_flags = UAO_FLAG_NOSWAP;
		aobj->u_obj.uo_refs = UVM_OBJ_KERN;
		kobj_alloced = UAO_FLAG_KERNOBJ;
	} else if (flags & UAO_FLAG_KERNSWAP) {
		KASSERT(kobj_alloced == UAO_FLAG_KERNOBJ);
		aobj = &kernel_object_store;
		kobj_alloced = UAO_FLAG_KERNSWAP;
	} else {
		aobj = pool_get(&uvm_aobj_pool, PR_WAITOK);
		aobj->u_pages = pages;
		aobj->u_flags = 0;
		aobj->u_obj.uo_refs = 1;
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
			aobj->u_swhash = hashinit(UAO_SWHASH_BUCKETS(aobj), HASH_LIST, M_UVMAOBJ, mflags, &aobj->u_swhashmask);
			if (aobj->u_swhash == NULL)
				panic("uao_create: hashinit swhash failed");
		} else {
			aobj->u_swslots = malloc(pages * sizeof(int), M_UVMAOBJ, mflags);
			if (aobj->u_swslots == NULL)
				panic("uao_create: malloc swslots failed");
			memset(aobj->u_swslots, 0, pages * sizeof(int));
		}

		if (flags) {
			aobj->u_flags &= ~UAO_FLAG_NOSWAP; /* clear noswap */
			return(&aobj->u_obj);
		}
	}

	/*
 	 * init aobj fields
 	 */

	simple_lock_init(&aobj->u_obj.vmobjlock);
	aobj->u_obj.pgops = &aobj_pager;
	TAILQ_INIT(&aobj->u_obj.memq);
	aobj->u_obj.uo_npages = 0;

	/*
 	 * now that aobj is ready, add it to the global list
 	 */

	simple_lock(&uao_list_lock);
	LIST_INSERT_HEAD(&uao_list, aobj, u_list);
	simple_unlock(&uao_list_lock);
	return(&aobj->u_obj);
}


/*
 * uao_init: set up aobj pager subsystem
 *
 * => called at boot time from uvm_pager_init()
 */

void
uao_init(void)
{
	static int uao_initialized;

	if (uao_initialized)
		return;
	uao_initialized = TRUE;
	LIST_INIT(&uao_list);
	simple_lock_init(&uao_list_lock);

	/*
	 * NOTE: Pages fror this pool must not come from a pageable
	 * kernel map!
	 */

	//pool_init(&uao_swhash_elt_pool, sizeof(struct uao_swhash_elt), 0, 0, 0, "uaoeltpl", NULL);
	//pool_init(&uvm_aobj_pool, sizeof(struct uvm_aobj), 0, 0, 0, "aobjpl", &pool_allocator_nointr);
}

/*
 * uao_reference: add a ref to an aobj
 *
 * => aobj must be unlocked
 * => just lock it and call the locked version
 */

void
uao_reference(uobj)
	struct uvm_object *uobj;
{
	simple_lock(&uobj->vmobjlock);
	uao_reference_locked(uobj);
	simple_unlock(&uobj->vmobjlock);
}

/*
 * uao_reference_locked: add a ref to an aobj that is already locked
 *
 * => aobj must be locked
 * this needs to be separate from the normal routine
 * since sometimes we need to add a reference to an aobj when
 * it's already locked.
 */

void
uao_reference_locked(uobj)
	struct uvm_object *uobj;
{
	//UVMHIST_FUNC("uao_reference"); UVMHIST_CALLED(maphist);

	/*
 	 * kernel_object already has plenty of references, leave it alone.
 	 */

	if (UVM_OBJ_IS_KERN_OBJECT(uobj))
		return;

	uobj->uo_refs++;
	//UVMHIST_LOG(maphist, "<- done (uobj=0x%x, ref = %d)", uobj, uobj->uo_refs,0,0);
}

/*
 * uao_detach: drop a reference to an aobj
 *
 * => aobj must be unlocked
 * => just lock it and call the locked version
 */

void
uao_detach(uobj)
	struct uvm_object *uobj;
{
	simple_lock(&uobj->vmobjlock);
	uao_detach_locked(uobj);
}

/*
 * uao_detach_locked: drop a reference to an aobj
 *
 * => aobj must be locked, and is unlocked (or freed) upon return.
 * this needs to be separate from the normal routine
 * since sometimes we need to detach from an aobj when
 * it's already locked.
 */

void
uao_detach_locked(uobj)
	struct uvm_object *uobj;
{
	struct uvm_aobj *aobj = (struct uvm_aobj *)uobj;
	struct vm_page *pg;
	//UVMHIST_FUNC("uao_detach"); UVMHIST_CALLED(maphist);

	/*
 	 * detaching from kernel_object is a noop.
 	 */

	if (UVM_OBJ_IS_KERN_OBJECT(uobj)) {
		simple_unlock(&uobj->vmobjlock);
		return;
	}

	//UVMHIST_LOG(maphist,"  (uobj=0x%x)  ref=%d", uobj,uobj->uo_refs,0,0);
	uobj->uo_refs--;
	if (uobj->uo_refs) {
		simple_unlock(&uobj->vmobjlock);
		//UVMHIST_LOG(maphist, "<- done (rc>0)", 0,0,0,0);
		return;
	}

	/*
 	 * remove the aobj from the global list.
 	 */

	simple_lock(&uao_list_lock);
	LIST_REMOVE(aobj, u_list);
	simple_unlock(&uao_list_lock);

	/*
 	 * free all the pages left in the aobj.  for each page,
	 * when the page is no longer busy (and thus after any disk i/o that
	 * it's involved in is complete), release any swap resources and
	 * free the page itself.
 	 */

	uvm_lock_pageq();
	while ((pg = TAILQ_FIRST(&uobj->memq)) != NULL) {
		pmap_page_protect(pg, VM_PROT_NONE);
		if (pg->flags & PG_BUSY) {
			pg->flags |= PG_WANTED;
			uvm_unlock_pageq();
			UVM_UNLOCK_AND_WAIT(pg, &uobj->vmobjlock, FALSE,
			    "uao_det", 0);
			simple_lock(&uobj->vmobjlock);
			uvm_lock_pageq();
			continue;
		}
		uao_dropswap(&aobj->u_obj, pg->offset >> PAGE_SHIFT);
		uvm_pagefree(pg);
	}
	uvm_unlock_pageq();

	/*
 	 * finally, free the aobj itself.
 	 */

	uao_free(aobj);
}
