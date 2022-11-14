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

#include <devel/vm/include/vm_object.h>
#include <devel/vm/include/vm_page.h>
#include <devel/vm/include/vm_segment.h>
#include <devel/vm/include/vm.h>

/*
 *	Virtual memory objects maintain the actual data
 *	associated with allocated virtual memory.  A given
 *	page of memory exists within exactly one object.
 *
 *	An object is only deallocated when all "references"
 *	are given up.  Only one "reference" to a given
 *	region of an object should be writeable.
 *
 *	Associated with each object is a list of all resident
 *	memory pages belonging to that object; this list is
 *	maintained by the "vm_page" module, and locked by the object's
 *	lock.
 *
 *	Each object also records a "pager" routine which is
 *	used to retrieve (and store) pages to the proper backing
 *	storage.  In addition, objects may be backed by other
 *	objects from which they were virtual-copied.
 *
 *	The only items within the object structure which are
 *	modified after time of creation are:
 *		reference count		locked by object's lock
 *		pager routine		locked by object's lock
 *
 */

/*
 * New VM Object Structure:
 * Layout: (Assuming n Segments per Object (architecture dependent)
 *
 * 			 |----> Segment 1 -------> Pages 1 to (n-1)
 * 			 |
 * 			 |----> Segment 2 -------> Pages 1 to (n-1)
 * 			 |
 * Object ---|----> Segment 3 -------> Pages 1 to (n-1)
 * 			 |
 * 			 |----> Segment 4 -------> Pages 1 to (n-1)
 * 			 |
 * 			 |----> Segment (n-1) ---> Pages 1 to (n-1)
 *
 */

struct vm_object	kernel_object_store;
struct vm_object	kmem_object_store;

#define	VM_OBJECT_HASH_COUNT	157

int		vm_cache_max = 100;	/* can patch if necessary */
struct	vm_object_hash_head vm_object_hashtable[VM_OBJECT_HASH_COUNT];

long	object_collapses = 0;
long	object_bypasses  = 0;

static void _vm_object_allocate(vm_size_t, vm_object_t);

/*
 *	vm_object_init:
 *
 *	Initialize the VM objects module, and VM aobjects module.
 */
void
vm_object_init(size, flags)
	vm_size_t	size;
	int 		flags;
{
	register int	i;

	TAILQ_INIT(&vm_object_cached_list);
	SPLAY_INIT(&vm_object_cached_tree);
	RB_INIT(&vm_object_tree);
	vm_object_count = 0;
	simple_lock_init(&vm_cache_lock);
	simple_lock_init(&vm_object_tree_lock);

	for (i = 0; i < VM_OBJECT_HASH_COUNT; i++) {
		RB_INIT(&vm_object_hashtable[i]);
	}

	kernel_object = &kernel_object_store;
	_vm_object_allocate(size, kernel_object);

	kmem_object = &kmem_object_store;
	_vm_object_allocate(VM_KMEM_SIZE + VM_MBUF_SIZE, kmem_object);

	vm_aobject_init(size, kernel_object, flags);
}

/*
 *	vm_object_allocate:
 *
 *	Returns a new object with the given size.
 */

vm_object_t
vm_object_allocate(size)
	vm_size_t	size;
{
	register vm_object_t	result;

	result = (vm_object_t)malloc((u_long)sizeof(*result), M_VMOBJ, M_WAITOK);

	_vm_object_allocate(size, result);

	return (result);
}

static void
_vm_object_allocate(size, object)
	vm_size_t				size;
	register vm_object_t	object;
{
	CIRCLEQ_INIT(&object->seglist);
	vm_object_lock_init(object);
	object->ref_count = 1;
	object->resident_segment_count = 0;
	object->size = size;
	object->flags = OBJ_INTERNAL;	/* vm_allocate_with_segment will reset */
	object->copy = NULL;

	/*
	 *	Object starts out read-write, with no pager.
	 */
	object->pager = NULL;
	object->paging_offset = 0;
	object->shadow = NULL;
	object->shadow_offset = (vm_offset_t) 0;

	simple_lock(&vm_object_tree_lock);
	RB_INSERT(objectrbt, &vm_object_tree, object);
	vm_object_count++;
	cnt.v_nzfod += atop(size);
	simple_unlock(&vm_object_tree_lock);
}

