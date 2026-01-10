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
 *	@(#)vm_map.c	8.9 (Berkeley) 5/17/95
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
 *	Virtual memory mapping module.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mman.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>

/*
 *	Virtual memory maps provide for the mapping, protection,
 *	and sharing of virtual memory objects.  In addition,
 *	this module provides for an efficient virtual copy of
 *	memory from one map to another.
 *
 *	Synchronization is required prior to most operations.
 *
 *	Maps consist of an ordered doubly-linked list of simple
 *	entries; a single hint is used to speed up lookups.
 *
 *	In order to properly represent the sharing of virtual
 *	memory regions among maps, the map structure is bi-level.
 *	Top-level ("address") maps refer to regions of sharable
 *	virtual memory.  These regions are implemented as
 *	("sharing") maps, which then refer to the actual virtual
 *	memory objects.  When two address maps "share" memory,
 *	their top-level maps both have references to the same
 *	sharing map.  When memory is virtual-copied from one
 *	address map to another, the references in the sharing
 *	maps are actually copied -- no copying occurs at the
 *	virtual memory object level.
 *
 *	Since portions of maps are specified by start/end addreses,
 *	which may not align with existing map entries, all
 *	routines merely "clip" entries to these start/end values.
 *	[That is, an entry is split into two, bordering at a
 *	start or end value.]  Note that these clippings may not
 *	always be necessary (as the two resulting entries are then
 *	not changed); however, the clipping is done for convenience.
 *	No attempt is currently made to "glue back together" two
 *	abutting entries.
 *
 *	As mentioned above, virtual copy operations are performed
 *	by copying VM object references from one sharing map to
 *	another, and then marking both regions as copy-on-write.
 *	It is important to note that only one writeable reference
 *	to a VM object region exists in any map -- this means that
 *	shadow object creation can be delayed until a write operation
 *	occurs.
 */

#undef RB_AUGMENT
static void vm_rb_augment(vm_map_entry_t);
#define	RB_AUGMENT(x)	vm_rb_augment(x)

static int 		vm_rb_compare(vm_map_entry_t, vm_map_entry_t);
static vm_size_t vm_rb_space(const vm_map_t, const vm_map_entry_t);
static vm_size_t vm_rb_subtree_space(vm_map_entry_t);
static void 	vm_rb_fixup(vm_map_t, vm_map_entry_t);
static void 	vm_rb_insert(vm_map_t, vm_map_entry_t);
static void 	vm_rb_remove(vm_map_t, vm_map_entry_t);
static vm_size_t vm_cl_space(const vm_map_t, const vm_map_entry_t);
static void 	vm_cl_insert(vm_map_t, vm_map_entry_t);
static void 	vm_cl_remove(vm_map_t, vm_map_entry_t);
static bool_t 	vm_map_search_next_entry(vm_map_t, vm_offset_t, vm_map_entry_t);
static bool_t 	vm_map_search_prev_entry(vm_map_t, vm_offset_t, vm_map_entry_t);
static void		_vm_map_clip_end(vm_map_t, vm_map_entry_t, vm_offset_t);
static void		_vm_map_clip_start(vm_map_t, vm_map_entry_t, vm_offset_t);

RB_PROTOTYPE(vm_map_rb_tree, vm_map_entry, rb_entry, vm_rb_compare);
RB_GENERATE(vm_map_rb_tree, vm_map_entry, rb_entry, vm_rb_compare);

/*
 *	vm_map_startup:
 *
 *	Initialize the vm_map module.  Must be called before
 *	any other vm_map routines.
 *
 *	Map and entry structures are allocated from the general
 *	purpose memory pool with some exceptions:
 *
 *	- The kernel map and kmem submap are allocated statically.
 *	- Kernel map entries are allocated out of a static pool.
 *
 *	These restrictions are necessary since malloc() uses the
 *	maps and requires map entries.
 */

vm_map_t 						kmap_free;
vm_map_entry_t 					kentry_free;
static struct vm_map			kmap_init[MAX_KMAP];
static struct vm_map_entry		kentry_init[MAX_KMAPENT];

void
vm_map_startup(void)
{
    kmap_free 	= vm_pmap_bootinit(kmap_init, sizeof(struct vm_map), MAX_KMAP);
    kentry_free	= vm_pmap_bootinit(kentry_init, sizeof(struct vm_map_entry), MAX_KMAPENT);
}

/*
 * Allocate a vmspace structure, including a vm_map and pmap,
 * and initialize those structures.  The refcnt is set to 1.
 * The remaining fields must be initialized by the caller.
 */
struct vmspace *
vmspace_alloc(min, max, pageable)
	vm_offset_t min, max;
	int pageable;
{
	register struct vmspace *vm;

	MALLOC(vm, struct vmspace *, sizeof(struct vmspace), M_VMMAP, M_WAITOK);
	bzero(vm, (caddr_t) &vm->vm_startcopy - (caddr_t) vm);
	vm_map_init(&vm->vm_map, min, max, pageable);
	pmap_pinit(&vm->vm_pmap);
	vm->vm_map.pmap = &vm->vm_pmap;		/* XXX */
	vm->vm_refcnt = 1;
	return (vm);
}

void
vmspace_free(vm)
	register struct vmspace *vm;
{
	if (--vm->vm_refcnt == 0) {
		/*
		 * Lock the map, to wait out all other references to it.
		 * Delete all of the mappings and pages they hold,
		 * then call the pmap module to reclaim anything left.
		 */
		vm_map_lock(&vm->vm_map);
		(void) vm_map_delete(&vm->vm_map, vm->vm_map.min_offset, vm->vm_map.max_offset);
		pmap_release(&vm->vm_pmap);
		FREE(vm, M_VMMAP);
	}
}

/*
 * Red black tree functions
 *
 * The caller must hold the related map lock.
 */
static int
vm_rb_compare(a, b)
	vm_map_entry_t a, b;
{
	if (a->start < b->start)
		return (-1);
	else if (a->start > b->start)
		return (1);
	return (0);
}

static void
vm_rb_augment(entry)
	vm_map_entry_t entry;
{
	entry->space = vm_rb_subtree_space(entry);
}

static vm_size_t
vm_rb_space(map, entry)
    const vm_map_t map;
    const vm_map_entry_t entry;
{
    KASSERT(CIRCLEQ_NEXT(entry, cl_entry) != NULL);
    return (CIRCLEQ_NEXT(entry, cl_entry)->start - CIRCLEQ_FIRST(&map->cl_header)->end);
}

static vm_size_t
vm_rb_subtree_space(entry)
	const vm_map_entry_t entry;
{
	vm_offset_t space, tmp;

	space = entry->ownspace;
	if (RB_LEFT(entry, rb_entry)) {
		tmp = RB_LEFT(entry, rb_entry)->space;
		if (tmp > space) {
			space = tmp;
		}
	}

	if (RB_RIGHT(entry, rb_entry)) {
		tmp = RB_RIGHT(entry, rb_entry)->space;
		if (tmp > space) {
			space = tmp;
		}
	}

	return (space);
}

static void
vm_rb_fixup(map, entry)
	vm_map_t map;
	vm_map_entry_t entry;
{
	/* We need to traverse to the very top */
	do {
		entry->ownspace = vm_rb_space(map, entry);
		entry->space = vm_rb_subtree_space(entry);
	} while ((entry = RB_PARENT(entry, rb_entry)) != NULL);
}

static void
vm_rb_insert(map, entry)
    vm_map_t map;
	vm_map_entry_t entry;
{
	vm_offset_t space;
    vm_map_entry_t tmp;

    space = vm_rb_space(map, entry);
    entry->ownspace = entry->space = space;
    tmp = RB_INSERT(vm_map_rb_tree, &(map)->rb_root, entry);
#ifdef DIAGNOSTIC
    if (tmp != NULL) {
		panic("vm_rb_insert: duplicate entry?");
    }
#endif
    vm_rb_fixup(map, entry);
    if (CIRCLEQ_PREV(entry, cl_entry) != RB_ROOT(&map->rb_root)) {
        vm_rb_fixup(map, CIRCLEQ_PREV(entry, cl_entry));
    }
}

static void
vm_rb_remove(map, entry)
    vm_map_t map;
	vm_map_entry_t entry;
{
    vm_map_entry_t parent;

    parent = RB_PARENT(entry, rb_entry);
    RB_REMOVE(vm_map_rb_tree, &(map)->rb_root, entry);
    if (CIRCLEQ_PREV(entry, cl_entry) != CIRCLEQ_FIRST(&map->cl_header)) {
        vm_rb_fixup(map, CIRCLEQ_PREV(entry, cl_entry));
    }
    if (parent) {
        vm_rb_fixup(map, parent);
    }
}

#ifdef DEBUG
int vm_debug_check_rbtree = 0;
#define vm_tree_sanity(x,y)			\
		if (vm_debug_check_rbtree)	\
			_vm_tree_sanity(x,y)
#else
#define vm_tree_sanity(x,y)
#endif

int
_vm_tree_sanity(map, name)
	vm_map_t map;
	const char *name;
{
	vm_map_entry_t tmp, trtmp;
	int n = 0, i = 1;

	RB_FOREACH(tmp, vm_map_rb_tree, &map->rb_root) {
		if (tmp->ownspace != vm_rb_space(map, tmp)) {
			printf("%s: %d/%d ownspace %lx != %lx %s\n",
			    name, n + 1, map->nentries, (u_long)tmp->ownspace, (u_long)vm_rb_space(map, tmp),
				CIRCLEQ_NEXT(tmp, cl_entry) == CIRCLEQ_FIRST(&map->cl_header) ? "(last)" : "");
			goto error;
		}
	}
	trtmp = NULL;
	RB_FOREACH(tmp, vm_map_rb_tree, &map->rb_root) {
		if (tmp->space != vm_rb_subtree_space(tmp)) {
			printf("%s: space %lx != %lx\n", name, (u_long)tmp->space, (u_long)vm_rb_subtree_space(tmp));
			goto error;
		}
		if (trtmp != NULL && trtmp->start >= tmp->start) {
			printf("%s: corrupt: 0x%lx >= 0x%lx\n", name, trtmp->start, tmp->start);
			goto error;
		}
		n++;

		trtmp = tmp;
	}

	if (n != map->nentries) {
		printf("%s: nentries: %d vs %d\n", name, n, map->nentries);
		goto error;
	}

	for (tmp = CIRCLEQ_FIRST(&map->cl_header)->cl_entry.cqe_next; tmp && tmp != CIRCLEQ_FIRST(&map->cl_header);
	    tmp = CIRCLEQ_NEXT(tmp, cl_entry), i++) {
		trtmp = RB_FIND(vm_map_rb_tree, &map->rb_root, tmp);
		if (trtmp != tmp) {
			printf("%s: lookup: %d: %p - %p: %p\n", name, i, tmp, trtmp, RB_PARENT(tmp, rb_entry));
			goto error;
		}
	}

	return (0);
error:
	return (-1);
}

/* Circular List Functions */
static vm_size_t
vm_cl_space(map, entry)
    const vm_map_t map;
    const vm_map_entry_t entry;
{
    vm_offset_t space, tmp;

    space = entry->ownspace;
    if(CIRCLEQ_FIRST(&map->cl_header)) {
        tmp = CIRCLEQ_FIRST(&map->cl_header)->space;
        if(tmp > space) {
            space = tmp;
        }
    }

    if(CIRCLEQ_LAST(&map->cl_header)) {
        tmp = CIRCLEQ_LAST(&map->cl_header)->space;
        if(tmp > space) {
            space = tmp;
        }
    }

    return (space);
}

static void
vm_cl_insert(map, entry)
	vm_map_t map;
	vm_map_entry_t entry;
{
	vm_map_entry_t head, tail;
    head = CIRCLEQ_FIRST(&map->cl_header);
    tail = CIRCLEQ_LAST(&map->cl_header);

    vm_offset_t space = vm_rb_space(map, entry);
    entry->ownspace = entry->space = space;
    if (head->space == vm_cl_space(map, entry)) {
        CIRCLEQ_INSERT_HEAD(&map->cl_header, head, cl_entry);
    }
    if (tail->space == vm_cl_space(map, entry)) {
        CIRCLEQ_INSERT_TAIL(&map->cl_header, tail, cl_entry);
    }
}

static void
vm_cl_remove(map, entry)
	vm_map_t map;
    vm_map_entry_t entry;
{
    vm_map_entry_t head, tail;
    head = CIRCLEQ_FIRST(&map->cl_header);
    tail = CIRCLEQ_LAST(&map->cl_header);

    if (head && vm_cl_space(map, entry)) {
        CIRCLEQ_REMOVE(&map->cl_header, head, cl_entry);
    }
    if (tail && vm_cl_space(map, entry)) {
        CIRCLEQ_REMOVE(&map->cl_header, tail, cl_entry);
    }
}

/*
 *	vm_map_create:
 *
 *	Creates and returns a new empty VM map with
 *	the given physical map structure, and having
 *	the given lower and upper address bounds.
 */
vm_map_t
vm_map_create(pmap, min, max, pageable)
	pmap_t		pmap;
	vm_offset_t	min, max;
	bool_t		pageable;
{
	register vm_map_t	result;
	vm_map_entry_t 		kentry;
	extern vm_map_t		kmem_map;

	if (kmem_map == NULL) {
		result = kmap_free;
		if (result == NULL) {
			panic("vm_map_create: out of maps");
		}
		kentry = CIRCLEQ_FIRST(&result->cl_header);
		kmap_free = (vm_map_t)CIRCLEQ_NEXT(kentry, cl_entry);
	} else {
		MALLOC(result, struct vm_map *, sizeof(struct vm_map *), M_VMMAP, M_WAITOK);
	}

	vm_map_init(result, min, max, pageable);
	result->pmap = pmap;
	return (result);
}

/*
 * Initialize an existing vm_map structure
 * such as that in the vmspace structure.
 * The pmap is set elsewhere.
 */
void
vm_map_init(map, min, max, pageable)
	register vm_map_t map;
	vm_offset_t	min, max;
	bool_t	pageable;
{
	CIRCLEQ_INIT(&map->cl_header);
	RB_INIT(&map->rb_root);
	map->nentries = 0;
	map->size = 0;
	map->ref_count = 1;
	map->is_main_map = TRUE;
	map->min_offset = min;
	map->max_offset = max;
	map->entries_pageable = pageable;
	map->first_free = CIRCLEQ_FIRST(&map->cl_header);
	map->hint = CIRCLEQ_FIRST(&map->cl_header);
	map->timestamp = 0;
	lockinit(&map->lock, PVM, "thrd_sleep", 0, 0);
	simple_lock_init(&map->ref_lock, "vm_map_ref_lock");
	simple_lock_init(&map->hint_lock, "vm_map_hint_lock");
}

#ifdef DEBUG
void
vm_map_entry_isspecial(map)
	vm_map_t map;
{
	bool_t isspecial;

	isspecial = (map == kernel_map || map == kmem_map || map == mb_map || map == pager_map);
	if ((isspecial && map->entries_pageable) || (!isspecial && !map->entries_pageable)) {
		panic("vm_map_entry_dispose: bogus map");
	}
}
#endif

/*
 *	vm_map_entry_create:	[ internal use only ]
 *
 *	Allocates a VM map entry for insertion.
 *	No entry fields are filled in.  This routine is
 */
vm_map_entry_t
vm_map_entry_create(map)
	vm_map_t	map;
{
	vm_map_entry_t	entry;

#ifdef DEBUG
	vm_map_entry_isspecial(map);
#endif
	if (map->entries_pageable) {
		MALLOC(entry, struct vm_map_entry *, sizeof(struct vm_map_entry *), M_VMMAPENT, M_WAITOK);
	} else {
		if (entry == kentry_free) {
			kentry_free = CIRCLEQ_NEXT(kentry_free, cl_entry);
		}
	}
	if (entry == NULL) {
		panic("vm_map_entry_create: out of map entries");
	}

	return (entry);
}

/*
 *	vm_map_entry_dispose:	[ internal use only ]
 *
 *	Inverse of vm_map_entry_create.
 */
void
vm_map_entry_dispose(map, entry)
	vm_map_t	map;
	vm_map_entry_t	entry;
{
	if (map->entries_pageable) {
		FREE(entry, M_VMMAPENT);
	} else {
		CIRCLEQ_NEXT(entry, cl_entry) = kentry_free;
		kentry_free = entry;
	}
}

/*
 *	vm_map_entry_{un,}link:
 *
 *	Insert/remove entries from maps.
 */
static void
vm_map_entry_link(map, after_where, entry)
	vm_map_t map;
	vm_map_entry_t after_where, entry;
{
	map->nentries++;
	CIRCLEQ_PREV(entry, cl_entry) = after_where;
	CIRCLEQ_NEXT(entry, cl_entry) = CIRCLEQ_NEXT(after_where, cl_entry);
	CIRCLEQ_PREV(entry, cl_entry)->cl_entry.cqe_next = entry;
	CIRCLEQ_NEXT(entry, cl_entry)->cl_entry.cqe_prev = entry;
	vm_cl_insert(map, entry);
	vm_rb_insert(map, entry);
}

static void
vm_map_entry_unlink(map, entry)
	vm_map_t map;
	vm_map_entry_t entry;
{
	map->nentries--;
	vm_cl_remove(map, entry);
	vm_rb_remove(map, entry);
}

/*
 *	vm_map_reference:
 *
 *	Creates another valid reference to the given map.
 *
 */
void
vm_map_reference(map)
	register vm_map_t	map;
{
	if (map == NULL)
		return;

	simple_lock(&map->ref_lock);
#ifdef DEBUG
	if (map->ref_count == 0) {
		panic("vm_map_reference: zero ref_count");
	}
#endif
	map->ref_count++;
	simple_unlock(&map->ref_lock);
}

/*
 *	vm_map_deallocate:
 *
 *	Removes a reference from the specified map,
 *	destroying it if no references remain.
 *	The map should not be locked.
 */
void
vm_map_deallocate(map)
	register vm_map_t	map;
{

	if (map == NULL)
		return;

	simple_lock(&map->ref_lock);
	if (--map->ref_count > 0) {
		simple_unlock(&map->ref_lock);
		return;
	}

	/*
	 *	Lock the map, to wait out all other references
	 *	to it.
	 */

	vm_map_lock_drain_interlock(map);

	(void)vm_map_delete(map, map->min_offset, map->max_offset);

	pmap_destroy(map->pmap);

	vm_map_unlock(map);

	FREE(map, M_VMMAP);
}

/*
 *	vm_map_insert:
 *
 *	Inserts the given whole VM object into the target
 *	map at the specified address range.  The object's
 *	size should match that of the address range.
 *
 *	Requires that the map be locked, and leaves it so.
 */
int
vm_map_insert(map, object, offset, start, end)
	vm_map_t	map;
	vm_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	start;
	vm_offset_t	end;
{
	register vm_map_entry_t		new_entry;
	register vm_map_entry_t		prev_entry;
	vm_map_entry_t				temp_entry;
	int 						error, advice;

	advice = VM_ADVICE(4);
	vm_tree_sanity(map, "map insert");

	/*
	 *	Check that the start and end points are not bogus.
	 */

	if ((start < map->min_offset) || (end > map->max_offset) ||
			(start >= end)) {
		return (KERN_INVALID_ADDRESS);
	}

	/*
	 *	Find the entry prior to the proposed
	 *	starting address; if it's part of an
	 *	existing entry, this range is bogus.
	 */

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		return (KERN_NO_SPACE);
	}

	prev_entry = temp_entry;

	/*
	 *	Assert that the next entry doesn't overlap the
	 *	end point.
	 */

	 if((CIRCLEQ_NEXT(prev_entry, cl_entry) != CIRCLEQ_FIRST(&map->cl_header)) && (CIRCLEQ_NEXT(prev_entry, cl_entry)->start < end)) {
		 return (KERN_NO_SPACE);
	 }

	/*
	 *	See if we can avoid creating a new entry by
	 *	extending one of our neighbors.
	 */
	if (object == NULL) {
		if ((prev_entry != CIRCLEQ_FIRST(&map->cl_header)) &&
		    (prev_entry->end == start) &&
		    (map->is_main_map) &&
		    (prev_entry->is_a_map == FALSE) &&
		    (prev_entry->is_sub_map == FALSE) &&
		    (prev_entry->inheritance == VM_INHERIT_DEFAULT) &&
		    (prev_entry->protection == VM_PROT_DEFAULT) &&
		    (prev_entry->max_protection == VM_PROT_DEFAULT) &&
			(prev_entry->advice != advice) &&
		    (prev_entry->wired_count == 0) &&
			(prev_entry->aref.ar_amap &&
					amap_refs(prev_entry->aref.ar_amap) != 1)) {
			error = vm_object_coalesce(prev_entry->object.vm_object, NULL,
					prev_entry->offset, (vm_offset_t) 0,
					(vm_size_t) (prev_entry->end - prev_entry->start),
					(vm_size_t) (end - prev_entry->end));
			if (error) {
				if (prev_entry->aref.ar_amap) {
					vm_amap_extend(prev_entry, map->size);
				} else if (CIRCLEQ_NEXT(prev_entry, cl_entry)->aref.ar_amap) {
					vm_amap_extend(CIRCLEQ_NEXT(prev_entry, cl_entry), prev_entry->end - prev_entry->start);
				} else {
					vm_amap_extend(CIRCLEQ_NEXT(prev_entry, cl_entry), map->size);
				}

				/*
				 *	Coalesced the two objects - can extend
				 *	the previous map entry to include the
				 *	new range.
				 */
				map->size += (end - prev_entry->end);
				prev_entry->end = end;

				vm_rb_fixup(map, prev_entry);
				vm_tree_sanity(map, " map insert");
				return (KERN_SUCCESS);
			}
		}
	}

	/*
	 *	Create a new entry
	 */

	new_entry = vm_map_entry_create(map);
	new_entry->start = start;
	new_entry->end = end;

	new_entry->is_a_map = FALSE;
	new_entry->is_sub_map = FALSE;
	new_entry->object.vm_object = object;
	new_entry->offset = offset;

	new_entry->copy_on_write = FALSE;
	new_entry->needs_copy = FALSE;

	if (map->is_main_map) {
		vm_amap_t 	amap;
		vm_offset_t to_add;

		new_entry->inheritance = VM_INHERIT_DEFAULT;
		new_entry->protection = VM_PROT_DEFAULT;
		new_entry->max_protection = VM_PROT_DEFAULT;
		new_entry->wired_count = 0;
		new_entry->advice = advice;

		to_add = VM_AMAP_CHUNK << PAGE_SHIFT;
		amap = vm_amap_alloc(map->size, to_add, M_WAITOK);
		new_entry->aref.ar_pageoff = 0;
		new_entry->aref.ar_amap = amap;
	}  else {
		new_entry->aref.ar_amap = NULL;
	}

	/*
	 *	Insert the new entry into the list
	 */

	vm_map_entry_link(map, prev_entry, new_entry);
	map->size += new_entry->end - new_entry->start;

	/*
	 *	Update the free space hint
	 */

	if ((map->first_free == prev_entry) && (prev_entry->end >= new_entry->start)) {
		map->first_free = new_entry;
	}

	return (KERN_SUCCESS);
}

