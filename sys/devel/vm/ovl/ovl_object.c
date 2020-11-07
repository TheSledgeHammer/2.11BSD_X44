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
#include <devel/vm/ovl/overlay.h>

struct ovl_object	overlay_object_store;
struct ovl_object	omem_object_store;

#define	OVL_OBJECT_HASH_COUNT	157

struct	ovl_object_hash_head 	ovl_object_hashtable[OVL_OBJECT_HASH_COUNT];
struct 	vobject_hash_head 		ovl_vobject_hashtable[OVL_OBJECT_HASH_COUNT];

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

	RB_INIT(&ovl_object_tree);
	ovl_object_count = 0;

	simple_lock_init(&ovl_object_tree_lock);

	for (i = 0; i < OVL_OBJECT_HASH_COUNT; i++)
		RB_INIT(&ovl_object_hashtable[i]);
		TAILQ_INIT(&ovl_vobject_hashtable[i]);

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

	result = (ovl_object_t)overlay_malloc((u_long)sizeof *result, OVL_OBJECT, M_WAITOK);

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
	object->ovo_flags = OBJ_INTERNAL;

	/*
	 *	Object starts out read-write.
	 */

	simple_lock(&ovl_object_tree_lock);
	RB_INSERT(object_t, &ovl_object_tree, object);
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
ovl_object_setoverlayer(object, overlay, overlay_offset)
	ovl_object_t	object;
	ovl_overlay_t	overlay;
	vm_offset_t		overlay_offset;
{
	ovl_object_lock(object);
	object->ovo_overlay = overlay;
	object->ovo_overlay_offset = overlay_offset;
	ovl_object_unlock(object);
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
ovl_object_hash(overlay)
	ovl_overlay_t	overlay;
{
    Fnv32_t hash1 = fnv_32_buf(&overlay, sizeof(&overlay), FNV1_32_INIT) % OVL_OBJECT_HASH_COUNT;
    Fnv32_t hash2 = (((unsigned long)overlay)%OVL_OBJECT_HASH_COUNT);
    return (hash1^hash2);
}

/*
 *	ovl_object_lookup looks in the object cache for an object with the
 *	specified hash index.
 */

ovl_object_t
ovl_object_lookup(overlay)
	ovl_overlay_t	overlay;
{
	struct ovl_object_hash_head			*bucket;
	register ovl_object_hash_entry_t	entry;
	ovl_object_t 						object;

	bucket = &ovl_object_hashtable[ovl_object_hash(overlay)];

	for (entry = RB_FIRST(ovl_object_hash_head , bucket); entry != NULL; entry = RB_NEXT(ovl_object_hash_head, bucket, entry)) {
		object = entry->ovoe_object;
		if (object->ovo_overlay == overlay) {
			ovl_object_lock(object);
			object->ovo_ref_count++;
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
ovl_object_enter(object, overlay)
	ovl_object_t	object;
	ovl_overlay_t	overlay;
{
	struct ovl_object_hash_head	*bucket;
	register ovl_object_hash_entry_t entry;

	if (object == NULL)
		return;
	if (overlay == NULL)
		return;

	bucket = &ovl_object_hashtable[ovl_object_hash(overlay)];
	entry = (ovl_object_hash_entry_t)overlay_malloc((u_long)sizeof *entry, OVL_OBJHASH, M_WAITOK);

	entry->ovoe_object = object;
	object->ovo_flags |= OBJ_CANPERSIST;

	RB_INSERT(ovl_object_hash_head, bucket, entry);
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
ovl_object_remove(overlay)
	register ovl_overlay_t	overlay;
{
	struct ovl_object_hash_head			*bucket;
	register ovl_object_hash_entry_t	entry;
	register ovl_object_t				object;

	bucket = &ovl_object_hashtable[ovl_object_hash(overlay)];

	for(entry = RB_FIRST(ovl_object_hash_head, bucket); entry != NULL; entry = RB_NEXT(ovl_object_hash_head, bucket, entry)) {
		object = entry->ovoe_object;
		if(object->ovo_overlay == overlay) {
			RB_REMOVE(ovl_object_hash_head, bucket, entry);
			free((caddr_t)entry, OVL_OBJHASH);
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
ovl_object_insert_vm_object(oobject, vobject)
    ovl_object_t oobject;
    vm_object_t vobject;
{
    struct vobject_hash_head  *vbucket;


    if(vobject == NULL) {
        return;
    }
    vbucket = &ovl_vobject_hashtable[ovl_vobject_hash(oobject, vobject)];
    oobject->ovo_vm_object = vobject;
    oobject->ovo_flags |= OVL_OBJ_VM_OBJ;

    TAILQ_INSERT_HEAD(vbucket, oobject, ovo_vobject_hlist);
    ovl_vobject_count++;
}

vm_object_t
ovl_object_lookup_vm_object(oobject, vobject)
	ovl_object_t 	oobject;
    vm_object_t 	vobject;
{
    struct vobject_hash_head *vbucket;

    vbucket = &ovl_vobject_hashtable[ovl_vobject_hash(oobject, vobject)];
    for(oobject = TAILQ_FIRST(vbucket); oobject != NULL; oobject = TAILQ_NEXT(oobject, ovo_vobject_hlist)) {
        if(oobject->ovo_vm_object == vobject) {
            return (vobject);
        }
    }
    return (NULL);
}

void
ovl_object_remove_vm_object(oobject, vobject)
	ovl_object_t oobject;
	vm_object_t  vobject;
{
    struct vobject_hash_head *vbucket;

    vbucket = &ovl_vobject_hashtable[ovl_vobject_hash(oobject, vobject)];
    for(oobject = TAILQ_FIRST(vbucket); oobject != NULL; oobject = TAILQ_NEXT(oobject, ovo_vobject_hlist)) {
        if(oobject->ovo_vm_object == vobject) {
            if(vobject != NULL) {
                TAILQ_REMOVE(vbucket, oobject, ovo_vobject_hlist);
            }
        }
    }
}
