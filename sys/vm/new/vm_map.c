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
#include <vm/include/vm_amap.h>

#include <vm/include/vm_pager.h>

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

static int vm_rb_compare(vm_map_entry_t, vm_map_entry_t);
static vm_size_t vm_rb_space(const vm_map_t, const vm_map_entry_t);
static vm_size_t vm_rb_subtree_space(vm_map_entry_t);
static void vm_rb_fixup(vm_map_t, vm_map_entry_t);
static void vm_rb_insert(vm_map_t, vm_map_entry_t);
static void vm_rb_remove(vm_map_t, vm_map_entry_t);
static vm_size_t vm_cl_space(const vm_map_t, const vm_map_entry_t);
static void vm_cl_insert(vm_map_t, vm_map_entry_t);
static void vm_cl_remove(vm_map_t, vm_map_entry_t);
static bool_t vm_map_search_next_entry(vm_map_t, vm_offset_t, vm_map_entry_t);
static bool_t vm_map_search_prev_entry(vm_map_t, vm_offset_t, vm_map_entry_t);
static void	_vm_map_clip_end(vm_map_t, vm_map_entry_t, vm_offset_t);
static void	_vm_map_clip_start(vm_map_t, vm_map_entry_t, vm_offset_t);

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
    kmap_free 	= vm_pbootinit(kmap_init, sizeof(struct vm_map), MAX_KMAP);
    kentry_free	= vm_pbootinit(kentry_init, sizeof(struct vm_map_entry), MAX_KMAPENT);
}


/* TODO:
 * - add amap_merge and improve vm_map merge capabilities (NetBSD 2.1)
 */
/*
 * wrapper for calling amap_ref()
 */
static __inline void
vm_map_reference_amap(entry, flags)
	vm_map_entry_t entry;
	int flags;
{
	vm_amap_ref(entry->aref.ar_amap, entry->aref.ar_pageoff, (entry->end - entry->start) >> PAGE_SHIFT, flags);
}

/*
 * wrapper for calling amap_unref()
 */
static __inline void
vm_map_unreference_amap(entry, flags)
	vm_map_entry_t entry;
	int flags;
{
	vm_amap_unref(entry->aref.ar_amap, entry->aref.ar_pageoff, (entry->end - entry->start) >> PAGE_SHIFT, flags);
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
	int 						advice;

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

			if (vm_object_coalesce(prev_entry->object.vm_object,
					NULL,
					prev_entry->offset,
					(vm_offset_t) 0,
					(vm_size_t)(prev_entry->end
						     - prev_entry->start),
					(vm_size_t)(end - prev_entry->end))) {

				if (prev_entry->aref.ar_amap) {
					vm_amap_extend(prev_entry, map->size);
				}

				/*
				 *	Coalesced the two objects - can extend
				 *	the previous map entry to include the
				 *	new range.
				 */
				map->size += (end - prev_entry->end);
				prev_entry->end = end;
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
 *	vm_map_clip_start:	[ internal use only ]
 *
 *	Asserts that the given entry begins at or after
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
#define vm_map_clip_start(map, entry, startaddr) 	\
{ 													\
	if (startaddr > entry->start) 					\
		_vm_map_clip_start(map, entry, startaddr); 	\
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
	caddr_t new_adj;

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

	if (entry->is_a_map || entry->is_sub_map)
	 	vm_map_reference(new_entry->object.share_map);
	else
		vm_object_reference(new_entry->object.vm_object);

	vm_tree_sanity(map, "clip_start leave");
}

/*
 *	vm_map_clip_end:	[ internal use only ]
 *
 *	Asserts that the given entry ends at or before
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */

#define vm_map_clip_end(map, entry, endaddr) 	\
{ 												\
	if (endaddr < entry->end) 					\
		_vm_map_clip_end(map, entry, endaddr); 	\
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
	caddr_t new_adj;

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

	if (entry->is_a_map || entry->is_sub_map)
	 	vm_map_reference(new_entry->object.share_map);
	else
		vm_object_reference(new_entry->object.vm_object);

	vm_tree_sanity(map, "clip_end leave");
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
	CIRCLEQ_FOREACH(current, map->cl_header, cl_entry) {
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
	CIRCLEQ_FOREACH(current, map->cl_header, cl_entry) {
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