/*
 *	SAVE_HINT:
 *
 *	Saves the specified entry as the hint for
 *	future lookups.  Performs necessary interlocks.
 */
#define	SAVE_HINT(map,value) 				\
		simple_lock(&(map)->hint_lock); 	\
		(map)->hint = (value); 				\
		simple_unlock(&(map)->hint_lock);

/*
 *	vm_map_lookup_entry:	[ internal use only ]
 *
 *	Finds the map entry containing (or
 *	immediately preceding) the specified address
 *	in the given map; the entry is returned
 *	in the "entry" parameter.  The boolean
 *	result indicates whether the address is
 *	actually contained in the map.
 */
bool_t
vm_map_lookup_entry(map, address, entry)
	register vm_map_t		map;
	register vm_offset_t	address;
	vm_map_entry_t			*entry;		/* OUT */
{
	register vm_map_entry_t		cur;
	register vm_map_entry_t		last;
    bool_t use_tree = FALSE;

	/*
	 *	Start looking either from the head or tail of the
	 *	list, or from the hint.
	 */

	simple_lock(&map->hint_lock);
	cur = map->hint;
	simple_unlock(&map->hint_lock);

	if(address < cur->start) {
		if (vm_map_search_prev_entry(map, address, cur)) {
			SAVE_HINT(map, cur);
			KDASSERT((*entry)->start <= address);
	        KDASSERT(address < (*entry)->end);
	        return (TRUE);
		} else {
			use_tree = TRUE;
	        goto search_tree;
		}
	}
	if (address > cur->start){
		if(vm_map_search_next_entry(map, address, cur)) {
			SAVE_HINT(map, cur);
	        KDASSERT((*entry)->start <= address);
	        KDASSERT(address < (*entry)->end);
	        return (TRUE);
		} else {
			use_tree = TRUE;
			goto search_tree;
		}
	}

search_tree:

    vm_tree_sanity(map, __func__);

    if (use_tree) {
    	struct vm_map_entry *prev = CIRCLEQ_FIRST(&map->cl_header);
        cur = RB_ROOT(&map->rb_root);
        while (cur) {
        	if (address >= cur->start) {
        		if (address < cur->end) {
        			*entry = cur;
                    SAVE_HINT(map, cur);
                    KDASSERT((*entry)->start <= address);
                    KDASSERT(address < (*entry)->end);
                    return (TRUE);
                }
                prev = cur;
                cur = RB_RIGHT(cur, rb_entry);
            } else {
                cur = RB_LEFT(cur, rb_entry);
            }
        }
        *entry = prev;
        goto failed;
    }

failed:
    SAVE_HINT(map, *entry);
    KDASSERT((*entry) == CIRCLEQ_FIRST(&map->cl_header) || (*entry)->end <= address);
    KDASSERT(CIRCLEQ_NEXT(*entry, cl_entry) == CIRCLEQ_FIRST(&map->cl_header) || address < CIRCLEQ_NEXT(*entry, cl_entry)->start);
    return (FALSE);
}

