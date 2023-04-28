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

#include <ovl/include/ovl.h>
#include <ovl/include/ovl_segment.h>
#include <ovl/include/ovl_page.h>

struct ovl_object	overlay_object_store;
struct ovl_object	omem_object_store;
ovl_object_t		overlay_object;
ovl_object_t		omem_object;

#define	OVL_OBJECT_HASH_COUNT	157

struct ovl_object_hash_head 	ovl_object_hashtable[OVL_OBJECT_HASH_COUNT];

struct ovl_object_rbt			ovl_object_tree;
long							ovl_object_count;
simple_lock_data_t				ovl_object_tree_lock;

struct ovl_vm_object_hash_head 		ovl_vm_object_hashtable[OVL_OBJECT_HASH_COUNT];
long							ovl_vm_object_count;
simple_lock_data_t				ovl_vm_object_hash_lock;

static void _ovl_object_allocate(vm_size_t, ovl_object_t);

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

	RB_INIT(&ovl_object_tree);
	ovl_object_count = 0;

	simple_lock_init(&ovl_object_tree_lock, "ovl_object_tree_lock");
	simple_lock_init(&ovl_vm_object_hash_lock, "ovl_vm_object_hash_lock");

	for (i = 0; i < OVL_OBJECT_HASH_COUNT; i++) {
		RB_INIT(&ovl_object_hashtable[i]);
		TAILQ_INIT(&ovl_vm_object_hashtable[i]);
	}

	overlay_object = &overlay_object_store;
	_ovl_object_allocate(size, overlay_object);

	omem_object = &omem_object_store;
	_ovl_object_allocate(size, omem_object);
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

	result = (ovl_object_t)overlay_malloc((u_long)sizeof *result, M_OVLOBJ, M_WAITOK);

	_ovl_object_allocate(size, result);

	return (result);
}

static void
_ovl_object_allocate(size, object)
	vm_size_t				size;
	register ovl_object_t	object;
{
	ovl_object_lock_init(object);
	object->ref_count = 1;
	object->segment_count = 0;
	object->page_count = 0;
	object->size = size;
	object->flags = OBJ_INTERNAL;

	/*
	 *	Object starts out read-write.
	 */
	object->pager = NULL;
	object->paging_offset = 0;

	ovl_object_tree_lock();
	RB_INSERT(ovl_object_rbt, &ovl_object_tree, object);
	ovl_object_count++;
	ovl_object_tree_unlock();
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
	if (object == NULL) {
		return;
	}

	ovl_object_lock(object);
	object->ref_count++;
	ovl_object_unlock(object);
}

void
ovl_object_deallocate(object)
	register ovl_object_t	object;
{
	ovl_object_t temp;

	while (object != NULL) {

		ovl_object_lock(object);
		if (--(object->ref_count) != 0) {
			ovl_object_unlock(object);
			return;
		}

		ovl_object_remove(object->pager);

		ovl_object_terminate(object);
		object = temp;
	}
}

void
ovl_object_terminate(object)
	register ovl_object_t	object;
{
	while (object->paging_in_progress) {
			ovl_object_sleep(object, object, FALSE);
			ovl_object_lock(object);
	}

	ovl_object_unlock(object);

	if (object->pager != NULL) {
		vm_pager_deallocate(object->pager);
	}

	ovl_object_tree_lock();
	RB_REMOVE(object_rbt, &ovl_object_tree, object);
	ovl_object_count--;
	ovl_object_tree_unlock();

	overlay_free((caddr_t)object, M_OVLOBJ);
}

void
ovl_object_setpager(object, pager, paging_offset, read_only)
	ovl_object_t 	object;
	vm_pager_t		pager;
	vm_offset_t		paging_offset;
	bool_t			read_only;
{
	ovl_object_lock(object);
	object->pager = pager;
	object->paging_offset = paging_offset;
	ovl_object_unlock(object);
}