/* vm object splay tree comparator */
/*
int
vm_object_spt_compare(obj1, obj2)
	vm_object_t obj1, obj2;
{
	if(obj1->size < obj2->size) {
		return (-1);
	} else if(obj1->size > obj2->size) {
		return (1);
	}
	return (0);
}

SPLAY_PROTOTYPE(object_spt, vm_object, cached_tree, vm_object_spt_compare);
SPLAY_GENERATE(object_spt, vm_object, cached_tree, vm_object_spt_compare);
*/

/*
 *	vm_object_reference:
 *
 *	Gets another reference to the given object.
 */
void
vm_object_reference(object)
	register vm_object_t	object;
{
	if (object == NULL)
		return;

	vm_object_lock(object);
	object->ref_count++;
	vm_object_unlock(object);
}

/*
 *	vm_object_deallocate:
 *
 *	Release a reference to the specified object,
 *	gained either through a vm_object_allocate
 *	or a vm_object_reference call.  When all references
 *	are gone, storage associated with this object
 *	may be relinquished.
 *
 *	No object may be locked.
 */
void
vm_object_deallocate(object)
	register vm_object_t	object;
{
	vm_object_t	temp;

	while (object != NULL) {

		/* deallocate an aobject */
		vm_aobject_deallocate(object);

		/*
		 *	The cache holds a reference (uncounted) to
		 *	the object; we must lock it before removing
		 *	the object.
		 */

		vm_object_cache_lock();

		/*
		 *	Lose the reference
		 */
		vm_object_lock(object);
		if (--(object->ref_count) != 0) {

			/*
			 *	If there are still references, then
			 *	we are done.
			 */
			vm_object_unlock(object);
			vm_object_cache_unlock();
			return;
		}

		/*
		 *	See if this object can persist.  If so, enter
		 *	it in the cache, then deactivate all of its
		 *	pages.
		 */

		if (object->flags & OBJ_CANPERSIST) {
			//SPLAY_INSERT(object_spt, &vm_object_cached_tree, object);
			TAILQ_INSERT_TAIL(&vm_object_cached_list, object, cached_list);
			vm_object_cached++;
			vm_object_cache_unlock();

			vm_object_deactivate_pages(object);
			vm_object_unlock(object);

			vm_object_cache_trim();
			return;
		}

		/*
		 *	Make sure no one can look us up now.
		 */
		vm_object_remove(object->pager);
		vm_object_cache_unlock();

		temp = object->shadow;
		vm_object_terminate(object);
		/* unlocks and deallocates object */
		object = temp;
	}
}

/*
 *	vm_object_terminate actually destroys the specified object, freeing
 *	up all previously used resources.
 *
 *	The object must be locked.
 */
void
vm_object_terminate(object)
	register vm_object_t	object;
{
	register vm_segment_t	seg;
	register vm_page_t		page;
	vm_object_t				shadow_object;

	/*
	 *	Detach the object from its shadow if we are the shadow's
	 *	copy.
	 */
	shadow_object = object->shadow;
	if (shadow_object != NULL) {
		vm_object_lock(shadow_object);
		if (shadow_object->copy == object) {
			shadow_object->copy = NULL;
		}
#if 0
		else if (shadow_object->copy != NULL) {
			panic("vm_object_terminate: copy/shadow inconsistency");
		}
#endif
		vm_object_unlock(shadow_object);
	}

	//vm_object_shadow_detach(object);

	/*
	 * Wait until the pageout daemon is through with the object.
	 */
	while (object->paging_in_progress) {
		vm_object_sleep(object, object, FALSE);
		vm_object_lock(object);
	}

	/*
	 * If not an internal object clean all the pages, removing them
	 * from paging queues as we go.
	 *
	 * XXX need to do something in the event of a cleaning error.
	 */
	if ((object->flags & OBJ_INTERNAL) == 0)
		(void) vm_object_page_clean(object, 0, 0, TRUE, TRUE);

	/*
	 * Now free the pages & segments.
	 * For internal objects, this also removes them from paging queues.
	 */
	while ((seg = CIRCLEQ_FIRST(object->seglist)) != NULL) {
		VM_SEGMENT_CHECK(seg);
		vm_segment_lock_lists();
		while ((page = TAILQ_FIRST(seg->sg_memq)) != NULL) {
			VM_PAGE_CHECK(page);
			vm_page_lock_queues();
			vm_page_free(page);
			cnt.v_pfree++;
			vm_page_unlock_queues();
		}
		vm_segment_free(seg);
		cnt.v_segment_free++;
		vm_segment_unlock_list();
	}
	vm_object_unlock(object);

	/*
	 * Let the pager know object is dead.
	 */

	if (object->pager != NULL)
		vm_pager_deallocate(object->pager);

	simple_lock(&vm_object_tree_lock);
	RB_REMOVE(objectrbt, &vm_object_tree, object);
	vm_object_count--;
	simple_unlock(&vm_object_tree_lock);

	/*
	 * Free the space for the object.
	 */
	free((caddr_t)object, M_VMOBJ);
}