static bool_t
vm_map_search_next_entry(map, address, entry)
    register vm_map_t	    map;
    register vm_offset_t	address;
    vm_map_entry_t		    entry;		/* OUT */
{
    register vm_map_entry_t	cur;
    register vm_map_entry_t	first = CIRCLEQ_FIRST(&map->cl_header);
    register vm_map_entry_t	last = CIRCLEQ_LAST(&map->cl_header);

    CIRCLEQ_FOREACH(entry, &map->cl_header, cl_entry) {
        if (address >= first->start) {
            cur = CIRCLEQ_NEXT(first, cl_entry);
            if ((cur != last) && (cur->end > address)) {
                cur = entry;
                SAVE_HINT(map, cur);
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

static bool_t
vm_map_search_prev_entry(map, address, entry)
    register vm_map_t	    map;
    register vm_offset_t	address;
    vm_map_entry_t		    entry;		/* OUT */
{
    register vm_map_entry_t	cur;
    register vm_map_entry_t	first = CIRCLEQ_FIRST(&map->cl_header);
    register vm_map_entry_t	last = CIRCLEQ_LAST(&map->cl_header);

    CIRCLEQ_FOREACH_REVERSE(entry, &map->cl_header, cl_entry) {
        if (address <= last->start) {
            cur = CIRCLEQ_PREV(last, cl_entry);
            if ((cur != first) && (cur->end > address)) {
                cur = entry;
                SAVE_HINT(map, cur);
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

/*
 * Find sufficient space for `length' bytes in the given map, starting at
 * `start'.  The map must be locked.  Returns 0 on success, 1 on no space.
 */
int
vm_map_findspace(map, start, length, addr)
	register vm_map_t map;
	register vm_offset_t start;
	vm_size_t length;
	vm_offset_t *addr;
{
	register vm_map_entry_t entry, next;
	register vm_offset_t end;
	vm_map_entry_t tmp;

	if (start < map->min_offset)
		start = map->min_offset;
	if (start > map->max_offset)
		return (1);

	/*
	 * Look for the first possible address; if there's already
	 * something at this address, we have to start after it.
	 */
	if (start == map->min_offset) {
		if ((entry = map->first_free) != CIRCLEQ_FIRST(&map->cl_header)) {
			start = entry->end;
		}
	} else {
		if (vm_map_lookup_entry(map, start, &tmp)) {
			start = tmp->end;
		}
		entry = tmp;
	}


	/* If there is not enough space in the whole tree, we fail */
	tmp = RB_ROOT(&map->rb_root);
	if (tmp == NULL || tmp->space < length) {
		return (1);
	}

	/*
	 * Look through the rest of the map, trying to fit a new region in
	 * the gap between existing regions, or after the very last region.
	 */
	for (;; start = (entry = next)->end) {
		/*
		 * Find the end of the proposed new region.  Be sure we didn't
		 * go beyond the end of the map, or wrap around the address;
		 * if so, we lose.  Otherwise, if this is the last entry, or
		 * if the proposed new region fits before the next entry, we
		 * win.
		 */
		end = start + length;
		if (end > map->max_offset || end < start) {
			return (1);
		}
		next = CIRCLEQ_NEXT(entry, cl_entry);
		if (next == CIRCLEQ_FIRST(&map->cl_header) || next->start >= end) {
			break;
		}
	}
	SAVE_HINT(map, entry);
	*addr = start;
	return (0);
}

/*
 *	vm_map_find finds an unallocated region in the target address
 *	map with the given length.  The search is defined to be
 *	first-fit from the specified address; the region found is
 *	returned in the same parameter.
 *
 */
int
vm_map_find(map, object, offset, addr, length, find_space)
	vm_map_t	map;
	vm_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	*addr;		/* IN/OUT */
	vm_size_t	length;
	bool_t	find_space;
{
	register vm_offset_t	start;
	int			result;

	start = *addr;
	vm_map_lock(map);
	if (find_space) {
		if (vm_map_findspace(map, start, length, addr)) {
			vm_map_unlock(map);
			return (KERN_NO_SPACE);
		}
		start = *addr;
	}
	result = vm_map_insert(map, object, offset, start, start + length);
	vm_map_unlock(map);
	return (result);
}

/*
 *	vm_map_simplify_entry:	[ internal use only ]
 *
 *	Simplify the given map entry by:
 *		removing extra sharing maps
 *		[XXX maybe later] merging with a neighbor
 */
void
vm_map_simplify_entry(map, entry)
	vm_map_t	map;
	vm_map_entry_t	entry;
{
	vm_map_entry_t next, prev;
	vm_size_t prevsize, esize;

	/*
	 *	If this entry corresponds to a sharing map, then
	 *	see if we can remove the level of indirection.
	 *	If it's not a sharing map, then it points to
	 *	a VM object, so see if we can merge with either
	 *	of our neighbors.
	 */

	if (entry->is_sub_map) {
		return;
	}

	prev = CIRCLEQ_PREV(entry, cl_entry);
	if (entry != CIRCLEQ_FIRST(&map->cl_header)) {
		prevsize = prev->end - prev->start;
		if ((prev->end == entry->start) &&
				(prev->object.vm_object == entry->object.vm_object) &&
				(!prev->object.vm_object ||
						(prev->offset + prevsize == entry->offset)) &&
						(prev->protection == entry->protection) &&
						(prev->max_protection == entry->max_protection) &&
						(prev->inheritance == entry->inheritance) &&
						(prev->wired_count == entry->wired_count)) {
			if (map->first_free == prev) {
				map->first_free = entry;
			}
			if (map->hint == prev) {
				map->hint = entry;
			}
			vm_map_entry_unlink(map, prev);
			entry->start = prev->start;
			entry->offset = prev->offset;
			if (prev->object.vm_object) {
				vm_object_deallocate(prev->object.vm_object);
			}
			vm_map_entry_dispose(map, prev);
		}
	}

	next = CIRCLEQ_NEXT(entry, cl_entry);
	if (entry != CIRCLEQ_FIRST(&map->cl_header)) {
		esize = entry->end - entry->start;
		if ((entry->end == next->start) &&
				(next->object.vm_object == entry->object.vm_object) &&
				(!entry->object.vm_object ||
						(entry->offset + esize == next->offset)) &&
						(next->protection == entry->protection) &&
						(next->max_protection == entry->max_protection) &&
						(next->inheritance == entry->inheritance) &&
						(next->wired_count == entry->wired_count)) {
			if (map->first_free == next){
				map->first_free = entry;
			}
			if (map->hint == next) {
				map->hint = entry;
			}
			vm_map_entry_unlink(map, next);
			entry->end = next->end;
			if (next->object.vm_object) {
				vm_object_deallocate(next->object.vm_object);
			}
			vm_map_entry_dispose(map, next);
		}
	}
}

/*
 *	vm_map_clip_start:	[ internal use only ]
 *
 *	Asserts that the given entry begins at or after
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */

void
vm_map_clip_start(map, entry, startaddr)
	register vm_map_t		map;
	register vm_map_entry_t	entry;
	register vm_offset_t	startaddr;
{
	if (startaddr > entry->start) {
		_vm_map_clip_start(map, entry, startaddr);
	}
}

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */
static void
_vm_map_clip_start(map, entry, start)
	register vm_map_t		map;
	register vm_map_entry_t	entry;
	register vm_offset_t	start;
{
	register vm_map_entry_t	new_entry;
	vm_offset_t new_adj;

	/*
	 *	See if we can simplify this entry first
	 */

	vm_map_simplify_entry(map, entry);
	vm_tree_sanity(map, "clip_start entry");

	/*
	 *	Split off the front portion --
	 *	note that we must insert the new
	 *	entry BEFORE this one, so that
	 *	this entry has the specified starting
	 *	address.
	 */

	new_entry = vm_map_entry_create(map);
	*new_entry = *entry;

	new_entry->end = start;
	new_adj = start - new_entry->start;
	if (entry->object.vm_object) {
		entry->offset += new_adj;	/* shift start over */
	} else {
		entry->offset += (start - entry->start);
	}

	/* Does not change order for the RB tree */
	entry->start = start;

	if (new_entry->aref.ar_amap) {
		vm_amap_splitref(&new_entry->aref, &entry->aref, new_adj);
	}

	vm_map_entry_link(map, CIRCLEQ_PREV(entry, cl_entry), new_entry);

	if (entry->is_a_map || entry->is_sub_map) {
	 	vm_map_reference(new_entry->object.share_map);
	} else {
		vm_object_reference(new_entry->object.vm_object);
	}

	vm_tree_sanity(map, "clip_start leave");
}

/*
 *	vm_map_clip_end:	[ internal use only ]
 *
 *	Asserts that the given entry ends at or before
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void
vm_map_clip_end(map, entry, endaddr)
	register vm_map_t		map;
	register vm_map_entry_t	entry;
	register vm_offset_t	endaddr;
{
	if (endaddr < entry->end) {
		_vm_map_clip_end(map, entry, endaddr);
	}
}

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */
static void
_vm_map_clip_end(map, entry, end)
	register vm_map_t	map;
	register vm_map_entry_t	entry;
	register vm_offset_t	end;
{
	register vm_map_entry_t	new_entry;
	vm_offset_t new_adj;

	vm_tree_sanity(map, "clip_end entry");

	/*
	 *	Create a new entry and insert it
	 *	AFTER the specified entry
	 */

	new_entry = vm_map_entry_create(map);
	*new_entry = *entry;

	new_entry->start = entry->end = end;
	new_adj = end - entry->start;
	if (new_entry->object.vm_object) {
		new_entry->offset += new_adj;
	} else {
		new_entry->offset += (end - entry->start);
	}

	vm_rb_fixup(map, entry);

	if (entry->aref.ar_amap) {
		vm_amap_splitref(&entry->aref, &new_entry->aref, new_adj);
	}

	vm_map_entry_link(map, entry, new_entry);

	if (entry->is_a_map || entry->is_sub_map) {
	 	vm_map_reference(new_entry->object.share_map);
	} else {
		vm_object_reference(new_entry->object.vm_object);
	}

	vm_tree_sanity(map, "clip_end leave");
}

/*
 *	VM_MAP_RANGE_CHECK:	[ internal use only ]
 *
 *	Asserts that the starting and ending region
 *	addresses fall within the valid range of the map.
 */
#define	VM_MAP_RANGE_CHECK(map, start, end) {	\
		if (start < vm_map_min(map))			\
			start = vm_map_min(map);			\
		if (end > vm_map_max(map))				\
			end = vm_map_max(map);				\
		if (start > end)						\
			start = end;						\
}

/*
 *	vm_map_submap:		[ kernel use only ]
 *
 *	Mark the given range as handled by a subordinate map.
 *
 *	This range must have been created with vm_map_find,
 *	and no other operations may have been performed on this
 *	range prior to calling vm_map_submap.
 *
 *	Only a limited number of operations can be performed
 *	within this rage after calling vm_map_submap:
 *		vm_fault
 *	[Don't try vm_map_copy!]
 *
 *	To remove a submapping, one must first remove the
 *	range from the superior map, and then destroy the
 *	submap (if desired).  [Better yet, don't try it.]
 */
int
vm_map_submap(map, start, end, submap)
	register vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	vm_map_t				submap;
{
	vm_map_entry_t		entry;
	register int		result = KERN_INVALID_ARGUMENT;

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &entry)) {
		vm_map_clip_start(map, entry, start);
	} else {
		 entry = CIRCLEQ_NEXT(entry, cl_entry);
	}

	vm_map_clip_end(map, entry, end);

	if ((entry->start == start) && (entry->end == end) &&
			(!entry->is_a_map) &&
			(entry->object.vm_object == NULL) &&
			(entry->aref.ar_amap == NULL) &&
			(!entry->copy_on_write)) {
		entry->is_a_map = FALSE;
		entry->is_sub_map = TRUE;
		entry->offset = 0;
		vm_map_reference(entry->object.sub_map = submap);
		result = KERN_SUCCESS;
	}
	vm_map_unlock(map);

	return(result);
}

/*
 *	vm_map_protect:
 *
 *	Sets the protection of the specified address
 *	region in the target map.  If "set_max" is
 *	specified, the maximum protection is to be set;
 *	otherwise, only the current protection is affected.
 */
int
vm_map_protect(map, start, end, new_prot, set_max)
	register vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register vm_prot_t		new_prot;
	register bool_t		set_max;
{
	register vm_map_entry_t		current;
	vm_map_entry_t			entry;

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &entry)) {
		vm_map_clip_start(map, entry, start);
	} else {
		entry = CIRCLEQ_NEXT(entry, cl_entry);
	}

	/*
	 *	Make a first pass to check for protection
	 *	violations.
	 */

	current = entry;
	while ((current != CIRCLEQ_FIRST(&map->cl_header)) && (current->start < end)) {
		if (current->is_sub_map) {
			return (KERN_INVALID_ARGUMENT);
		}
		if ((new_prot & current->max_protection) != new_prot) {
			vm_map_unlock(map);
			return (KERN_PROTECTION_FAILURE);
		}
		current = CIRCLEQ_NEXT(entry, cl_entry);
	}

	/*
	 *	Go back and fix up protections.
	 *	[Note that clipping is not necessary the second time.]
	 */

	current = entry;

	while ((current != CIRCLEQ_FIRST(&map->cl_header)) && (current->start < end)) {
		vm_prot_t	old_prot;

		vm_map_clip_end(map, current, end);

		old_prot = current->protection;
		if (set_max) {
			current->protection =
				(current->max_protection = new_prot) &
					old_prot;
		} else {
			current->protection = new_prot;
		}

		/*
		 *	Update physical map if necessary.
		 *	Worry about copy-on-write here -- CHECK THIS XXX
		 */

		if (current->protection != old_prot) {

#define MASK(entry)	((entry)->copy_on_write ? ~VM_PROT_WRITE : VM_PROT_ALL)
#define	max(a,b)	((a) > (b) ? (a) : (b))

			if (current->is_a_map) {
				vm_map_entry_t	share_entry;
				vm_offset_t	share_end;

				vm_map_lock(current->object.share_map);
				(void) vm_map_lookup_entry(
						current->object.share_map,
						current->offset,
						&share_entry);
				share_end = current->offset +
					(current->end - current->start);
				while ((share_entry !=
					CIRCLEQ_FIRST(&current->object.share_map->cl_header)) &&
					(share_entry->start < share_end)) {

					pmap_protect(map->pmap,
						(max(share_entry->start,
							current->offset) -
							current->offset +
							current->start),
						min(share_entry->end,
							share_end) -
						current->offset +
						current->start,
						current->protection &
							MASK(share_entry));

					share_entry = CIRCLEQ_NEXT(share_entry, cl_entry);
				}
				vm_map_unlock(current->object.share_map);
			} else {
			 	pmap_protect(map->pmap, current->start,
					current->end,
					current->protection & MASK(entry));
			}
#undef	max
#undef	MASK
		}
		current = CIRCLEQ_NEXT(current, cl_entry);
	}

	vm_map_unlock(map);
	return (KERN_SUCCESS);
}

/*
 *	vm_map_inherit:
 *
 *	Sets the inheritance of the specified address
 *	range in the target map.  Inheritance
 *	affects how the map will be shared with
 *	child maps at the time of vm_map_fork.
 */
int
vm_map_inherit(map, start, end, new_inheritance)
	register vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register vm_inherit_t	new_inheritance;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t	temp_entry;

	switch (new_inheritance) {
	case VM_INHERIT_ZERO:
	case VM_INHERIT_DONATE_COPY:
	case VM_INHERIT_NONE:
	case VM_INHERIT_COPY:
	case VM_INHERIT_SHARE:
		break;
	default:
		return (KERN_INVALID_ARGUMENT);
	}

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		entry = temp_entry;
		vm_map_clip_start(map, entry, start);
	} else {
		entry = CIRCLEQ_NEXT(temp_entry, cl_entry);
	}

	while ((entry != CIRCLEQ_FIRST(&map->cl_header)) && (entry->start < end)) {
		vm_map_clip_end(map, entry, end);

		entry->inheritance = new_inheritance;

		entry = CIRCLEQ_NEXT(entry, cl_entry);
	}

	vm_map_unlock(map);
	return (KERN_SUCCESS);
}

/*
 * vm_map_advice: set advice code for range of addrs in map.
 *
 * => map must be unlocked
 */
int
vm_map_advice(map, start, end, new_advice)
	register vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	int 					new_advice;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t temp_entry;

	vm_map_lock(map);
	VM_MAP_RANGE_CHECK(map, start, end);

	if (vm_map_lookup_entry(map, start, &temp_entry)) {
		entry = temp_entry;
		vm_map_clip_start(map, entry, start);
	} else {
		entry = CIRCLEQ_NEXT(temp_entry, cl_entry);
	}

	while ((entry != CIRCLEQ_FIRST(&map->cl_header)) && (entry->start < end)) {
		vm_map_clip_end(map, entry, end);

		switch (new_advice) {
		case MADV_NORMAL:
		case MADV_RANDOM:
		case MADV_SEQUENTIAL:
			/* nothing special here */
			break;

		default:
			vm_map_unlock(map);
			return EINVAL;
		}
		entry->advice = new_advice;
		entry = CIRCLEQ_NEXT(entry, cl_entry);
	}

	vm_map_unlock(map);
	return (KERN_SUCCESS);
}

