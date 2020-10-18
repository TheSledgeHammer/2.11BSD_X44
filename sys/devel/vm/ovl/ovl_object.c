/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Copyright (c) 2020
 *	Martin Kelly. All rights reserved.
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
 *	@(#)vm_object.h	8.4 (Berkeley) 1/9/95
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/fnv_hash.h>

#include <devel/vm/include/vm_page.h>
#include <devel/vm/ovl/ovl_object.h>

struct ovl_object	kernel_ovl_object_store;
struct ovl_object	vm_ovl_object_store;

#define	OVL_OBJECT_HASH_COUNT	157

int		ovl_cache_max = 100;	/* can patch if necessary */
struct	ovl_object_hash_head 	ovl_object_hashtable[OVL_OBJECT_HASH_COUNT];

static void _ovl_object_allocate (vm_size_t, ovl_object_t);

/*
 *	ovl_object_init:
 *
 *	Initialize the OVL objects module.
 */
void
ovl_object_init(size)
	vm_size_t	size;
{
	register int	i;

	RB_INIT(&ovl_object_cached_tree);
	RB_INIT(&ovl_object_tree);
	ovl_object_count = 0;
	simple_lock_init(&ovl_cache_lock);
	simple_lock_init(&ovl_object_tree_lock);

	for (i = 0; i < OVL_OBJECT_HASH_COUNT; i++)
		RB_INIT(&ovl_object_hashtable[i]);

	kern_ovl_object = &kernel_object_store;
	_ovl_object_allocate(size, kern_ovl_object);

	vm_ovl_object = &vm_ovl_object_store;
	_ovl_object_allocate(size, vm_ovl_object);
}

/*
 *	ovl_object_allocate:
 *
 *	Returns a new object with the given size.
 */

ovl_object_t
ovl_object_allocate(size)
	vm_size_t	size;
{
	register ovl_object_t	result;

	result = (ovl_object_t)malloc((u_long)sizeof *result, M_OVLOBJ, M_WAITOK);

	_ovl_object_allocate(size, result);

	return(result);
}

static void
_ovl_object_allocate(size, object)
	vm_size_t				size;
	register ovl_object_t	object;
{
	ovl_object_lock_init(object);
	object->ovo_ref_count = 1;
	object->ovo_size = size;
	object->ovo_flags = OVL_OBJ_INTERNAL;

	/*
	 *	Object starts out read-write.
	 */

	simple_lock(&ovl_object_tree_lock);
	RB_INSERT(object_q, &ovl_object_tree, object);
	ovl_object_count++;
	//cnt.v_nzfod += atop(size);
	simple_unlock(&ovl_object_tree_lock);
}

/*
 *	ovl_object_reference:
 *
 *	Gets another reference to the given object.
 */
void
ovl_object_reference(object)
	register ovl_object_t	object;
{
	if (object == NULL)
		return;

	ovl_object_lock(object);
	object->ovo_ref_count++;
	ovl_object_unlock(object);
}

void
ovl_object_deallocate(object)
	register ovl_object_t object;
{
	ovl_object_t	temp;

	while (object != NULL) {

		/*
		 *	The cache holds a reference (uncounted) to
		 *	the object; we must lock it before removing
		 *	the object.
		 */

		ovl_object_cache_lock();

		/*
		 *	Lose the reference
		 */
		ovl_object_lock(object);
		if (--(object->ovo_ref_count) != 0) {

			/*
			 *	If there are still references, then
			 *	we are done.
			 */
			ovl_object_unlock(object);
			ovl_object_cache_unlock();
			return;
		}

		/*
		 *	See if this object can persist.  If so, enter
		 *	it in the cache, then deactivate all of its
		 *	pages.
		 */

		if (object->ovo_flags & OVL_OBJ_CANPERSIST) {

			RB_INSERT(object_q, &ovl_object_cached_tree, object);
			ovl_object_cached++;
			ovl_object_cache_unlock();

//			ovl_object_deactivate_pages(object);
			ovl_object_unlock(object);

			ovl_object_cache_trim();
			return;
		}

		/*
		 *	Make sure no one can look us up now.
		 */
		ovl_object_remove(object->ovo_index);
		ovl_object_cache_unlock();

		ovl_object_terminate(object);
		/* unlocks and deallocates object */
		object = temp;
	}
}

/*
 *	Trim the object cache to size.
 */
void
ovl_object_cache_trim()
{
	register ovl_object_t	object;

	ovl_object_cache_lock();
	while (ovl_object_cached > ovl_cache_max) {
		object = RB_FIRST(object_q, ovl_object_cached_tree);
		ovl_object_cache_unlock();

		if (object != ovl_object_lookup(object->ovo_index))
			panic("ovl_object_deactivate: I'm sooo confused.");

		ovl_object_cache_lock();
	}
	ovl_object_cache_unlock();
}

/* ovl object rbtree comparator */
int
ovl_object_rb_compare(obj1, obj2)
	struct ovl_object *obj1, *obj2;
{
	if(obj1->ovo_size < obj2->ovo_size) {
		return (-1);
	} else if(obj1->ovo_size > obj2->ovo_size) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(object_t, ovl_object, ovo_object_tree, ovl_object_rb_compare);
RB_GENERATE(object_t, ovl_object, ovo_object_tree, ovl_object_rb_compare);
RB_PROTOTYPE(ovl_object_hash_head, ovl_object_hash_entry, ovoe_hlinks, ovl_object_rb_compare);
RB_GENERATE(ovl_object_hash_head, ovl_object_hash_entry, ovoe_hlinks, ovl_object_rb_compare);

/*
 *	ovl_object_hash generates a hash the object/id pair.
 */
u_long
ovl_object_hash(object)
	ovl_object_t object;
{
    Fnv32_t hash1 = fnv_32_buf(&object, sizeof(&object), FNV1_32_INIT) % OVL_OBJECT_HASH_COUNT;
    Fnv32_t hash2 = (((unsigned long)object)%OVL_OBJECT_HASH_COUNT);
    return (hash1^hash2);
}

/*
 *	ovl_object_lookup looks in the object cache for an object with the
 *	specified hash index.
 */

ovl_object_t
ovl_object_lookup(index, object)
	u_long 			index;
	ovl_object_t 	object;
{
	struct ovl_object_hash_head	*bucket;
	register ovl_object_hash_entry_t	entry;

	bucket = &ovl_object_hashtable[ovl_object_hash(object)];
	ovl_object_cache_lock();

	for (entry = RB_FIRST(ovl_object_hash_head , bucket); entry != NULL; entry = RB_NEXT(ovl_object_hash_head, bucket, entry)) {
		object = entry->ovoe_object;
		if (object->ovo_index == index) {
			ovl_object_lock(object);
			if (object->ovo_ref_count == 0) {
				RB_REMOVE(object_q, &ovl_object_cached_tree, object);
				ovl_object_cached--;
			}
			object->ovo_ref_count++;
			ovl_object_unlock(object);
			ovl_object_cache_unlock();
			return (object);
		}
	}

	ovl_object_cache_unlock();
	return (NULL);
}

/*
 *	ovl_object_enter enters the specified object/index/id into
 *	the hash table.
 */
void
ovl_object_enter(object)
	ovl_object_t	object;
{
	struct ovl_object_hash_head	*bucket;
	register ovl_object_hash_entry_t entry;

	if (object == NULL)
		return;

	bucket = &ovl_object_hashtable[ovl_object_hash(object)];
	entry = (ovl_object_hash_entry_t)malloc((u_long)sizeof *entry, M_OVLOBJHASH, M_WAITOK);
	entry->ovoe_object = object;
	object->ovo_flags |= OVL_OBJ_CANPERSIST;

	ovl_object_cache_lock();
	RB_INSERT(ovl_object_hash_head, bucket, entry);
	ovl_object_cache_unlock();
}

/*
 *	ovl_object_remove:
 *
 *	Remove the entry from the hash table.
 *	Note:  This assumes that the object cache
 *	is locked.  XXX this should be fixed
 *	by reorganizing vm_object_deallocate.
 */
void
ovl_object_remove(object)
	register ovl_object_t	object;
{
	struct ovl_object_hash_head			*bucket;
	register ovl_object_hash_entry_t	entry;

	bucket = &ovl_object_hashtable[ovl_object_hash(object)];
	u_long index = ovl_object_hash(object);

	for(entry = RB_FIRST(ovl_object_hash_head, bucket); entry != NULL; entry = RB_NEXT(ovl_object_hash_head, bucket, entry)) {
		object = entry->ovoe_object;
		if(ovl_object_hash(entry->ovoe_object) == index) {
			RB_REMOVE(ovl_object_hash_head, bucket, entry);
			free((caddr_t)entry, M_OVLOBJHASH);
			break;
		}
	}
}

/*
 *	ovl_object_cache_clear removes all objects from the cache.
 */
void
ovl_object_cache_clear()
{
	register ovl_object_t	object;

	/*
	 *	Remove each object in the cache by scanning down the
	 *	list of cached objects.
	 */
	ovl_object_cache_lock();
	while ((object = RB_FIRST(object_q, ovl_object_cached_tree)) != NULL) {
		ovl_object_cache_unlock();

		/*
		 * Note: it is important that we use vm_object_lookup
		 * to gain a reference, and not vm_object_reference, because
		 * the logic for removing an object from the cache lies in
		 * lookup.
		 */
		if (object != ovl_object_lookup(object))
			panic("ovl_object_cache_clear: I'm sooo confused.");

		ovl_object_cache_lock();
	}
	ovl_object_cache_unlock();
}
