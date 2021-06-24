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
#include <vm/include/vm_object.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_pager.h>

#include <devel/vm/uvm/uvm.h>
#include <devel/vm/uvm/vm_aobject.h>
#include <devel/sys/malloctypes.h>

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

void
vm_aobject_init(size, object, flags)
	vm_size_t				size;
	vm_object_t 			object;
	int 					flags;
{
	register struct uao_swhash_elt 	*elt;

	LIST_INIT(&aobject_list);
	simple_lock_init(&aobject_list_lock, "aobject_list_lock");

	elt = (struct uao_swhash_elt *)malloc(sizeof(struct uao_swhash_elt *), M_VMAOBJ, M_WAITOK);

	vm_aobject_allocate(size, object, flags);
}

static void
vm_aobject_allocate(size, object, flags)
	vm_size_t				size;
	vm_object_t 			object;
	int 					flags;
{
	register vm_aobject_t 	aobject;

	if(flags & UAO_FLAG_KERNOBJ) {
		aobject = (vm_aobject_t)&kernel_object_store;
		aobject->u_flags = UAO_FLAG_NOSWAP;
		aobject->u_ref_count = VM_OBJ_KERN;
		object = (vm_object_t)&aobject;
		vm_object_allocate(sizeof(object));
	} else {										/* normal object */
		aobject = (struct vm_aobject *)malloc(sizeof(struct vm_aobject *), M_VMAOBJ, M_WAITOK);
		aobject->u_flags = object->flags;
		aobject->u_ref_count = object->ref_count;
	}

	simple_lock(&aobject_list_lock);
	LIST_INSERT_HEAD(&aobject_list, aobject, u_list);
	simple_unlock(&aobject_list_lock);
}

/*
 * uao_detach: drop a reference to an aobj
 *
 * => aobj must be unlocked, we will lock it
 */