#ifdef notyet
bool_t
vm_object_segment_clean(object, start, end, syncio, de_queue)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
	bool_t					syncio;
	bool_t					de_queue;
{
	register vm_segment_t	segment;

	if (object == NULL) {
		return (TRUE);
	}

	if ((object->flags & OBJ_INTERNAL) && object->pager == NULL) {
		vm_object_collapse(object);
		if (object->pager == NULL) {
			vm_pager_t pager;

			vm_object_unlock(object);
			pager = vm_pager_allocate(PG_DFLT, (caddr_t) 0, object->size, VM_PROT_ALL, (vm_offset_t) 0);
			if (pager) {
				vm_object_setpager(object, pager, 0, FALSE);
			}
			vm_object_lock(object);
		}
	}
	if (object->pager == NULL) {
		return (FALSE);
	}

	CIRCLEQ_FOREACH(segment, object->seglist, sg_list) {
		if ((start == end || (segment->sg_offset >= start && segment->sg_offset < end))) {
			if ((segment->sg_flags & SEG_CLEAN) && pmap_is_modified(VM_SEGMENT_TO_PHYS(segment))) {

			}
		}
	}
}
#endif

/*
 *	vm_object_page_clean
 *
 *	Clean all dirty pages in the specified range of object.
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
vm_object_page_clean(object, start, end, syncio, de_queue)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
	bool_t				syncio;
	bool_t				de_queue;
{
	register vm_segment_t	segment;
	register vm_page_t		page;
	int onqueue;
	bool_t noerror = TRUE;

	if (object == NULL)
		return (TRUE);

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
			pager = vm_pager_allocate(PG_DFLT, (caddr_t) 0, object->size, VM_PROT_ALL, (vm_offset_t) 0);
			if (pager)
				vm_object_setpager(object, pager, 0, FALSE);
			vm_object_lock(object);
		}
	}
	if (object->pager == NULL)
		return (FALSE);

	again:
	/*
	 * Wait until the pageout daemon is through with the object.
	 */
	while (object->paging_in_progress) {
		vm_object_sleep(object, object, FALSE);
		vm_object_lock(object);
	}
	/*
	 * Loop through the object page list cleaning as necessary.
	 */
	TAILQ_FOREACH(page, segment->sg_memq, listq) {
		if ((start == end || (page->offset >= start && page->offset < end)) && !(page->flags & PG_FICTITIOUS)) {
			if ((page->flags & PG_CLEAN) && pmap_is_modified(VM_PAGE_TO_PHYS(page)))
				page->flags &= ~PG_CLEAN;
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
				} else
					onqueue = 0;
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
					printf("%s: pager_put error\n", "vm_object_page_clean");
					page->flags |= PG_CLEAN;
					noerror = FALSE;
				}
				vm_object_lock(object);
				object->paging_in_progress--;
				if (!de_queue && onqueue) {
					vm_page_lock_queues();
					if (onqueue > 0)
						vm_page_activate(page);
					else
						vm_page_deactivate(page);
					vm_page_unlock_queues();
				}
				page->flags &= ~PG_BUSY;
				PAGE_WAKEUP(page);
				goto again;
			}
		}
	}
	return (noerror);
}

/*
 *	vm_object_deactivate_pages
 *
 *	Deactivate all pages in the specified object.  (Keep its pages
 *	in memory even though it is no longer referenced.)
 *
 *	The object must be locked.
 */
void
vm_object_deactivate_pages(object)
	register vm_object_t	object;
{
	register vm_segment_t 	segment;
	register vm_page_t	 	page;

	CIRCLEQ_FOREACH(segment, object->seglist, sg_list) {
		if (segment->sg_object == object) {
			vm_segment_lock_lists();
			if (segment->sg_memq == NULL) {
				vm_segment_deactivate(segment);
			} else {
				TAILQ_FOREACH(page, segment->sg_memq, listq) {
					if (page->segment == segment) {
						vm_page_lock_queues();
						vm_page_deactivate(page);
						vm_page_unlock_queues();
					}
				}
			}
			vm_segment_unlock_lists();
		}
	}
}