/*
 * vm_map_willneed: apply MADV_WILLNEED
 */
int
vm_map_willneed(map, start, end)
	register vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	vm_map_entry_t	entry;

	vm_map_lock_read(map);
	VM_MAP_RANGE_CHECK(map, start, end);
	if (!vm_map_lookup_entry(map, start, &entry)) {
		entry = CIRCLEQ_NEXT(entry, cl_entry);
	}
	while(entry->start < end) {
		vm_object_t obj = entry->object.vm_object;

		/*
		 * For now, we handle only the easy but commonly-requested case.
		 * ie. start prefetching of backing obj pages.
		 */

		if(entry->is_a_map && obj != NULL) {
			vm_offset_t offset;
			vm_offset_t size;

			offset = entry->offset;
			if (start < entry->start) {
				offset += entry->start - start;
			}
			size = entry->offset + (entry->end - entry->start);
			if (entry->end < end) {
				size -= end - entry->end;
			}
		}
		entry = CIRCLEQ_NEXT(entry, cl_entry);
	}

	vm_map_unlock_read(map);
	return (KERN_SUCCESS);
}

/*
 *	vm_map_pageable:
 *
 *	Sets the pageability of the specified address
 *	range in the target map.  Regions specified
 *	as not pageable require locked-down physical
 *	memory and physical page maps.
 *
 *	The map must not be locked, but a reference
 *	must remain to the map throughout the call.
 */
