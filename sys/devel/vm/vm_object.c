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

#include <devel/vm/include/vm_page.h>
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

struct vm_object	kernel_object_store;
struct vm_object	kmem_object_store;

#define	VM_OBJECT_HASH_COUNT	157

int		vm_cache_max = 100;	/* can patch if necessary */
struct	vm_object_hash_head vm_object_hashtable[VM_OBJECT_HASH_COUNT];

long	object_collapses = 0;
long	object_bypasses  = 0;

static void _vm_object_allocate (vm_size_t, vm_object_t);

/*
 *	vm_object_init:
 *
 *	Initialize the VM objects module.
 */
void
vm_object_init(size)
	vm_size_t	size;
{
	register int	i;

	TAILQ_INIT(&vm_object_cached_list);
	TAILQ_INIT(&vm_object_list);
	vm_object_count = 0;
	simple_lock_init(&vm_cache_lock);
	simple_lock_init(&vm_object_list_lock);

	for (i = 0; i < VM_OBJECT_HASH_COUNT; i++)
		TAILQ_INIT(&vm_object_hashtable[i]);

	kernel_object = &kernel_object_store;
	_vm_object_allocate(size, kernel_object);

	kmem_object = &kmem_object_store;
	_vm_object_allocate(VM_KMEM_SIZE + VM_MBUF_SIZE, kmem_object);
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

	result = (vm_object_t)malloc((u_long)sizeof *result, M_VMOBJ, M_WAITOK);

	_vm_object_allocate(size, result);

	return(result);
}

static void
_vm_object_allocate(size, object)
	vm_size_t		size;
	register vm_object_t	object;
{
	TAILQ_INIT(&object->memq);
	vm_object_lock_init(object);
	object->ref_count = 1;
	object->resident_page_count = 0;
	object->size = size;
	object->flags = OBJ_INTERNAL;	/* vm_allocate_with_pager will reset */
	object->paging_in_progress = 0;
	object->copy = NULL;

	/*
	 *	Object starts out read-write, with no pager.
	 */

	object->pager = NULL;
	object->paging_offset = 0;
	object->shadow = NULL;
	object->shadow_offset = (vm_offset_t) 0;

	simple_lock(&vm_object_list_lock);
	TAILQ_INSERT_TAIL(&vm_object_list, object, object_list);
	vm_object_count++;
	cnt.v_nzfod += atop(size);
	simple_unlock(&vm_object_list_lock);
}

/*
 *	vm_object_hash hashes the pager/id pair.
 */
u_long
vm_object_hash(pager)
	vm_pager_t pager;
{
    Fnv32_t hash1 = fnv_32_buf(&pager, sizeof(&pager), FNV1_32_INIT) % VM_OBJECT_HASH_COUNT;
    //Fnv32_t hash2 = fnv_32_buf(&pager, sizeof(&pager), FNV1_32_INIT) % OVL_OBJECT_HASH_COUNT;
    return (hash1);
}

/*
 *	vm_object_lookup looks in the object cache for an object with the
 *	specified pager and paging id.
 */

vm_object_t
vm_object_lookup(pager)
	vm_pager_t	pager;
{
	register vm_object_hash_entry_t	entry;
	vm_object_t			object;

	vm_object_cache_lock();

	for (entry = TAILQ_FIRST(vm_object_hashtable[vm_object_hash(pager)]); entry != NULL; entry = TAILQ_NEXT(entry, hash_links)) {
		object = entry->object;
		if (object->pager == pager) {
			vm_object_lock(object);
			if (object->ref_count == 0) {
				TAILQ_REMOVE(&vm_object_cached_list, object, cached_list);
				vm_object_cached--;
			}
			object->ref_count++;
			vm_object_unlock(object);
			vm_object_cache_unlock();
			return(object);
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
	TAILQ_INSERT_TAIL(bucket, entry, hash_links);
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

	for (entry = TAILQ_FIRST(bucket); entry != NULL; entry = TAILQ_NEXT(entry, hash_links)) {
		object = entry->object;
		if (object->pager == pager) {
			TAILQ_REMOVE(bucket, entry, hash_links);
			free((caddr_t)entry, M_VMOBJHASH);
			break;
		}
	}
}