/*
 *	Trim the object cache to size.
 */
void
vm_object_cache_trim()
{
	register vm_object_t	object;

	vm_object_cache_lock();
	while (vm_object_cached > vm_cache_max) {
		//object = SPLAY_FIRST(&vm_object_cached_tree);
		object = TAILQ_FIRST(vm_object_cached_list);
		vm_object_cache_unlock();

		if (object != vm_object_lookup(object->pager))
			panic("vm_object_deactivate: I'm sooo confused.");

		pager_cache(object, FALSE);

		vm_object_cache_lock();
	}
	vm_object_cache_unlock();
}

/*
 *	vm_object_pmap_copy:
 *
 *	Makes all physical pages in the specified
 *	object range copy-on-write.  No writeable
 *	references to these pages should remain.
 *
 *	The object must *not* be locked.
 */
void
vm_object_pmap_copy(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_segment_t 	seg;
	register vm_page_t		page;

	if (object == NULL)
		return;

	vm_object_lock(object);
	for (seg = CIRCLEQ_FIRST(object->seglist); seg != NULL;	seg = CIRCLEQ_NEXT(seg, sg_list)) {
		if (seg->sg_object == object && seg != NULL) {
			for (page = TAILQ_FIRST(seg->sg_memq); page != NULL; page =	TAILQ_NEXT(page, listq)) {
				if ((start <= page->offset) && (page->offset < end)) {
					pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_READ);
					page->flags |= PG_COPYONWRITE;
				}
			}
		}
	}
	vm_object_unlock(object);
}

/*
 *	vm_object_pmap_remove:
 *
 *	Removes all physical pages in the specified
 *	segment of an object from all physical maps.
 *
 *	The object must *not* be locked.
 */
void
vm_object_pmap_remove(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_segment_t 	segment;
	register vm_page_t		page;

	if (object == NULL) {
		return;
	}

	vm_object_lock(object);
	CIRCLEQ_FOREACH(segment, &object->seglist, sg_list) {
		if (segment->sg_object == object && segment != NULL) {
			TAILQ_FOREACH(page, &segment->sg_memq, listq) {
				if ((start <= page->offset) && (page->offset < end)) {
					pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_NONE);
				}
			}
		}
	}
	vm_object_unlock(object);
}

/*
 * shadow slot hash function
 * TODO:
 * - determine a shadow object from shadow slot.
 * - validate/consistency check of shadow object.
 */
u_long
vm_object_shadow_index(object, offset)
	vm_object_t object;
	vm_offset_t offset;
{
	Fnv32_t slot = fnv_32_buf(&object, (sizeof(&object)+offset), FNV1_32_INIT)%(sizeof(&object)+offset);

	return (slot);
}

/*
 * Add shadow object to list of anon objects list
 */
void
vm_object_shadow_add(aobject, shadow, offset)
	vm_aobject_t aobject;
	vm_object_t shadow;
	vm_offset_t offset;
{
    if (shadow) {
        shadow->ref_count++;
        LIST_INSERT_HEAD(&aobject_list, aobject, u_list);
    }
    aobject->u_obj.shadow = shadow;
    aobject->u_obj.shadow_offset = offset;
    //aobject->u_obj.flags |= flags;
}

void
vm_object_shadow_remove(aobject, shadow)
	vm_aobject_t aobject;
	vm_object_t  shadow;
{
    if (aobject->u_obj.shadow == shadow) {
        return;
    }
    if (aobject->u_obj.shadow) {
        aobject->u_obj.shadow->ref_count--;
        LIST_REMOVE(aobject, u_list);
    }
}

void
vm_object_shadow_attach(object, offset)
    vm_object_t object;
	vm_offset_t offset;
{
    register vm_aobject_t aobject;

    aobject = (vm_aobject_t)object;
    vm_object_shadow_add(aobject, object, offset);
}

void
vm_object_shadow_detach(object)
	vm_object_t object;
{
	register vm_aobject_t aobject;

	aobject = (vm_aobject_t)object;
    aobject->u_obj.shadow = object->shadow;
    if (aobject->u_obj.shadow != NULL) {
        vm_object_lock(aobject->u_obj.shadow);
        if (aobject->u_obj.shadow->copy == object) {
			aobject->u_obj.shadow->copy = NULL;
		}
        vm_object_unlock(aobject->u_obj.shadow);
    }
    vm_object_shadow_remove(aobject, object);
}