int
vm_map_pageable(map, start, end, new_pageable)
	register vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register bool_t			new_pageable;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t			start_entry;
	register vm_offset_t	failed;
	int						rv;

	vm_map_lock(map);

	VM_MAP_RANGE_CHECK(map, start, end);

	/*
	 *	Only one pageability change may take place at one
	 *	time, since vm_fault assumes it will be called
	 *	only once for each wiring/unwiring.  Therefore, we
	 *	have to make sure we're actually changing the pageability
	 *	for the entire region.  We do so before making any changes.
	 */

	if (vm_map_lookup_entry(map, start, &start_entry) == FALSE) {
		vm_map_unlock(map);
		return (KERN_INVALID_ADDRESS);
	}
	entry = start_entry;

	/*
	 *	Actions are rather different for wiring and unwiring,
	 *	so we have two separate cases.
	 */

	if (new_pageable) {

		vm_map_clip_start(map, entry, start);

		/*
		 *	Unwiring.  First ensure that the range to be
		 *	unwired is really wired down and that there
		 *	are no holes.
		 */
		while ((entry != CIRCLEQ_FIRST(&map->cl_header)) && (entry->start < end)) {

		    if (entry->wired_count == 0 ||
			(entry->end < end &&
			 (CIRCLEQ_NEXT(entry, cl_entry) == CIRCLEQ_FIRST(&map->cl_header) ||
					 CIRCLEQ_NEXT(entry, cl_entry)->start > entry->end))) {
		    	vm_map_unlock(map);
		    	return (KERN_INVALID_ARGUMENT);
		    }
		    entry = CIRCLEQ_NEXT(entry, cl_entry);
		}

		/*
		 *	Now decrement the wiring count for each region.
		 *	If a region becomes completely unwired,
		 *	unwire its physical pages and mappings.
		 */
		vm_map_set_recursive(&map->lock);

		entry = start_entry;
		while ((entry != CIRCLEQ_FIRST(&map->cl_header)) && (entry->start < end)) {
		    vm_map_clip_end(map, entry, end);

		    entry->wired_count--;
		    if (entry->wired_count == 0) {
		    	vm_fault_unwire(map, entry->start, entry->end);
		    }

		    entry = CIRCLEQ_NEXT(entry, cl_entry);
		}
		vm_map_clear_recursive(&map->lock);
	} else {
		/*
		 *	Wiring.  We must do this in two passes:
		 *
		 *	1.  Holding the write lock, we create any shadow
		 *	    or zero-fill objects that need to be created.
		 *	    Then we clip each map entry to the region to be
		 *	    wired and increment its wiring count.  We
		 *	    create objects before clipping the map entries
		 *	    to avoid object proliferation.
		 *
		 *	2.  We downgrade to a read lock, and call
		 *	    vm_fault_wire to fault in the pages for any
		 *	    newly wired area (wired_count is 1).
		 *
		 *	Downgrading to a read lock for vm_fault_wire avoids
		 *	a possible deadlock with another thread that may have
		 *	faulted on one of the pages to be wired (it would mark
		 *	the page busy, blocking us, then in turn block on the
		 *	map lock that we hold).  Because of problems in the
		 *	recursive lock package, we cannot upgrade to a write
		 *	lock in vm_map_lookup.  Thus, any actions that require
		 *	the write lock must be done beforehand.  Because we
		 *	keep the read lock on the map, the copy-on-write status
		 *	of the entries we modify here cannot change.
		 */

		/*
		 *	Pass 1.
		 */
		while ((entry != CIRCLEQ_FIRST(&map->cl_header)) && (entry->start < end)) {
		    if (entry->wired_count == 0) {

				/*
				 *	Perform actions of vm_map_lookup that need
				 *	the write lock on the map: create a shadow
				 *	object for a copy-on-write region, or an
				 *	object for a zero-fill region.
				 *
				 *	We don't have to do this for entries that
				 *	point to sharing maps, because we won't hold
				 *	the lock on the sharing map.
				 */
		    	if (!entry->is_a_map) {
		    		if (entry->needs_copy && ((entry->protection & VM_PROT_WRITE) != 0)) {
		    			vm_object_shadow(&entry->object.vm_object, &entry->offset, (vm_size_t)(entry->end - entry->start));
		    			entry->needs_copy = FALSE;
		    		} else if (entry->object.vm_object == NULL) {
		    			vm_amap_copy(map, entry, M_WAITOK, TRUE, start, end);
		    			entry->object.vm_object =  vm_object_allocate((vm_size_t)(entry->end - entry->start));
		    			entry->offset = (vm_offset_t)0;
		    		}
		    	}
		    }

		    vm_map_clip_start(map, entry, start);
		    vm_map_clip_end(map, entry, end);
		    entry->wired_count++;

		    /*
		     * Check for holes
		     */
		    if (entry->end < end && (CIRCLEQ_NEXT(entry, cl_entry) == CIRCLEQ_FIRST(&map->cl_header) || CIRCLEQ_NEXT(entry, cl_entry)->start > entry->end)) {
		    	while (entry != CIRCLEQ_FIRST(&map->cl_header) && entry->end > start) {
		    		entry->wired_count--;
		    		entry = CIRCLEQ_PREV(entry, cl_entry);
		    	}
		    	vm_map_unlock(map);
		    	return (KERN_INVALID_ARGUMENT);
		    }
		    entry = CIRCLEQ_NEXT(entry, cl_entry);
		}

		/*
		 *	Pass 2.
		 */

		/*
		 * HACK HACK HACK HACK
		 *
		 * If we are wiring in the kernel map or a submap of it,
		 * unlock the map to avoid deadlocks.  We trust that the
		 * kernel threads are well-behaved, and therefore will
		 * not do anything destructive to this region of the map
		 * while we have it unlocked.  We cannot trust user threads
		 * to do the same.
		 *
		 * HACK HACK HACK HACK
		 */
		if (vm_map_pmap(map) == kernel_pmap) {
		    vm_map_unlock(map);		/* trust me ... */
		} else {
		    vm_map_set_recursive(&map->lock);
		    lockmgr(&map->lock, LK_DOWNGRADE, (void *)0, curproc->p_pid);
		}

		rv = 0;
		entry = start_entry;
		while (entry != CIRCLEQ_FIRST(&map->cl_header) && entry->start < end) {
		    /*
		     * If vm_fault_wire fails for any page we need to
		     * undo what has been done.  We decrement the wiring
		     * count for those pages which have not yet been
		     * wired (now) and unwire those that have (later).
		     *
		     * XXX this violates the locking protocol on the map,
		     * needs to be fixed.
		     */
		    if (rv){
			entry->wired_count--;
		    } else if (entry->wired_count == 1) {
		    	rv = vm_fault_wire(map, entry->start, entry->end);
		    	if (rv) {
		    		failed = entry->start;
		    		entry->wired_count--;
		    	}
		    }
		    entry = CIRCLEQ_NEXT(entry, cl_entry);
		}

		if (vm_map_pmap(map) == kernel_pmap) {
		    vm_map_lock(map);
		} else {
		    vm_map_clear_recursive(&map->lock);
		}
		if (rv) {
		    vm_map_unlock(map);
		    (void) vm_map_pageable(map, start, failed, TRUE);
		    return (rv);
		}
	}

	vm_map_unlock(map);

	return (KERN_SUCCESS);
}

/*
 * vm_map_clean
 *
 * Push any dirty cached pages in the address range to their pager.
 * If syncio is TRUE, dirty pages are written synchronously.
 * If invalidate is TRUE, any cached pages are freed as well.
 *
 * Returns an error if any part of the specified range is not mapped.
 */
int
vm_map_clean(map, start, end, syncio, invalidate)
	vm_map_t 		map;
	vm_offset_t		start;
	vm_offset_t		end;
	bool_t			syncio;
	bool_t			invalidate;
{
	register vm_map_entry_t current;
	vm_map_entry_t 			entry;
	vm_size_t 				size;
	vm_offset_t 			offset;
	vm_object_t 			object;
	vm_amap_t 	 			amap;
	bool_t 					lookup, clean;

	vm_map_lock_read(map);
	VM_MAP_RANGE_CHECK(map, start, end);
	lookup = vm_map_lookup_entry(map, start, &entry);
	if (!lookup) {
		vm_map_unlock_read(map);
		return (KERN_INVALID_ADDRESS);
	}

	/*
	 * Make a first pass to check for holes.
	 */
	CIRCLEQ_FOREACH(current, &map->cl_header, cl_entry) {
		if (current == entry) {
			if (current->start < end || end > current->end) {
				if (current->is_sub_map) {
					vm_map_unlock_read(map);
					return (KERN_INVALID_ARGUMENT);
				}
				vm_map_entry_t 	first, next;

				first = CIRCLEQ_FIRST(&map->cl_header);
				next = CIRCLEQ_NEXT(current, cl_entry);
				if (next == first || current->end != next->start) {
					vm_map_unlock_read(map);
					return (KERN_INVALID_ARGUMENT);
				}
			}
		}
	}

	/*
	 * Make a second pass, cleaning/uncaching pages from the indicated
	 * objects as we go.
	 */
	CIRCLEQ_FOREACH(current, &map->cl_header, cl_entry) {
		if (current == entry) {
			if (current->start < end || end > current->end) {
				amap = current->aref.ar_amap;
				KASSERT(start >= current->start);

				offset = current->offset + (start - current->start);
				size = (end <= current->end ? end : current->end) - start;

				if (amap != NULL) {
					vm_amap_clean(current, size, offset, amap);
				}

				if (current->is_a_map) {
					register vm_map_t 	smap;
					vm_map_entry_t 		tentry;
					vm_size_t 			tsize;

					smap = current->object.share_map;
					vm_map_lock_read(smap);
					(void)vm_map_lookup_entry(smap, offset, &tentry);
					tsize = tentry->end - offset;
					if (tsize < size) {
						size = tsize;
					}
					object = tentry->object.vm_object;
					offset = tentry->offset + (offset - tentry->start);
					vm_object_lock(object);
					vm_map_unlock_read(smap);
				} else {
					object = current->object.vm_object;
					vm_object_lock(object);
				}

				/*
				 * Flush pages if writing is allowed.
				 * XXX should we continue on an error?
				 */
				clean = vm_object_segment_page_clean(object, offset, offset+size, syncio, FALSE);
				if ((current->protection & VM_PROT_WRITE) && !clean) {
					vm_object_unlock(object);
					vm_map_unlock_read(map);
					return (KERN_FAILURE);
				}
				if (invalidate) {
					vm_object_segment_page_remove(object, offset, offset+size);
				}
				vm_object_unlock(object);
				start += size;
			}
		}
	}

	vm_map_unlock_read(map);
	return (KERN_SUCCESS);
}

/*
 *	vm_map_entry_unwire:	[ internal use only ]
 *
 *	Make the region specified by this entry pageable.
 *
 *	The map in question should be locked.
 *	[This is the reason for this routine's existence.]
 */
void
vm_map_entry_unwire(map, entry)
	vm_map_t		map;
	register vm_map_entry_t	entry;
{
	vm_fault_unwire(map, entry->start, entry->end);
	entry->wired_count = 0;
}

/*
 *	vm_map_entry_delete:	[ internal use only ]
 *
 *	Deallocate the given entry from the target map.
 */