void
vm_aobject_detach(obj)
	vm_object_t obj;
{
	vm_aobject_t aobj = (struct vm_aobject *)obj;
	vm_segment_t sg;
	vm_page_t 	pg;
	boolean_t busybody;

	/*
 	 * detaching from kernel_object is a noop.
 	 */
	if (obj->ref_count == VM_OBJ_KERN)
		return;

	simple_lock(&obj->Lock);

	obj->ref_count--;					/* drop ref! */
	if (obj->ref_count) {				/* still more refs? */
		simple_unlock(&obj->Lock);
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
	for (sg = CIRCLEQ_FIRST(obj->seglist); sg != NULL; sg = CIRCLEQ_NEXT(sg, sg_list)) {
		if(sg->sg_flags & SEG_BUSY) {
			sg->sg_flags |= SEG_RELEASED;
			busybody = TRUE;
			continue;
		}
		for (pg = TAILQ_FIRST(sg->sg_memq); pg != NULL ; pg = TAILQ_NEXT(pg, listq)) {
			if (pg->flags & PG_BUSY) {
				pg->flags |= PG_RELEASED;
				busybody = TRUE;
				continue;
			}
			/* zap the mappings, free the swap slot, free the page */
			pmap_page_protect(VM_PAGE_TO_PHYS(pg), VM_PROT_NONE);
			vm_aobject_dropswap(&aobj->u_obj, pg->offset >> PAGE_SHIFT);
			vm_segment_lock_lists();
			vm_page_lock_queues();
			vm_page_free(pg);
			vm_page_unlock_queues();
			vm_segment_unlock_lists();
		}
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
	vm_aobject_free(aobj);
}

/*
 * uao_free: free all resources held by an aobj, and then free the aobj
 *
 * => the aobj should be dead
 */
static void
vm_aobject_free(aobj)
	vm_aobject_t aobj;
{

	if (UAO_USES_SWHASH(aobj)) {
		int i, hashbuckets = aobj->u_swhashmask + 1;

		/*
		 * free the swslots from each hash bucket,
		 * then the hash bucket, and finally the hash table itself.
		 */
		for (i = 0; i < hashbuckets; i++) {
			struct uao_swhash_elt *elt, *next;

			for (elt = LIST_FIRST(aobj->u_swhash[i]); elt != NULL; elt = next) {
				int j;

				for (j = 0; j < UAO_SWHASH_CLUSTER_SIZE; j++) {
					int slot = elt->slots[j];

					if (slot) {
						vm_swap_free(slot, 1);

						/*
						 * this page is no longer
						 * only in swap.
						 */
						simple_lock(&uvm.swap_data_lock);
						cnt.v_swpgonly--;
						simple_unlock(&uvm.swap_data_lock);
					}
				}
				next = LIST_NEXT(elt, list);
			}
		}
		FREE(aobj->u_swhash, M_VMAOBJ);
	} else {
		int i;

		/*
		 * free the array
		 */

		for (i = 0; i < aobj->u_pages; i++) {
			int slot = aobj->u_swslots[i];

			if (slot) {
				vm_swap_free(slot, 1);

				/* this page is no longer only in swap. */
				simple_lock(&uvm.swap_data_lock);
				cnt.v_swpgonly--;
				simple_unlock(&uvm.swap_data_lock);
			}
		}
		FREE(aobj->u_swslots, M_VMAOBJ);
	}
}

/*
 * functions
 */

/*
 * hash table/array related functions
 */

void
vm_aobject_swhash_allocate(aobject, pages, flags)
	vm_aobject_t		aobject;
	int 				pages;
	int 				flags;
{
	const int kernswap = (flags & UAO_FLAG_KERNSWAP) != 0;
	if (flags == 0 || kernswap) {
		if (UAO_USES_SWHASH(aobject)) {
			/* allocate hash table or array depending on object size */
			aobject->u_swhash = hashinit(UAO_SWHASH_BUCKETS(aobject), M_VMAOBJ, &aobject->u_swhashmask);
		} else {
			aobject->u_swhash = malloc(pages * sizeof(int), M_VMAOBJ, flags);
			memset(aobject->u_swslots, 0, pages * sizeof(int));
		}
		if (flags) {
			aobject->u_flags &= ~UAO_FLAG_NOSWAP; /* clear noswap */
		}
	}
}

/*
 * uao_find_swhash_elt: find (or create) a hash table entry for a page
 * offset.
 *
 * => the object should be locked by the caller
 */

static struct uao_swhash_elt *
vm_aobject_find_swhash_elt(aobject, pageidx, create)
	vm_aobject_t	aobject;
	int pageidx;
	boolean_t create;
{
	struct aobjectswhash *swhash;
	struct uao_swhash_elt *elt;
	int page_tag;

	swhash = UAO_SWHASH_HASH(aobject, pageidx); 	/* first hash to get bucket */
	page_tag = UAO_SWHASH_ELT_TAG(pageidx); 		/* tag to search for */

	/*
	 * now search the bucket for the requested tag
	 */
	for (elt = LIST_FIRST(swhash); elt != NULL; elt = LIST_NEXT(elt, list)) {
		if (elt->tag == page_tag)
			return (elt);
	}

	/* fail now if we are not allowed to create a new entry in the bucket */
	if (!create)
		return NULL;

	/*
	 * allocate a new entry for the bucket and init/insert it in
	 */

	elt = (struct uao_swhash_elt *)malloc(elt, sizeof(struct uao_swhash_elt *), M_VMAOBJ, M_NOWAIT);
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
vm_aobject_find_swslot(aobj, pageidx)
	vm_aobject_t aobj;
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
		struct uao_swhash_elt *elt = vm_aobject_find_swhash_elt(aobj, pageidx, FALSE);

		if (elt) {
			return (UAO_SWHASH_ELT_PAGESLOT(elt, pageidx));
		} else {
			return (NULL);
		}
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
vm_aobject_set_swslot(obj, pageidx, slot)
	vm_object_t obj;
	int pageidx, slot;
{
	vm_aobject_t aobj = (struct vm_aobject *)obj;
	int oldslot;

	/*
	 * if noswap flag is set, then we can't set a slot
	 */

	if (aobj->u_flags & UAO_FLAG_NOSWAP) {

		if (slot == 0)
			return (0); /* a clear is ok */

		/* but a set is not */
		printf("uao_set_swslot: obj = %p\n", obj);
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
		struct uao_swhash_elt *elt = vm_aobject_find_swhash_elt(aobj, pageidx, slot ? TRUE : FALSE);
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
		} else { 			/* freeing slot ... */
			if (oldslot) 	/* to be safe */
				elt->count--;

			if (elt->count == 0) {
				LIST_REMOVE(elt, list);
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
 * vm_aobject_flush (uao_flush): uh, yea, sure it's flushed.  really!
 */
boolean_t
vm_aobject_flush(obj, start, end, flags)
	vm_object_t obj;
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
	return (TRUE);
}

/*
 * vm_aobject_dropswap (uao_dropswap):  release any swap resources from this aobj page.
 *
 * => aobj must be locked or have a reference count of 0.
 */
void
vm_aobject_dropswap(obj, pageidx)
	vm_object_t obj;
	int pageidx;
{
	int slot;

	slot = vm_aobject_set_swslot(obj, pageidx, 0);
	if (slot) {
		vm_swap_free(slot, 1);
	}
}