/*
 *	vm_object_shadow:
 *
 *	Create a new object which is backed by the
 *	specified existing object range.  The source
 *	object reference is deallocated.
 *
 *	The new object and offset into that object
 *	are returned in the source parameters.
 */
void
vm_object_shadow(object, offset, length)
	vm_object_t	*object;	/* IN/OUT */
	vm_offset_t	*offset;	/* IN/OUT */
	vm_size_t	length;
{
	register vm_object_t	source;
	register vm_object_t	result;

	source = *object;

	/*
	 *	Allocate a new object with the given length
	 */

	if ((result = vm_object_allocate(length)) == NULL) {
		panic("vm_object_shadow: no object for shadowing");
	}

	/*
	 *	The new object shadows the source object, adding
	 *	a reference to it.  Our caller changes his reference
	 *	to point to the new object, removing a reference to
	 *	the source object.  Net result: no change of reference
	 *	count.
	 */
	result->shadow = source;

	/*
	 *	Store the offset into the source object,
	 *	and fix up the offset into the new object.
	 */

	result->shadow_offset = *offset;

	//vm_object_shadow_attach(result, result->shadow_offset);

	/*
	 *	Return the new things
	 */

	*offset = 0;
	*object = result;
}

/* vm object rbtree comparator */
int
vm_object_rb_compare(obj1, obj2)
	struct vm_object *obj1, *obj2;
{
	if(obj1->size < obj2->size) {
		return (-1);
	} else if(obj1->size > obj2->size) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(objectrbt, vm_object, object_tree, vm_object_rb_compare);
RB_GENERATE(objectrbt, vm_object, object_tree, vm_object_rb_compare);

RB_PROTOTYPE(vm_object_hash_head, vm_object_hash_entry, hash_links, vm_object_rb_compare);
RB_GENERATE(vm_object_hash_head, vm_object_hash_entry, hash_links, vm_object_rb_compare);

/*
 *	vm_object_hash hashes the pager/id pair.
 */
unsigned long
vm_object_hash(pager)
	vm_pager_t pager;
{
    Fnv32_t hash1 = fnv_32_buf(&pager, sizeof(&pager), FNV1_32_INIT) % VM_OBJECT_HASH_COUNT;
    Fnv32_t hash2 = (((unsigned long)pager)%VM_OBJECT_HASH_COUNT);
    return (hash1^hash2);
}

/*
 *	vm_object_lookup looks in the object cache for an object with the
 *	specified pager and paging id.
 */
vm_object_t
vm_object_lookup(pager)
	vm_pager_t	pager;
{
	struct vm_object_hash_head		*bucket;
	register vm_object_hash_entry_t	entry;
	vm_object_t						object;

	vm_object_cache_lock();
	bucket = &vm_object_hashtable[vm_object_hash(pager)];
	for (entry = RB_FIRST(vm_object_hash_head, bucket); entry != NULL; entry = RB_NEXT(vm_object_hash_head, bucket, entry)) {
		object = entry->object;
		if (object->pager == pager) {
			vm_object_lock(object);
			if (object->ref_count == 0) {
				//SPLAY_REMOVE(object_spt, &vm_object_cached_tree, object);
				TAILQ_REMOVE(&vm_object_cached_list, object, cached_list);
				vm_object_cached--;
			}
			object->ref_count++;
			vm_object_unlock(object);
			vm_object_cache_unlock();
			return (object);
		}
	}

	vm_object_cache_unlock();
	return (NULL);
}

/*
 *	vm_object_enter enters the specified object/pager/id into
 *	the hash table.
 */
void
vm_object_enter(object, pager)
	vm_object_t	object;
	vm_pager_t	pager;
{
	struct vm_object_hash_head	*bucket;
	register vm_object_hash_entry_t	entry;

	/*
	 *	We don't cache null objects, and we can't cache
	 *	objects with the null pager.
	 */

	if (object == NULL)
		return;
	if (pager == NULL)
		return;

	bucket = &vm_object_hashtable[vm_object_hash(pager)];
	entry = (vm_object_hash_entry_t)malloc((u_long)sizeof *entry, M_VMOBJHASH, M_WAITOK);
	entry->object = object;
	object->flags |= OBJ_CANPERSIST;

	vm_object_cache_lock();
	RB_INSERT(vm_object_hash_head, bucket, entry);
	vm_object_cache_unlock();
}

/*
 *	vm_object_remove:
 *
 *	Remove the pager from the hash table.
 *	Note:  This assumes that the object cache
 *	is locked.  XXX this should be fixed
 *	by reorganizing vm_object_deallocate.
 */
void
vm_object_remove(pager)
	register vm_pager_t	pager;
{
	struct vm_object_hash_head		*bucket;
	register vm_object_hash_entry_t	entry;
	register vm_object_t			object;

	bucket = &vm_object_hashtable[vm_object_hash(pager)];

	for (entry = RB_FIRST(vm_object_hash_head, bucket); entry != NULL; entry = RB_NEXT(vm_object_hash_head, bucket, entry)) {
		object = entry->object;
		if (object->pager == pager) {
			RB_REMOVE(vm_object_hash_head, bucket, entry);
			free((caddr_t)entry, M_VMOBJHASH);
			break;
		}
	}
}

/*
 *	vm_object_cache_clear removes all objects from the cache.
 *
 */
void
vm_object_cache_clear()
{
	register vm_object_t	object;

	/*
	 *	Remove each object in the cache by scanning down the
	 *	list of cached objects.
	 */
	vm_object_cache_lock();
	//while((object = SPLAY_FIND(object_spt, &vm_object_cached_tree, object)) != NULL) {
	while ((object = TAILQ_FIRST(&vm_object_cached_list)) != NULL) {
		vm_object_cache_unlock();

		/*
		 * Note: it is important that we use vm_object_lookup
		 * to gain a reference, and not vm_object_reference, because
		 * the logic for removing an object from the cache lies in
		 * lookup.
		 */
		if (object != vm_object_lookup(object->pager))
			panic("vm_object_cache_clear: I'm sooo confused.");
		pager_cache(object, FALSE);

		vm_object_cache_lock();
	}
	vm_object_cache_unlock();
}

bool_t	vm_object_collapse_allowed = TRUE;
/*
 *	vm_object_collapse:
 *
 *	Collapse an object with the object backing it.
 *	Pages in the backing object are moved into the
 *	parent, and the backing object is deallocated.
 *
 *	Requires that the object be locked and the page
 *	queues be unlocked.
 *
 */
void
vm_object_collapse(object)
	register vm_object_t	object;

{
	register vm_object_t	backing_object;
	register vm_offset_t	backing_offset;
	register vm_size_t		size;
	register vm_offset_t	new_offset;
	register vm_segment_t	segment;
	register vm_page_t		p, pp;

	if (!vm_object_collapse_allowed)
		return;

	while (TRUE) {
		/*
		 *	Verify that the conditions are right for collapse:
		 *
		 *	The object exists and no pages in it are currently
		 *	being paged out (or have ever been paged out).
		 */
		if (object == NULL || object->paging_in_progress != 0 || object->pager != NULL)
			return;
		/*
		 *		There is a backing object, and
		 */

		if ((backing_object = object->shadow) == NULL)
			return;

		vm_object_lock(backing_object);
		/*
		 *	...
		 *		The backing object is not read_only,
		 *		and no pages in the backing object are
		 *		currently being paged out.
		 *		The backing object is internal.
		 */

		if ((backing_object->flags & OBJ_INTERNAL) == 0
				|| backing_object->paging_in_progress != 0) {
			vm_object_unlock(backing_object);
			return;
		}

		/*
		 *	The backing object can't be a copy-object:
		 *	the shadow_offset for the copy-object must stay
		 *	as 0.  Furthermore (for the 'we have all the
		 *	pages' case), if we bypass backing_object and
		 *	just shadow the next object in the chain, old
		 *	pages from that object would then have to be copied
		 *	BOTH into the (former) backing_object and into the
		 *	parent object.
		 */
		if (backing_object->shadow != NULL
				&& backing_object->shadow->copy != NULL) {
			vm_object_unlock(backing_object);
			return;
		}

		/*
		 *	We know that we can either collapse the backing
		 *	object (if the parent is the only reference to
		 *	it) or (perhaps) remove the parent's reference
		 *	to it.
		 */

		backing_offset = object->shadow_offset;
		size = object->size;

		/*
		 *	If there is exactly one reference to the backing
		 *	object, we can collapse it into the parent.
		 */
		if (backing_object->ref_count == 1) {

			/*
			 *	We can collapse the backing object.
			 *
			 *	Move all in-memory pages from backing_object
			 *	to the parent.  Pages that have been paged out
			 *	will be overwritten by any of the parent's
			 *	pages that shadow them.
			 */
			while ((segment = CIRCLEQ_FIRST(backing_object->seglist)) != NULL) {
				if(segment->sg_object == backing_object) {
					while ((p = TAILQ_FIRST(segment->sg_memq)) != NULL) {
						new_offset = (p->offset - backing_offset);

						/*
						 *	If the parent has a page here, or if
						 *	this page falls outside the parent,
						 *	dispose of it.
						 *
						 *	Otherwise, move it as planned.
						 */

						if (p->offset < backing_offset || new_offset >= size) {
							vm_page_lock_queues();
							vm_page_free(p);
							vm_page_unlock_queues();
						} else {
							pp = vm_page_lookup(segment, new_offset);
							if (pp != NULL && !(pp->flags & PG_FAKE)) {
								vm_page_lock_queues();
								vm_page_free(p);
								vm_page_unlock_queues();
							} else {
								if (pp) {
									/* may be someone waiting for it */
									PAGE_WAKEUP(pp);
									vm_page_lock_queues();
									vm_page_free(pp);
									vm_page_unlock_queues();
								}
								vm_page_rename(p, segment, new_offset);
							}
						}
					}
				}
			}
			/*
			 *	Move the pager from backing_object to object.
			 *
			 *	XXX We're only using part of the paging space
			 *	for keeps now... we ought to discard the
			 *	unused portion.
			 */

			if (backing_object->pager) {
				object->pager = backing_object->pager;
				object->paging_offset = backing_offset
						+ backing_object->paging_offset;
				backing_object->pager = NULL;
			}

			/*
			 *	Object now shadows whatever backing_object did.
			 *	Note that the reference to backing_object->shadow
			 *	moves from within backing_object to within object.
			 */

			object->shadow = backing_object->shadow;
			object->shadow_offset += backing_object->shadow_offset;
			if (object->shadow != NULL && object->shadow->copy != NULL) {
				panic("vm_object_collapse: we collapsed a copy-object!");
			}
			/*
			 *	Discard backing_object.
			 *
			 *	Since the backing object has no pages, no
			 *	pager left, and no object references within it,
			 *	all that is necessary is to dispose of it.
			 */

			vm_object_unlock(backing_object);

			simple_lock(&vm_object_list_lock);
			RB_REMOVE(objectrbt, &vm_object_tree, backing_object);
			vm_object_count--;
			simple_unlock(&vm_object_list_lock);

			free((caddr_t) backing_object, M_VMOBJ);

			object_collapses++;
		} else {
			/*
			 *	If all of the pages in the backing object are
			 *	shadowed by the parent object, the parent
			 *	object no longer has to shadow the backing
			 *	object; it can shadow the next one in the
			 *	chain.
			 *
			 *	The backing object must not be paged out - we'd
			 *	have to check all of the paged-out pages, as
			 *	well.
			 */

			if (backing_object->pager != NULL) {
				vm_object_unlock(backing_object);
				return;
			}

			/*
			 *	Should have a check for a 'small' number
			 *	of pages here.
			 */
			CIRCLEQ_FOREACH(segment, backing_object->seglist, sg_list) {
				TAILQ_FOREACH(p, segment->sg_memq, listq) {
					new_offset = (p->offset - backing_offset);
					/*
					 *	If the parent has a page here, or if
					 *	this page falls outside the parent,
					 *	keep going.
					 *
					 *	Otherwise, the backing_object must be
					 *	left in the chain.
					 */

					if (p->offset >= backing_offset && new_offset < size && ((pp = vm_page_lookup(segment, new_offset)) == NULL
									|| (pp->flags & PG_FAKE))) {
						/*
						 *	Page still needed.
						 *	Can't go any further.
						 */
						vm_object_unlock(backing_object);
						return;
					}
				}
			}

			/*
			 *	Make the parent shadow the next object
			 *	in the chain.  Deallocating backing_object
			 *	will not remove it, since its reference
			 *	count is at least 2.
			 */

			object->shadow = backing_object->shadow;
			vm_object_reference(object->shadow);
			object->shadow_offset += backing_object->shadow_offset;

			/*
			 *	Backing object might have had a copy pointer
			 *	to us.  If it did, clear it.
			 */
			if (backing_object->copy == object) {
				backing_object->copy = NULL;
			}

			/*	Drop the reference count on backing_object.
			 *	Since its ref_count was at least 2, it
			 *	will not vanish; so we don't need to call
			 *	vm_object_deallocate.
			 */
			backing_object->ref_count--;
			vm_object_unlock(backing_object);

			object_bypasses++;

		}

		/*
		 *	Try again with this object's new backing object.
		 */
	}
}

/*
 *	vm_object_segment_page_remove: [internal]
 *
 *	Removes all physical pages in the specified
 *	segment from the segments's list of pages.
 *
 *	The object must be locked.
 */
void
vm_object_segment_page_remove(segment, start, end)
	register vm_segment_t 	segment;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_page_t		page;