void
vm_map_entry_delete(map, entry)
	register vm_map_t	map;
	register vm_map_entry_t	entry;
{
	if (entry->wired_count != 0) {
		vm_map_entry_unwire(map, entry);
	}

	vm_map_entry_unlink(map, entry);
	map->size -= entry->end - entry->start;

	if (entry->is_a_map || entry->is_sub_map) {
		vm_map_deallocate(entry->object.share_map);
	} else {
	 	vm_object_deallocate(entry->object.vm_object);
	}

	vm_map_entry_dispose(map, entry);
}

/*
 *	vm_map_delete:	[ internal use only ]
 *
 *	Deallocates the given address range from the target
 *	map.
 *
 *	When called with a sharing map, removes pages from
 *	that region from all physical maps.
 */
int
vm_map_delete(map, start, end)
	register vm_map_t	map;
	vm_offset_t		start;
	register vm_offset_t	end;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		first_entry;

	/*
	 *	Find the start of the region, and clip it
	 */

	if (!vm_map_lookup_entry(map, start, &first_entry)) {
		entry = CIRCLEQ_NEXT(first_entry, cl_entry);
	} else {
		entry = first_entry;
		vm_map_clip_start(map, entry, start);

		/*
		 *	Fix the lookup hint now, rather than each
		 *	time though the loop.
		 */

		SAVE_HINT(map, CIRCLEQ_PREV(entry, cl_entry));
	}

	/*
	 *	Save the free space hint
	 */

	if (map->first_free->start >= start) {
		map->first_free = CIRCLEQ_PREV(entry, cl_entry);
	}

	/*
	 *	Step through all entries in this region
	 */

	while ((entry != CIRCLEQ_FIRST(&map->cl_header)) && (entry->start < end)) {
		vm_map_entry_t		next;
		register vm_offset_t	s, e;
		register vm_object_t	object;

		vm_map_clip_end(map, entry, end);

		next = CIRCLEQ_NEXT(entry, cl_entry);
		s = entry->start;
		e = entry->end;

		/*
		 *	Unwire before removing addresses from the pmap;
		 *	otherwise, unwiring will put the entries back in
		 *	the pmap.
		 */

		object = entry->object.vm_object;
		if (entry->wired_count != 0) {
			vm_map_entry_unwire(map, entry);
		}

		/*
		 *	If this is a sharing map, we must remove
		 *	*all* references to this data, since we can't
		 *	find all of the physical maps which are sharing
		 *	it.
		 */

		if (object == kernel_object || object == kmem_object) {
			vm_object_segment_page_remove(object, entry->offset,
					entry->offset + (e - s));
		} else if (!map->is_main_map) {
			vm_object_pmap_remove(object,
					 entry->offset,
					 entry->offset + (e - s));
		} else {
			pmap_remove(map->pmap, s, e);
		}

		/*
		 *	Delete the entry (which may delete the object)
		 *	only after removing all pmap entries pointing
		 *	to its pages.  (Otherwise, its page frames may
		 *	be reallocated, and any modify bits will be
		 *	set in the wrong object!)
		 */

		vm_map_entry_delete(map, entry);
		entry = next;
	}
	return (KERN_SUCCESS);
}

/*
 *	vm_map_remove:
 *
 *	Remove the given address range from the target map.
 *	This is the exported form of vm_map_delete.
 */
int
vm_map_remove(map, start, end)
	register vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register int		result;

	vm_map_lock(map);
	VM_MAP_RANGE_CHECK(map, start, end);
	result = vm_map_delete(map, start, end);
	vm_map_unlock(map);

	return (result);
}

/*
 *	vm_map_check_protection:
 *
 *	Assert that the target map allows the specified
 *	privilege on the entire address region given.
 *	The entire region must be allocated.
 */
bool_t
vm_map_check_protection(map, start, end, protection)
	register vm_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	register vm_prot_t		protection;
{
	register vm_map_entry_t	entry;
	vm_map_entry_t		tmp_entry;

	if (!vm_map_lookup_entry(map, start, &tmp_entry)) {
		return (FALSE);
	}

	entry = tmp_entry;

	while (start < end) {
		if (entry == CIRCLEQ_FIRST(&map->cl_header)) {
			return (FALSE);
		}

		/*
		 *	No holes allowed!
		 */

		if (start < entry->start) {
			return (FALSE);
		}

		/*
		 * Check protection associated with entry.
		 */

		if ((entry->protection & protection) != protection) {
			return (FALSE);
		}

		/* go to next entry */

		start = entry->end;
		entry = CIRCLEQ_NEXT(entry, cl_entry);
	}
	return (TRUE);
}

/*
 *	vm_map_copy_entry:
 *
 *	Copies the contents of the source entry to the destination
 *	entry.  The entries *must* be aligned properly.
 */
void
vm_map_copy_entry(src_map, dst_map, src_entry, dst_entry)
	vm_map_t				src_map, dst_map;
	register vm_map_entry_t	src_entry, dst_entry;
{
	vm_object_t	temp_object;

	if (src_entry->is_sub_map || dst_entry->is_sub_map) {
		return;
	}

	if (dst_entry->object.vm_object != NULL &&
	    (dst_entry->object.vm_object->flags & OBJ_INTERNAL) == 0) {
		printf("vm_map_copy_entry: copying over permanent data!\n");
	}

	/*
	 *	If our destination map was wired down,
	 *	unwire it now.
	 */

	if (dst_entry->wired_count != 0) {
		vm_map_entry_unwire(dst_map, dst_entry);
	}

	/*
	 *	If we're dealing with a sharing map, we
	 *	must remove the destination pages from
	 *	all maps (since we cannot know which maps
	 *	this sharing map belongs in).
	 */

	if (dst_map->is_main_map) {
		pmap_remove(dst_map->pmap, dst_entry->start, dst_entry->end);
	} else {
		vm_object_pmap_remove(dst_entry->object.vm_object,
			dst_entry->offset,
			dst_entry->offset +
				(dst_entry->end - dst_entry->start));
	}

	if (src_entry->wired_count == 0) {

		bool_t	src_needs_copy;

		/*
		 *	If the source entry is marked needs_copy,
		 *	it is already write-protected.
		 */
		if (!src_entry->needs_copy) {

			bool_t	su;

			/*
			 *	If the source entry has only one mapping,
			 *	we can just protect the virtual address
			 *	range.
			 */
			if (!(su = src_map->is_main_map)) {
				simple_lock(&src_map->ref_lock);
				su = (src_map->ref_count == 1);
				simple_unlock(&src_map->ref_lock);
			}

			if (su) {
				pmap_protect(src_map->pmap,
					src_entry->start,
					src_entry->end,
					src_entry->protection & ~VM_PROT_WRITE);
			} else {
				vm_object_pmap_copy(src_entry->object.vm_object,
					src_entry->offset,
					src_entry->offset + (src_entry->end
							    -src_entry->start));
			}
		}

		/*
		 *	Make a copy of the object.
		 */
		temp_object = dst_entry->object.vm_object;
		vm_object_copy(src_entry->object.vm_object,
				src_entry->offset,
				(vm_size_t)(src_entry->end -
					    src_entry->start),
				&dst_entry->object.vm_object,
				&dst_entry->offset,
				&src_needs_copy);
		/*
		 *	If we didn't get a copy-object now, mark the
		 *	source map entry so that a shadow will be created
		 *	to hold its changed pages.
		 */
		if (src_needs_copy) {
			src_entry->needs_copy = TRUE;
		}

		/*
		 *	The destination always needs to have a shadow
		 *	created.
		 */
		dst_entry->needs_copy = TRUE;

		/*
		 *	Mark the entries copy-on-write, so that write-enabling
		 *	the entry won't make copy-on-write pages writable.
		 */
		src_entry->copy_on_write = TRUE;
		dst_entry->copy_on_write = TRUE;
		/*
		 *	Get rid of the old object.
		 */
		vm_object_deallocate(temp_object);

		pmap_copy(dst_map->pmap, src_map->pmap, dst_entry->start,
			dst_entry->end - dst_entry->start, src_entry->start);
	} else {
		/*
		 *	Of course, wired down pages can't be set copy-on-write.
		 *	Cause wired pages to be copied into the new
		 *	map by simulating faults (the new pages are
		 *	pageable)
		 */
		vm_fault_copy_entry(dst_map, src_map, dst_entry, src_entry);
	}
}

/*
 *	vm_map_copy:
 *
 *	Perform a virtual memory copy from the source
 *	address map/range to the destination map/range.
 *
 *	If src_destroy or dst_alloc is requested,
 *	the source and destination regions should be
 *	disjoint, not only in the top-level map, but
 *	in the sharing maps as well.  [The best way
 *	to guarantee this is to use a new intermediate
 *	map to make copies.  This also reduces map
 *	fragmentation.]
 */