/* ovl object rbtree comparator */
int
ovl_object_rb_compare(obj1, obj2)
	struct ovl_object *obj1, *obj2;
{
	if (obj1->size < obj2->size) {
		return (-1);
	} else if (obj1->size > obj2->size) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(ovl_object_rbt, ovl_object, object_tree, ovl_object_rb_compare);
RB_GENERATE(ovl_object_rbt, ovl_object, object_tree, ovl_object_rb_compare);
RB_PROTOTYPE(ovl_object_hash_head, ovl_object_hash_entry, hlinks, ovl_object_rb_compare);
RB_GENERATE(ovl_object_hash_head, ovl_object_hash_entry, hlinks, ovl_object_rb_compare);

/*
 *	ovl_object_hash generates a hash the object/id pair.
 */
u_long
ovl_object_hash(pager)
	vm_pager_t	pager;
{
    Fnv32_t hash1 = fnv_32_buf(&pager, sizeof(&pager), FNV1_32_INIT) % OVL_OBJECT_HASH_COUNT;
    Fnv32_t hash2 = (((unsigned long)pager)%OVL_OBJECT_HASH_COUNT);
    return (hash1^hash2);
}

/*
 *	ovl_object_lookup looks in the object cache for an object with the
 *	specified hash index.
 */

ovl_object_t
ovl_object_lookup(pager)
	vm_pager_t	pager;
{
	struct ovl_object_hash_head			*bucket;
	register ovl_object_hash_entry_t	entry;
	ovl_object_t 						object;

	bucket = &ovl_object_hashtable[ovl_object_hash(pager)];
	RB_FOREACH(entry, ovl_object_hash_head, bucket) {
		object = entry->object;
		if (object->pager == pager) {
			ovl_object_lock(object);
			object->ref_count++;
			ovl_object_unlock(object);
			return (object);
		}
	}
	return (NULL);
}

/*
 *	ovl_object_enter enters the specified object/index/id into
 *	the hash table.
 */
void
ovl_object_enter(object, pager)
	ovl_object_t	object;
	vm_pager_t		pager;
{
	struct ovl_object_hash_head	*bucket;
	register ovl_object_hash_entry_t entry;

	if (object == NULL) {
		return;
	}
	if (pager == NULL) {
		return;
	}

	bucket = &ovl_object_hashtable[ovl_object_hash(pager)];
	entry = (ovl_object_hash_entry_t)overlay_malloc((u_long)sizeof *entry, M_OVLOBJHASH, M_WAITOK);

	entry->object = object;
	object->flags |= OBJ_CANPERSIST;

	ovl_object_lock(object);
	RB_INSERT(ovl_object_hash_head, bucket, entry);
	ovl_object_unlock(object);
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
ovl_object_remove(pager)
	register vm_pager_t	pager;
{
	struct ovl_object_hash_head			*bucket;
	register ovl_object_hash_entry_t	entry;
	register ovl_object_t				object;

	bucket = &ovl_object_hashtable[ovl_object_hash(pager)];
	RB_FOREACH(entry, ovl_object_hash_head, bucket) {
		object = entry->object;
		if (object->pager == pager) {
			RB_REMOVE(ovl_object_hash_head, bucket, entry);
			overlay_free(entry, M_OVLOBJHASH);
			break;
		}
	}
}

/* vm objects */
u_long
ovl_vobject_hash(oobject, vobject)
	ovl_object_t 	oobject;
	vm_object_t 	vobject;
{
	Fnv32_t hash1 = fnv_32_buf(&oobject, sizeof(&oobject), FNV1_32_INIT) % OVL_OBJECT_HASH_COUNT;
	Fnv32_t hash2 = fnv_32_buf(&vobject, sizeof(&vobject), FNV1_32_INIT) % OVL_OBJECT_HASH_COUNT;
	return (hash1 ^ hash2);
}

void
ovl_object_enter_vm_object(oobject, vobject)
    ovl_object_t oobject;
    vm_object_t vobject;
{
    struct ovl_vm_object_hash_head  	*vbucket;

    if(vobject == NULL) {
        return;
    }

    vbucket = &ovl_vm_object_hashtable[ovl_vobject_hash(oobject, vobject)];
    oobject->vm_object = vobject;
    oobject->flags |= OVL_OBJ_VM_OBJ;

    ovl_vm_object_hash_lock();
    TAILQ_INSERT_HEAD(vbucket, oobject, vm_object_hlist);
    ovl_vm_object_hash_unlock();
    ovl_vm_object_count++;
}

vm_object_t
ovl_object_lookup_vm_object(oobject)
	ovl_object_t 	oobject;
{
	register vm_object_t 	vobject;
    struct ovl_vm_object_hash_head *vbucket;

    vbucket = &ovl_vm_object_hashtable[ovl_vobject_hash(oobject, vobject)];
    ovl_vm_object_hash_lock();
    TAILQ_FOREACH(oobject, vbucket, vm_object_hlist) {
    	if(vobject == TAILQ_NEXT(oobject, vm_object_hlist)->vm_object) {
    		vobject = TAILQ_NEXT(oobject, vm_object_hlist)->vm_object;
    		ovl_vm_object_hash_unlock();
    		return (vobject);
    	}
    }
    ovl_vm_object_hash_unlock();
    return (NULL);
}

void
ovl_object_remove_vm_object(vobject)
	vm_object_t  vobject;
{
	register ovl_object_t oobject;
    struct ovl_vm_object_hash_head *vbucket;

    vbucket = &ovl_vm_object_hashtable[ovl_vobject_hash(oobject, vobject)];
    TAILQ_FOREACH(oobject, vbucket, vm_object_hlist) {
       	if (vobject == TAILQ_NEXT(oobject, vm_object_hlist)->vm_object) {
       		vobject = TAILQ_NEXT(oobject, vm_object_hlist)->vm_object;
       		if (vobject != NULL) {
       			TAILQ_REMOVE(vbucket, oobject, vm_object_hlist);
       			ovl_vm_object_count--;
       		}
       	}
    }
}