	if(segment == NULL) {
		return;
	}

	for (page = TAILQ_FIRST(segment->sg_memq); page != NULL; page =	TAILQ_NEXT(page, listq)) {
		if ((start <= page->offset) && (page->offset < end)) {
			pmap_page_protect(VM_PAGE_TO_PHYS(page), VM_PROT_NONE);
			vm_page_lock_queues();
			vm_page_free(page);
			vm_page_unlock_queues();
		}
	}
}

/*
 *	vm_object_page_remove: [internal]
 *
 *	Removes all physical pages in the specified
 *	object range from the object's list of pages.
 *
 *	The object must be locked.
 */
void
vm_object_page_remove(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_segment_t 	segment;

	if (object == NULL) {
		return;
	}

	for (segment = CIRCLEQ_FIRST(object->seglist); segment != NULL;	segment = CIRCLEQ_NEXT(segment, sg_list)) {
		if ((start <= segment->sg_offset) && (segment->sg_offset < end)) {
			vm_object_segment_page_remove(segment, start, end);
		}
	}
}

/*
 *	vm_object_segment_remove: [internal]
 *
 *	Removes all segments in the specified
 *	object range from the object's list of segments.
 *
 *	The object must be locked.
 */
void
vm_object_segment_remove(object, start, end)
	register vm_object_t	object;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register vm_segment_t 	segment;

	if (object == NULL)
		return;

	for (segment = CIRCLEQ_FIRST(object->seglist); segment != NULL; segment = CIRCLEQ_NEXT(segment, sg_list)) {
		if ((start <= segment->sg_offset) && (segment->sg_offset < end)) {
			if (segment->sg_memq == NULL) {
				vm_segment_lock_lists();
				vm_segment_free(segment);
				vm_segment_unlock_lists();
			}
		}
	}
}