int
vm_map_copy(dst_map, src_map,
			  dst_addr, len, src_addr,
			  dst_alloc, src_destroy)
	vm_map_t	dst_map;
	vm_map_t	src_map;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
	bool_t	dst_alloc;
	bool_t	src_destroy;
{
	register
	vm_map_entry_t	src_entry;
	register
	vm_map_entry_t	dst_entry;
	vm_map_entry_t	tmp_entry;
	vm_offset_t	src_start;
	vm_offset_t	src_end;
	vm_offset_t	dst_start;
	vm_offset_t	dst_end;
	vm_offset_t	src_clip;
	vm_offset_t	dst_clip;
	int		result;
	bool_t	old_src_destroy;

	/*
	 *	XXX While we figure out why src_destroy screws up,
	 *	we'll do it by explicitly vm_map_delete'ing at the end.
	 */

	old_src_destroy = src_destroy;
	src_destroy = FALSE;

	/*
	 *	Compute start and end of region in both maps
	 */

	src_start = src_addr;
	src_end = src_start + len;
	dst_start = dst_addr;
	dst_end = dst_start + len;

	/*
	 *	Check that the region can exist in both source
	 *	and destination.
	 */

	if ((dst_end < dst_start) || (src_end < src_start)) {
		return (KERN_NO_SPACE);
	}

	/*
	 *	Lock the maps in question -- we avoid deadlock
	 *	by ordering lock acquisition by map value
	 */

	if (src_map == dst_map) {
		vm_map_lock(src_map);
	}
	else if ((long) src_map < (long) dst_map) {
	 	vm_map_lock(src_map);
		vm_map_lock(dst_map);
	} else {
		vm_map_lock(dst_map);
	 	vm_map_lock(src_map);
	}

	result = KERN_SUCCESS;

	/*
	 *	Check protections... source must be completely readable and
	 *	destination must be completely writable.  [Note that if we're
	 *	allocating the destination region, we don't have to worry
	 *	about protection, but instead about whether the region
	 *	exists.]
	 */

	if (src_map->is_main_map && dst_map->is_main_map) {
		if (!vm_map_check_protection(src_map, src_start, src_end,
					VM_PROT_READ)) {
			result = KERN_PROTECTION_FAILURE;
			goto Return;
		}

		if (dst_alloc) {
			/* XXX Consider making this a vm_map_find instead */
			if ((result = vm_map_insert(dst_map, NULL,
					(vm_offset_t) 0, dst_start, dst_end)) != KERN_SUCCESS) {
				goto Return;
			}
		} else if (!vm_map_check_protection(dst_map, dst_start, dst_end,
					VM_PROT_WRITE)) {
			result = KERN_PROTECTION_FAILURE;
			goto Return;
		}
	}

	/*
	 *	Find the start entries and clip.
	 *
	 *	Note that checking protection asserts that the
	 *	lookup cannot fail.
	 *
	 *	Also note that we wait to do the second lookup
	 *	until we have done the first clip, as the clip
	 *	may affect which entry we get!
	 */

	(void) vm_map_lookup_entry(src_map, src_addr, &tmp_entry);
	src_entry = tmp_entry;
	vm_map_clip_start(src_map, src_entry, src_start);

	(void) vm_map_lookup_entry(dst_map, dst_addr, &tmp_entry);
	dst_entry = tmp_entry;
	vm_map_clip_start(dst_map, dst_entry, dst_start);

	/*
	 *	If both source and destination entries are the same,
	 *	retry the first lookup, as it may have changed.
	 */

	if (src_entry == dst_entry) {
		(void) vm_map_lookup_entry(src_map, src_addr, &tmp_entry);
		src_entry = tmp_entry;
	}

	/*
	 *	If source and destination entries are still the same,
	 *	a null copy is being performed.
	 */

	if (src_entry == dst_entry) {
		goto Return;
	}

	/*
	 *	Go through entries until we get to the end of the
	 *	region.
	 */

	while (src_start < src_end) {
		/*
		 *	Clip the entries to the endpoint of the entire region.
		 */

		vm_map_clip_end(src_map, src_entry, src_end);
		vm_map_clip_end(dst_map, dst_entry, dst_end);

		/*
		 *	Clip each entry to the endpoint of the other entry.
		 */

		src_clip = src_entry->start + (dst_entry->end - dst_entry->start);
		vm_map_clip_end(src_map, src_entry, src_clip);

		dst_clip = dst_entry->start + (src_entry->end - src_entry->start);
		vm_map_clip_end(dst_map, dst_entry, dst_clip);

		/*
		 *	Both entries now match in size and relative endpoints.
		 *
		 *	If both entries refer to a VM object, we can
		 *	deal with them now.
		 */

		if (!src_entry->is_a_map && !dst_entry->is_a_map) {
			vm_map_copy_entry(src_map, dst_map, src_entry,
						dst_entry);
		} else {
			register vm_map_t	new_dst_map;
			vm_offset_t		new_dst_start;
			vm_size_t		new_size;
			vm_map_t		new_src_map;
			vm_offset_t		new_src_start;

			/*
			 *	We have to follow at least one sharing map.
			 */

			new_size = (dst_entry->end - dst_entry->start);

			if (src_entry->is_a_map) {
				new_src_map = src_entry->object.share_map;
				new_src_start = src_entry->offset;
			} else {
			 	new_src_map = src_map;
				new_src_start = src_entry->start;
				vm_map_set_recursive(&src_map->lock);
			}

			if (dst_entry->is_a_map) {
			    	vm_offset_t	new_dst_end;

				new_dst_map = dst_entry->object.share_map;
				new_dst_start = dst_entry->offset;

				/*
				 *	Since the destination sharing entries
				 *	will be merely deallocated, we can
				 *	do that now, and replace the region
				 *	with a null object.  [This prevents
				 *	splitting the source map to match
				 *	the form of the destination map.]
				 *	Note that we can only do so if the
				 *	source and destination do not overlap.
				 */

				new_dst_end = new_dst_start + new_size;

				if (new_dst_map != new_src_map) {
					vm_map_lock(new_dst_map);
					(void) vm_map_delete(new_dst_map,
							new_dst_start,
							new_dst_end);
					(void) vm_map_insert(new_dst_map,
							NULL,
							(vm_offset_t) 0,
							new_dst_start,
							new_dst_end);
					vm_map_unlock(new_dst_map);
				}
			} else {
			 	new_dst_map = dst_map;
				new_dst_start = dst_entry->start;
				vm_map_set_recursive(&dst_map->lock);
			}

			/*
			 *	Recursively copy the sharing map.
			 */

			(void) vm_map_copy(new_dst_map, new_src_map,
				new_dst_start, new_size, new_src_start,
				FALSE, FALSE);

			if (dst_map == new_dst_map) {
				vm_map_clear_recursive(&dst_map->lock);
			}
			if (src_map == new_src_map) {
				vm_map_clear_recursive(&src_map->lock);
			}
		}

		/*
		 *	Update variables for next pass through the loop.
		 */

		src_start = src_entry->end;
		src_entry = CIRCLEQ_NEXT(src_entry, cl_entry);
		dst_start = dst_entry->end;
		dst_entry = CIRCLEQ_NEXT(dst_entry, cl_entry);

		/*
		 *	If the source is to be destroyed, here is the
		 *	place to do it.
		 */

		if (src_destroy && src_map->is_main_map &&
						dst_map->is_main_map) {
			vm_map_entry_delete(src_map, CIRCLEQ_PREV(src_entry, cl_entry));
		}
	}

	/*
	 *	Update the physical maps as appropriate
	 */

	if (src_map->is_main_map && dst_map->is_main_map) {
		if (src_destroy) {
			pmap_remove(src_map->pmap, src_addr, src_addr + len);
		}
	}

	/*
	 *	Unlock the maps
	 */

Return: ;

	if (old_src_destroy) {
		vm_map_delete(src_map, src_addr, src_addr + len);
	}

	vm_map_unlock(src_map);
	if (src_map != dst_map) {
		vm_map_unlock(dst_map);
	}

	return(result);
}

/*
 * vmspace_fork:
 * Create a new process vmspace structure and vm_map
 * based on those of an existing process.  The new map
 * is based on the old map, according to the inheritance
 * values on the regions in that map.
 *
 * The source map must not be locked.
 */
struct vmspace *
vmspace_fork(vm1)
	register struct vmspace *vm1;
{
	register struct vmspace *vm2;
	vm_map_t		old_map;
	vm_map_t		new_map;
	vm_map_entry_t	old_first;
	vm_map_entry_t	old_entry;
	vm_map_entry_t	new_entry;
	pmap_t			new_pmap;

	old_map = &vm1->vm_map;
	vm_map_lock(old_map);

	vm2 = vmspace_alloc(old_map->min_offset, old_map->max_offset, old_map->entries_pageable);
	bcopy(&vm1->vm_startcopy, &vm2->vm_startcopy, (caddr_t) (vm1 + 1) - (caddr_t) &vm1->vm_startcopy);
	new_pmap = &vm2->vm_pmap;		/* XXX */
	new_map = &vm2->vm_map;			/* XXX */

	old_first = CIRCLEQ_FIRST(&old_map->cl_header);
	old_entry = CIRCLEQ_NEXT(old_first, cl_entry);

	while (old_entry != CIRCLEQ_FIRST(&old_map->cl_header)) {
		if (old_entry->is_sub_map) {
			panic("vm_map_fork: encountered a submap");
		}

		switch (old_entry->inheritance) {
		case VM_INHERIT_NONE:
			break;

		case VM_INHERIT_SHARE:
			/*
			 *	If we don't already have a sharing map:
			 */

			if (!old_entry->is_a_map) {
			 	vm_map_t	new_share_map;
				vm_map_entry_t	new_share_entry;

				/*
				 *	Create a new sharing map
				 */

				new_share_map = vm_map_create(NULL,
							old_entry->start,
							old_entry->end,
							TRUE);
				new_share_map->is_main_map = FALSE;

				/*
				 *	Create the only sharing entry from the
				 *	old task map entry.
				 */

				new_share_entry =
					vm_map_entry_create(new_share_map);
				*new_share_entry = *old_entry;
				new_share_entry->wired_count = 0;

				if (new_entry->aref.ar_amap) {
					/* share reference */
					vm_amap_ref(new_entry, AMAP_SHARED);
				}

				/*
				 *	Insert the entry into the new sharing
				 *	map
				 */

				vm_map_entry_link(new_share_map, CIRCLEQ_FIRST(&new_share_map->cl_header)->cl_entry.cqe_prev, new_share_entry);

				/*
				 *	Fix up the task map entry to refer
				 *	to the sharing map now.
				 */

				old_entry->is_a_map = TRUE;
				old_entry->object.share_map = new_share_map;
				old_entry->offset = old_entry->start;
			}

			/*
			 *	Clone the entry, referencing the sharing map.
			 */

			new_entry = vm_map_entry_create(new_map);
			*new_entry = *old_entry;
			new_entry->wired_count = 0;
			vm_map_reference(new_entry->object.share_map);

			/*
			 *	Insert the entry into the new map -- we
			 *	know we're inserting at the end of the new
			 *	map.
			 */

			vm_map_entry_link(new_map, CIRCLEQ_FIRST(&new_map->cl_header)->cl_entry.cqe_prev, new_entry);

			/*
			 *	Update the physical map
			 */

			pmap_copy(new_map->pmap, old_map->pmap,
				new_entry->start,
				(old_entry->end - old_entry->start),
				old_entry->start);
			break;

		case VM_INHERIT_COPY:
			/*
			 *	Clone the entry and link into the map.
			 */

			new_entry = vm_map_entry_create(new_map);
			if (new_entry->aref.ar_amap) {
				vm_amap_ref(new_entry, 0);
			}
			*new_entry = *old_entry;
			new_entry->wired_count = 0;
			new_entry->object.vm_object = NULL;
			new_entry->is_a_map = FALSE;
			vm_map_entry_link(new_map, CIRCLEQ_FIRST(&new_map->cl_header)->cl_entry.cqe_prev, new_entry);
			if (old_entry->aref.ar_amap != NULL) {
				if ((amap_flags(old_entry->aref.ar_amap) & AMAP_SHARED) != 0 || old_entry->wired_count != 0) {
					vm_amap_copy(new_map, new_entry, M_WAITOK, FALSE, 0, 0);
				}
			}
			if (old_entry->wired_count != 0) {
				vm_amap_cow_now(new_map, new_entry);
			} else {
				if (old_entry->aref.ar_amap) {
					if (old_entry->needs_copy) {
						if (old_entry->max_protection & VM_PROT_WRITE) {
							pmap_protect(old_map->pmap, old_entry->start, old_entry->end, old_entry->protection & ~VM_PROT_WRITE);
						}
						//old_entry->etype |= TRUE;
					}
				} else {
					pmap_copy(new_pmap, old_map->pmap, new_entry->start, (old_entry->end - old_entry->start), old_entry->start);
				}
			}
			if (old_entry->is_a_map) {
				int	check;

				check = vm_map_copy(new_map,
						old_entry->object.share_map,
						new_entry->start,
						(vm_size_t)(new_entry->end -
							new_entry->start),
						old_entry->offset,
						FALSE, FALSE);
				if (check != KERN_SUCCESS) {
					printf("vm_map_fork: copy in share_map region failed\n");
				}
			} else {
				vm_map_copy_entry(old_map, new_map, old_entry, new_entry);
			}
			break;
		}
		old_entry = CIRCLEQ_NEXT(old_entry, cl_entry);
	}

	new_map->size = old_map->size;
	vm_map_unlock(old_map);

	return (vm2);
}

/*
 *	vm_map_lookup:
 *
 *	Finds the VM object, offset, and
 *	protection for a given virtual address in the
 *	specified map, assuming a page fault of the
 *	type specified.
 *
 *	Leaves the map in question locked for read; return
 *	values are guaranteed until a vm_map_lookup_done
 *	call is performed.  Note that the map argument
 *	is in/out; the returned map must be used in
 *	the call to vm_map_lookup_done.
 *
 *	A handle (out_entry) is returned for use in
 *	vm_map_lookup_done, to make that fast.
 *
 *	If a lookup is requested with "write protection"
 *	specified, the map may be changed to perform virtual
 *	copying operations, although the data referenced will
 *	remain the same.
 */
int
vm_map_lookup(var_map, vaddr, fault_type, out_entry, object, offset, out_prot, wired, single_use)
	vm_map_t				*var_map;		/* IN/OUT */
	register vm_offset_t	vaddr;
	register vm_prot_t		fault_type;
	vm_map_entry_t			*out_entry;		/* OUT */
	vm_object_t				*object;		/* OUT */
	vm_offset_t				*offset;		/* OUT */
	vm_prot_t				*out_prot;		/* OUT */
	bool_t					*wired;			/* OUT */
	bool_t					*single_use;	/* OUT */
{
	vm_map_t				share_map;
	vm_offset_t				share_offset;
	register vm_map_entry_t	entry;
	register vm_map_t		map;
	register vm_prot_t		prot;
	register bool_t			su;

	map = *var_map;

RetryLookup: ;

	/*
	 *	Lookup the faulting address.
	 */

	vm_map_lock_read(map);

	/*
	 *	If the map has an interesting hint, try it before calling
	 *	full blown lookup routine.
	 */

	simple_lock(&map->hint_lock);
	entry = map->hint;
	simple_unlock(&map->hint_lock);

	*out_entry = entry;

	if ((entry == CIRCLEQ_FIRST(&map->cl_header)) ||
	    (vaddr < entry->start) || (vaddr >= entry->end)) {
		vm_map_entry_t	tmp_entry;

		/*
		 *	Entry was either not a valid hint, or the vaddr
		 *	was not contained in the entry, so do a full lookup.
		 */
		if (!vm_map_lookup_entry(map, vaddr, &tmp_entry)) {
			vm_map_unlock_read(map);
			return (KERN_INVALID_ADDRESS);
		}

		entry = tmp_entry;
		*out_entry = entry;
	}

	/*
	 *	Handle submaps.
	 */

	if (entry->is_sub_map) {
		vm_map_t	old_map = map;

		*var_map = map = entry->object.sub_map;
		vm_map_unlock_read(old_map);
		goto RetryLookup;
	}

	/*
	 *	Check whether this task is allowed to have
	 *	this page.
	 */

	prot = entry->protection;
	if ((fault_type & (prot)) != fault_type) {
		vm_map_unlock_read(map);
		return (KERN_PROTECTION_FAILURE);
	}

	/*
	 *	If this page is not pageable, we have to get
	 *	it for all possible accesses.
	 */

	if (*wired == (entry->wired_count != 0)) {
		prot = fault_type = entry->protection;
	}

	/*
	 *	If we don't already have a VM object, track
	 *	it down.
	 */

	if (su == !entry->is_a_map) {
	 	share_map = map;
		share_offset = vaddr;
	} else {
		vm_map_entry_t	share_entry;

		/*
		 *	Compute the sharing map, and offset into it.
		 */

		share_map = entry->object.share_map;
		share_offset = (vaddr - entry->start) + entry->offset;

		/*
		 *	Look for the backing store object and offset
		 */

		vm_map_lock_read(share_map);

		if (!vm_map_lookup_entry(share_map, share_offset,
					&share_entry)) {
			vm_map_unlock_read(share_map);
			vm_map_unlock_read(map);
			return (KERN_INVALID_ADDRESS);
		}
		entry = share_entry;
	}

	/*
	 *	If the entry was copy-on-write, we either ...
	 */

	if (entry->needs_copy) {
	    	/*
		 *	If we want to write the page, we may as well
		 *	handle that now since we've got the sharing
		 *	map locked.
		 *
		 *	If we don't need to write the page, we just
		 *	demote the permissions allowed.
		 */

		if (fault_type & VM_PROT_WRITE) {
			/*
			 *	Make a new object, and place it in the
			 *	object chain.  Note that no new references
			 *	have appeared -- one just moved from the
			 *	share map to the new object.
			 */

			if (lockmgr(&share_map->lock, LK_EXCLUPGRADE, (void *)0, curproc->p_pid)) {
				if (share_map != map) {
					vm_map_unlock_read(map);
				}
				goto RetryLookup;
			}

			vm_object_shadow(
				&entry->object.vm_object,
				&entry->offset,
				(vm_size_t) (entry->end - entry->start));

			entry->needs_copy = FALSE;

			lockmgr(&share_map->lock, LK_DOWNGRADE, (void *)0, curproc->p_pid);
		} else {
			/*
			 *	We're attempting to read a copy-on-write
			 *	page -- don't allow writes.
			 */

			prot &= (~VM_PROT_WRITE);
		}
	}

	/*
	 *	Create an object if necessary.
	 */
	if (entry->object.vm_object == NULL) {

		if (lockmgr(&share_map->lock, LK_EXCLUPGRADE, (void *)0, curproc->p_pid)) {
			if (share_map != map) {
				vm_map_unlock_read(map);
			}
			goto RetryLookup;
		}

		entry->object.vm_object = vm_object_allocate(
					(vm_size_t)(entry->end - entry->start));
		entry->offset = 0;
		lockmgr(&share_map->lock, LK_DOWNGRADE, (void *)0, curproc->p_pid);
	}

	/*
	 *	Return the object/offset from this entry.  If the entry
	 *	was copy-on-write or empty, it has been fixed up.
	 */

	*offset = (share_offset - entry->start) + entry->offset;
	*object = entry->object.vm_object;

	/*
	 *	Return whether this is the only map sharing this data.
	 */

	if (!su) {
		simple_lock(&share_map->ref_lock);
		su = (share_map->ref_count == 1);
		simple_unlock(&share_map->ref_lock);
	}

	*out_prot = prot;
	*single_use = su;

	return (KERN_SUCCESS);
}

/*
 *	vm_map_lookup_done:
 *
 *	Releases locks acquired by a vm_map_lookup
 *	(according to the handle returned by that lookup).
 */

void
vm_map_lookup_done(map, entry)
	register vm_map_t	map;
	vm_map_entry_t		entry;
{
	/*
	 *	If this entry references a map, unlock it first.
	 */

	if (entry->is_a_map) {
		vm_map_unlock_read(entry->object.share_map);
	}

	/*
	 *	Unlock the main-level map
	 */

	vm_map_unlock_read(map);
}

/*
 *	Routine:	vm_map_simplify
 *	Purpose:
 *		Attempt to simplify the map representation in
 *		the vicinity of the given starting address.
 *	Note:
 *		This routine is intended primarily to keep the
 *		kernel maps more compact -- they generally don't
 *		benefit from the "expand a map entry" technology
 *		at allocation time because the adjacent entry
 *		is often wired down.
 */
void
vm_map_simplify(map, start)
	vm_map_t	map;
	vm_offset_t	start;
{
	vm_map_entry_t	this_entry;
	vm_map_entry_t	prev_entry;

	vm_map_lock(map);
	if (
		(vm_map_lookup_entry(map, start, &this_entry)) &&
		((prev_entry = CIRCLEQ_PREV(this_entry, cl_entry)) != CIRCLEQ_FIRST(&map->cl_header)) &&

		(prev_entry->end == start) &&
		(map->is_main_map) &&

		(prev_entry->is_a_map == FALSE) &&
		(prev_entry->is_sub_map == FALSE) &&

		(this_entry->is_a_map == FALSE) &&
		(this_entry->is_sub_map == FALSE) &&

		(prev_entry->inheritance == this_entry->inheritance) &&
		(prev_entry->protection == this_entry->protection) &&
		(prev_entry->max_protection == this_entry->max_protection) &&
		(prev_entry->wired_count == this_entry->wired_count) &&

		(prev_entry->copy_on_write == this_entry->copy_on_write) &&
		(prev_entry->needs_copy == this_entry->needs_copy) &&

		(prev_entry->object.vm_object == this_entry->object.vm_object) &&
		((prev_entry->offset + (prev_entry->end - prev_entry->start))
		     == this_entry->offset)
	) {
		if (map->first_free == this_entry) {
			map->first_free = prev_entry;
		}

		SAVE_HINT(map, prev_entry);
		vm_map_entry_unlink(map, this_entry);
		prev_entry->end = this_entry->end;
	 	vm_object_deallocate(this_entry->object.vm_object);
		vm_map_entry_dispose(map, this_entry);
	}
	vm_map_unlock(map);
}

/*
 *	vm_map_print:	[ debug ]
 */
void
vm_map_print(map, full)
	register vm_map_t	map;
	bool_t				full;
{
   _vm_map_print(map, full, printf);
}

void
_vm_map_print(map, full, pr)
	register vm_map_t	map;
	bool_t				full;
	void				(*pr)(const char *, ...);
{
	register vm_map_entry_t	entry;
	extern int indent;

	iprintf("%s map 0x%x: pmap=0x%x,ref=%d,nentries=%d,version=%d\n",
		(map->is_main_map ? "Task" : "Share"),
 		(int) map, (int) (map->pmap), map->ref_count, map->nentries,
		map->timestamp);

	if (!full && indent) {
		return;
	}

	indent += 2;
	CIRCLEQ_FOREACH(entry, &map->cl_header, cl_entry) {
		iprintf("map entry 0x%x: start=0x%x, end=0x%x, ",
			(int) entry, (int) entry->start, (int) entry->end);
		if (map->is_main_map) {
		     	static char *inheritance_name[4] =
				{ "share", "copy", "none", "donate_copy"};
		     	(*pr)("prot=%x/%x/%s, ",
				entry->protection,
				entry->max_protection,
				inheritance_name[entry->inheritance]);
			if (entry->wired_count != 0) {
				(*pr)("wired, ");
			}
		}

		if (entry->is_a_map || entry->is_sub_map) {
			(*pr)("share=0x%x, offset=0x%x\n",
				(int) entry->object.share_map,
				(int) entry->offset);
			if ((CIRCLEQ_PREV(entry, cl_entry) == CIRCLEQ_FIRST(&map->cl_header)) ||
			    (!CIRCLEQ_PREV(entry, cl_entry)->is_a_map) ||
			    (CIRCLEQ_PREV(entry, cl_entry)->object.share_map !=
			     entry->object.share_map)) {
				indent += 2;
				vm_map_print(entry->object.share_map, full);
				indent -= 2;
			}

		} else {
			(*pr)("object=0x%x, offset=0x%x",
				(int) entry->object.vm_object,
				(int) entry->offset);
			if (entry->copy_on_write) {
				(*pr)(", copy (%s)",
				       entry->needs_copy ? "needed" : "done");
			}
			(*pr)("\n");

			if ((CIRCLEQ_PREV(entry, cl_entry) == CIRCLEQ_FIRST(&map->cl_header)) ||
			    (CIRCLEQ_PREV(entry, cl_entry)->is_a_map) ||
			    (CIRCLEQ_PREV(entry, cl_entry)->object.vm_object !=
			     entry->object.vm_object)) {
				indent += 2;
				vm_object_print(entry->object.vm_object, full);
				indent -= 2;
			}
		}
	}
	indent -= 2;
}