/*
 *	Routine:	vm_object_coalesce
 *	Function:	Coalesces two objects backing up adjoining
 *			regions of memory into a single object.
 *
 *	returns TRUE if objects were combined.
 *
 *	NOTE:	Only works at the moment if the second object is NULL -
 *		if it's not, which object do we lock first?
 *
 *	Parameters:
 *		prev_object	First object to coalesce
 *		prev_offset	Offset into prev_object
 *		next_object	Second object into coalesce
 *		next_offset	Offset into next_object
 *
 *		prev_size	Size of reference to prev_object
 *		next_size	Size of reference to next_object
 *
 *	Conditions:
 *	The object must *not* be locked.
 */
bool_t
vm_object_coalesce(prev_object, next_object, prev_offset, next_offset, prev_size, next_size)

	register vm_object_t	prev_object;
	vm_object_t				next_object;
	vm_offset_t				prev_offset, next_offset;
	vm_size_t				prev_size, next_size;
{
	vm_size_t newsize;

#ifdef	lint
	next_offset++;
#endif

	if (next_object != NULL) {
		return (FALSE);
	}

	if (prev_object == NULL) {
		return (TRUE);
	}

	vm_object_lock(prev_object);

	/*
	 *	Try to collapse the object first
	 */
	vm_object_collapse(prev_object);

	/*
	 *	Can't coalesce if:
	 *	. more than one reference
	 *	. paged out
	 *	. shadows another object
	 *	. has a copy elsewhere
	 *	(any of which mean that the pages not mapped to
	 *	prev_entry may be in use anyway)
	 */

	if (prev_object->ref_count > 1 ||
	prev_object->pager != NULL ||
	prev_object->shadow != NULL ||
	prev_object->copy != NULL) {
		vm_object_unlock(prev_object);
		return (FALSE);
	}

	/*
	 *	Remove any pages that may still be in the object from
	 *	a previous deallocation.
	 */

	vm_object_page_remove(prev_object, prev_offset + prev_size,
			prev_offset + prev_size + next_size);

	/*
	 *	Extend the object if necessary.
	 */
	newsize = prev_offset + prev_size + next_size;
	if (newsize > prev_object->size)
		prev_object->size = newsize;

	vm_object_unlock(prev_object);
	return (TRUE);
}
