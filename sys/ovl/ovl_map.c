/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mman.h>

#include <ovl/include/ovl.h>
#include <ovl/include/ovl_page.h>
#include <ovl/include/ovl_segment.h>

#undef RB_AUGMENT
static void ovl_rb_augment(ovl_map_entry_t);
#define	RB_AUGMENT(x)	ovl_rb_augment(x)

static int 			ovl_rb_compare(ovl_map_entry_t, ovl_map_entry_t);
static vm_size_t 	ovl_rb_space(const ovl_map_t, const ovl_map_entry_t);
static vm_size_t 	ovl_rb_subtree_space(ovl_map_entry_t);
static void 		ovl_rb_fixup(ovl_map_t, ovl_map_entry_t);
static void 		ovl_rb_insert(ovl_map_t, ovl_map_entry_t);
static void 		ovl_rb_remove(ovl_map_t, ovl_map_entry_t);
static vm_size_t 	ovl_cl_space(const ovl_map_t, const ovl_map_entry_t);
static void 		ovl_cl_insert(ovl_map_t, ovl_map_entry_t);
static void 		ovl_cl_remove(ovl_map_t, ovl_map_entry_t);
static bool_t 		ovl_map_search_next_entry(ovl_map_t, vm_offset_t, ovl_map_entry_t);
static bool_t 		ovl_map_search_prev_entry(ovl_map_t, vm_offset_t, ovl_map_entry_t);
static void		_ovl_map_clip_end(ovl_map_t, ovl_map_entry_t, vm_offset_t);
static void		_ovl_map_clip_start(ovl_map_t, ovl_map_entry_t, vm_offset_t);

RB_PROTOTYPE(ovl_map_rb_tree, ovl_map_entry, rb_entry, ovl_rb_compare);
RB_GENERATE(ovl_map_rb_tree, ovl_map_entry, rb_entry, ovl_rb_compare);

static void	        ovlmap_mapout(ovl_map_t, ovl_map_entry_t);
static void	        ovlmap_mapin(ovl_map_t, ovl_map_entry_t);

ovl_map_t 						omap_free;
ovl_map_entry_t 				oentry_free;
static struct ovl_map			omap_init[MAX_OMAP];
static struct ovl_map_entry		oentry_init[MAX_OMAPENT];

void
ovl_map_startup()
{
	omap_free 	= ovl_pmap_bootinit(omap_init, sizeof(struct ovl_map), MAX_OMAP);
	oentry_free	= ovl_pmap_bootinit(oentry_init, sizeof(struct ovl_map_entry), MAX_OMAPENT);
}

struct ovlspace *
ovlspace_alloc(min, max)
	vm_offset_t min, max;
{
	register struct ovlspace *ovl;

	OVERLAY_MALLOC(ovl, struct ovlspace *, sizeof(struct ovlspace *), M_OVLMAP, M_WAITOK);
	bzero(ovl,(caddr_t)&ovl->ovl_startcopy - (caddr_t)ovl);
	ovl_map_init(&ovl->ovl_map, min, max);
	pmap_pinit(&ovl->ovl_pmap);
	ovl->ovl_map.pmap = &ovl->ovl_pmap;
	ovl->ovl_refcnt = 1;
	return (ovl);
}

/* free ovlspace */
void
ovlspace_free(ovl)
	register struct ovlspace *ovl;
{
	if (--ovl->ovl_refcnt == 0) {
		/*
		 * Lock the map, to wait out all other references to it.
		 * Delete all of the mappings and pages they hold,
		 * then call the pmap module to reclaim anything left.
		 */
		ovl_map_lock(&ovl->ovl_map);
		(void) ovl_map_delete(&ovl->ovl_map, ovl->ovl_map.min_offset, ovl->ovl_map.max_offset);
		pmap_overlay_release(&ovl->ovl_pmap);
		OVERLAY_FREE(ovl, M_OVLMAP);
	}
}

/*
 * Red black tree functions
 *
 * The caller must hold the related map lock.
 */

static int
ovl_rb_compare(a, b)
	ovl_map_entry_t a, b;
{
	if (a->start < b->start)
		return(-1);
	else if (a->start > b->start)
		return (1);
	return(0);
}

static void
ovl_rb_augment(entry)
	struct ovl_map_entry *entry;
{
	entry->space = ovl_rb_subtree_space(entry);
}

static vm_size_t
ovl_rb_space(map, entry)
    const ovl_map_t map;
    const ovl_map_entry_t entry;
{
    KASSERT(CIRCLEQ_NEXT(entry, cl_entry) != NULL);
    return (CIRCLEQ_NEXT(entry, cl_entry)->start - CIRCLEQ_FIRST(&map->cl_header)->end);
}

static vm_size_t
ovl_rb_subtree_space(entry)
	const ovl_map_entry_t entry;
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
ovl_rb_fixup(map, entry)
	ovl_map_t map;
	ovl_map_entry_t entry;
{
	/* We need to traverse to the very top */
	do {
		entry->ownspace = ovl_rb_space(map, entry);
		entry->space = ovl_rb_subtree_space(entry);
	} while ((entry = RB_PARENT(entry, rb_entry)) != NULL);
}

static void
ovl_rb_insert(map, entry)
	ovl_map_t map;
	ovl_map_entry_t entry;
{
    vm_offset_t space;
    ovl_map_entry_t tmp;

    space = ovl_rb_space(map, entry);
    entry->ownspace = entry->space = space;
    tmp = RB_INSERT(ovl_map_rb_tree, &map->rb_root, entry);
#ifdef DIAGNOSTIC
    if (tmp != NULL)
		panic("ovl_rb_insert: duplicate entry?");
#endif
    ovl_rb_fixup(map, entry);
    if (CIRCLEQ_PREV(entry, cl_entry) != RB_ROOT(&map->rb_root)) {
        ovl_rb_fixup(map, CIRCLEQ_PREV(entry, cl_entry));
    }
}

static void
ovl_rb_remove(map, entry)
	ovl_map_t map;
	ovl_map_entry_t entry;
{
    struct ovl_map_entry *parent;

    parent = RB_PARENT(entry, rb_entry);
    RB_REMOVE(ovl_map_rb_tree, &map->rb_root, entry);
    if (CIRCLEQ_PREV(entry, cl_entry) != CIRCLEQ_FIRST(&map->cl_header)) {
        ovl_rb_fixup(map, CIRCLEQ_PREV(entry, cl_entry));
    }
    if (parent) {
        ovl_rb_fixup(map, parent);
    }
}

#ifdef DEBUG
int ovl_debug_check_rbtree = 0;
#define ovl_tree_sanity(x,y)		\
		if (vm_debug_check_rbtree)	\
			_ovl_tree_sanity(x,y)
#else
#define ovl_tree_sanity(x,y)
#endif

int
_ovl_tree_sanity(map, name)
	ovl_map_t map;
	const char *name;
{
	ovl_map_entry_t tmp, trtmp;
	int n = 0, i = 1;

	RB_FOREACH(tmp, ovl_map_rb_tree, &map->rb_root) {
		if (tmp->ownspace != ovl_rb_space(map, tmp)) {
			printf("%s: %d/%d ownspace %lx != %lx %s\n",
			    name, n + 1, map->nentries, (u_long)tmp->ownspace, (u_long)ovl_rb_space(map, tmp),
				CIRCLEQ_NEXT(tmp, cl_entry) == CIRCLEQ_FIRST(&map->cl_header) ? "(last)" : "");
			goto error;
		}
	}
	trtmp = NULL;
	RB_FOREACH(tmp, ovl_map_rb_tree, &map->rb_root) {
		if (tmp->space != ovl_rb_subtree_space(tmp)) {
			printf("%s: space %lx != %lx\n", name, (u_long)tmp->space, (u_long)ovl_rb_subtree_space(tmp));
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
		trtmp = RB_FIND(ovl_map_rb_tree, &map->rb_root, tmp);
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
ovl_cl_space(map, entry)
    const ovl_map_t map;
    const ovl_map_entry_t entry;
{
    vm_offset_t space, tmp;

    space = entry->ownspace;
    if(CIRCLEQ_FIRST(&map->cl_header)) {
        tmp = CIRCLEQ_FIRST(&map->cl_header)->space;
        if (tmp > space) {
            space = tmp;
        }
    }

    if(CIRCLEQ_LAST(&map->cl_header)) {
        tmp = CIRCLEQ_LAST(&map->cl_header)->space;
        if (tmp > space) {
            space = tmp;
        }
    }

    return (space);
}

static void
ovl_cl_insert(map, entry)
    ovl_map_t map;
    ovl_map_entry_t entry;
{
    ovl_map_entry_t head, tail;

    head = CIRCLEQ_FIRST(&map->cl_header);
    tail = CIRCLEQ_LAST(&map->cl_header);

    vm_offset_t space = ovl_rb_space(map, entry);
    entry->ownspace = entry->space = space;

    if(head->space == ovl_cl_space(map, entry)) {
        CIRCLEQ_INSERT_HEAD(&map->cl_header, head, cl_entry);
    }
    if(tail->space == ovl_cl_space(map, entry)) {
        CIRCLEQ_INSERT_TAIL(&map->cl_header, tail, cl_entry);
    }
}

static void
ovl_cl_remove(map, entry)
    ovl_map_t map;
    ovl_map_entry_t entry;
{
    struct ovl_map_entry *head, *tail;
    head = CIRCLEQ_FIRST(&map->cl_header);
    tail = CIRCLEQ_LAST(&map->cl_header);

    if(head && ovl_cl_space(map, entry)) {
        CIRCLEQ_REMOVE(&map->cl_header, head, cl_entry);
    }
    if(tail && ovl_cl_space(map, entry)) {
        CIRCLEQ_REMOVE(&map->cl_header, tail, cl_entry);
    }
}

ovl_map_t
ovl_map_create(pmap, min, max)
	pmap_t		pmap;
	vm_offset_t	min, max;
{
	register ovl_map_t	result;
	ovl_map_entry_t 	oentry;
	extern ovl_map_t	omem_map;

	if (omem_map == NULL) {
		result = omap_free;
		if (result == NULL) {
			panic("ovl_map_create: out of maps");
		}
		oentry = CIRCLEQ_FIRST(&result->cl_header);
		omap_free = (ovl_map_t)CIRCLEQ_NEXT(oentry, cl_entry);
	} else {
		OVERLAY_MALLOC(result, struct ovl_map *, sizeof(struct ovl_map *), M_OVLMAP, M_WAITOK);
	}

	ovl_map_init(result, min, max);
	result->pmap = pmap;
	return (result);
}

void
ovl_map_init(map, min, max)
	ovl_map_t map;
	vm_offset_t	min, max;
{
	CIRCLEQ_INIT(&map->cl_header);
	RB_INIT(&map->rb_root);
	map->nentries = 0;
	map->size = 0;
	map->ref_count = 1;
	map->is_main_map = TRUE;
	map->min_offset = min;
	map->max_offset = max;
	map->hint = CIRCLEQ_FIRST(&map->cl_header);
	map->timestamp = 0;
	lockinit(&map->lock, PVM, "thrd_sleep", 0, 0);
	simple_lock_init(&map->ref_lock, "ovl_map_ref_lock");
	simple_lock_init(&map->hint_lock, "ovl_map_hint_lock");
}

#ifdef DEBUG
void
ovl_map_entry_isspecial(map)
	vm_map_t map;
{
	bool_t isspecial;

	isspecial = (map == overlay_map || map == omem_map);
	if ((isspecial && map->entries_pageable) || (!isspecial && !map->entries_pageable)) {
		panic("ovl_map_entry_create: bogus map");
	}
}
#endif

ovl_map_entry_t
ovl_map_entry_create(map)
	ovl_map_t	map;
{
	ovl_map_entry_t	entry;

#ifdef DEBUG
	ovl_map_entry_isspecial(map);
#endif
	OVERLAY_MALLOC(entry, struct ovl_map_entry *, sizeof(struct ovl_map_entry *), M_OVLMAPENT, M_WAITOK);
	if (entry == oentry_free) {
		oentry_free = CIRCLEQ_NEXT(oentry_free, cl_entry);
	}
	if (entry == NULL) {
		panic("ovl_map_entry_create: out of map entries");
	}

	return(entry);
}

void
ovl_map_entry_dispose(map, entry)
	ovl_map_t		map;
	ovl_map_entry_t	entry;
{
#ifdef DEBUG
	bool_t				isspecial;
	isspecial = (map == overlay_map || map == omem_map);
	if (isspecial || !isspecial) {
		panic("ovl_map_entry_dispose: bogus map");
	}
#endif
	OVERLAY_FREE(entry, M_OVLMAPENT);
	CIRCLEQ_NEXT(entry, cl_entry) = oentry_free;
	oentry_free = entry;
}

/*
 *	ovl_map_entry_{un,}link:
 *
 *	Insert/remove entries from maps.
 */
static void
ovl_map_entry_link(map, after_where, entry)
	ovl_map_t map;
	ovl_map_entry_t after_where, entry;
{
	map->nentries++;
	CIRCLEQ_PREV(entry, cl_entry) = after_where;
	CIRCLEQ_NEXT(entry, cl_entry) = CIRCLEQ_NEXT(after_where, cl_entry);
	CIRCLEQ_PREV(entry, cl_entry)->cl_entry.cqe_next = entry;
	CIRCLEQ_NEXT(entry, cl_entry)->cl_entry.cqe_prev = entry;
	ovl_cl_insert(map, entry);
	ovl_rb_insert(map, entry);
}

static void
ovl_map_entry_unlink(map, entry)
	ovl_map_t map;
	ovl_map_entry_t entry;
{
	map->nentries--;
	ovl_cl_remove(map, entry);
	ovl_rb_remove(map, entry);
}

void
ovl_map_reference(map)
	register ovl_map_t	map;
{
	if (map == NULL)
		return;

	simple_lock(&map->ref_lock);
#ifdef DEBUG
	if (map->ovl_ref_count == 0)
		panic("ovl_map_reference: zero ref_count");
#endif
	map->ref_count++;
	simple_unlock(&map->ref_lock);
}

void
ovl_map_deallocate(map)
	register ovl_map_t	map;
{
	if (map == NULL)
		return;

	simple_lock(&map->ref_lock);
	if (map->ref_count-- > 0) {
		simple_unlock(&map->ref_lock);
		return;
	}

	/*
	 *	Lock the map, to wait out all other references
	 *	to it.
	 */

	ovl_map_lock_drain_interlock(map);

	(void) ovl_map_delete(map, map->min_offset, map->max_offset);

	pmap_overlay_destroy(map->pmap);

	ovl_map_unlock(map);

	OVERLAY_FREE(map, M_OVLMAP);
}

int
ovl_map_insert(map, object, offset, start, end)
	ovl_map_t		map;
	ovl_object_t	object;
	vm_offset_t		offset;
	vm_offset_t		start;
	vm_offset_t		end;
{
	register ovl_map_entry_t	new_entry;
	register ovl_map_entry_t	prev_entry;
	ovl_map_entry_t				temp_entry;

	/*
	 *	Check that the start and end points are not bogus.
	 */

	if ((start < map->min_offset) || (end > map->max_offset) || (start >= end)) {
		return (KERN_INVALID_ADDRESS);
	}

	prev_entry = temp_entry;

	/*
	 *	Assert that the next entry doesn't overlap the
	 *	end point.
	 */

    if((CIRCLEQ_NEXT(prev_entry, cl_entry) != CIRCLEQ_FIRST(&map->cl_header)) &&
    		(CIRCLEQ_NEXT(prev_entry, cl_entry)->start < end)) {
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
		    (prev_entry->max_protection == VM_PROT_DEFAULT)) {

			/*
			 * TODO: ovl_object_coalesce
			 */
			map->size += (end - prev_entry->end);
			prev_entry->end = end;
			return (KERN_SUCCESS);
		}
	}

	/*
	 *	Create a new entry
	 */
	new_entry = ovl_map_entry_create(map);
	new_entry->start = start;
	new_entry->end = end;

	new_entry->object.ovl_object = object;
	new_entry->offset = offset;

	if (map->is_main_map) {
		new_entry->inheritance = VM_INHERIT_DEFAULT;
		new_entry->protection = VM_PROT_DEFAULT;
		new_entry->max_protection = VM_PROT_DEFAULT;
	}

	/*
	 *	Insert the new entry into the list
	 */
	ovl_map_entry_link(map, prev_entry, new_entry);
	map->size += new_entry->end - new_entry->start;

	/*
	 *	Update the free space hint
	 */

	if ((map->first_free == prev_entry) && (prev_entry->end >= new_entry->start))
		map->first_free = new_entry;

	return (KERN_SUCCESS);
}

#define	OVL_SAVE_HINT(map,value) 			\
		simple_lock(&(map)->hint_lock); 	\
		(map)->hint = (value); 			\
		simple_unlock(&(map)->hint_lock);

bool_t
ovl_map_lookup_entry(map, address, entry)
    register ovl_map_t	    map;
    register vm_offset_t	address;
    ovl_map_entry_t		    *entry;		/* OUT */
{
    register ovl_map_entry_t cur;
    register ovl_map_entry_t	 last;
    bool_t use_tree = FALSE;

	simple_lock(&map->hint_lock);
	cur = map->hint;
	simple_unlock(&map->hint_lock);

    if(address < cur->start) {
        if(ovl_map_search_prev_entry(map, address, cur)) {
            OVL_SAVE_HINT(map, cur);
            KDASSERT((*entry)->start <= address);
            KDASSERT(address < (*entry)->end);
            return (TRUE);
        } else {
            use_tree = TRUE;
            goto search_tree;
        }
    }
    if (address > cur->start){
        if(ovl_map_search_next_entry(map, address, cur)) {
            OVL_SAVE_HINT(map, cur);
            KDASSERT((*entry)->start <= address);
            KDASSERT(address < (*entry)->end);
            return (TRUE);
        } else {
            use_tree = TRUE;
            goto search_tree;
        }
    }

search_tree:

    ovl_tree_sanity(map, __func__);

    if (use_tree) {
        struct ovl_map_entry *prev = CIRCLEQ_FIRST(&map->cl_header);
        cur = RB_ROOT(&map->rb_root);

        while (cur) {
            if(address >= cur->start) {
                if (address < cur->end) {
                    *entry = cur;
                    OVL_SAVE_HINT(map, cur);
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
    OVL_SAVE_HINT(map, *entry);
    KDASSERT((*entry) == CIRCLEQ_FIRST(&map->cl_header) || (*entry)->end <= address);
    KDASSERT(CIRCLEQ_NEXT(*entry, cl_entry) == CIRCLEQ_FIRST(&map->cl_header) || address < CIRCLEQ_NEXT(*entry, cl_entry)->start);
    return (FALSE);
}

static bool_t
ovl_map_search_next_entry(map, address, entry)
    register ovl_map_t	    map;
    register vm_offset_t	address;
    ovl_map_entry_t		    entry;		/* OUT */
{
    register ovl_map_entry_t	cur;
    register ovl_map_entry_t	first = CIRCLEQ_FIRST(&map->cl_header);
    register ovl_map_entry_t	last = CIRCLEQ_LAST(&map->cl_header);

    CIRCLEQ_FOREACH(entry, &map->cl_header, cl_entry) {
        if (address >= first->start) {
            cur = CIRCLEQ_NEXT(first, cl_entry);
            if((cur != last) && (cur->end > address)) {
                cur = entry;
                OVL_SAVE_HINT(map, cur);
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

static bool_t
ovl_map_search_prev_entry(map, address, entry)
    register ovl_map_t	    map;
    register vm_offset_t	address;
    ovl_map_entry_t		    entry;		/* OUT */
{
    register ovl_map_entry_t	cur;
    register ovl_map_entry_t	first = CIRCLEQ_FIRST(&map->cl_header);
    register ovl_map_entry_t	last = CIRCLEQ_LAST(&map->cl_header);

    CIRCLEQ_FOREACH_REVERSE(entry, &map->cl_header, cl_entry) {
        if (address <= last->start) {
            cur = CIRCLEQ_PREV(last, cl_entry);
            if ((cur != first) && (cur->end > address)) {
                cur = entry;
                OVL_SAVE_HINT(map, cur);
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

int
ovl_map_findspace(map, start, length, addr)
	register ovl_map_t map;
	register vm_offset_t start;
	vm_size_t length;
	vm_offset_t *addr;
{
	register ovl_map_entry_t entry, next;
	register vm_offset_t end;
	ovl_map_entry_t tmp;

	ovl_tree_sanity(map, "map_findspace entry");

	if (start < map->min_offset)
		start = map->min_offset;
	if (start > map->max_offset)
		return (1);

	/*
	 * Look for the first possible address; if there's already
	 * something at this address, we have to start after it.
	 */
	if (start == map->min_offset) {
		if ((entry = map->first_free) != CIRCLEQ_FIRST(&map->cl_header))
			start = entry->end;
	} else {
		if (ovl_map_lookup_entry(map, start, &tmp))
			start = tmp->end;
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
	OVL_SAVE_HINT(map, entry);
	*addr = start;
	return (0);
}

int
ovl_map_find(map, object, offset, addr, length, find_space)
	ovl_map_t	map;
	ovl_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	*addr;		/* IN/OUT */
	vm_size_t	length;
	bool_t	find_space;
{
	register vm_offset_t	start;
	int						result;

	start = *addr;
	ovl_map_lock(map);
	if (find_space) {
		if (ovl_map_findspace(map, start, length, addr)) {
			ovl_map_unlock(map);
			return (KERN_NO_SPACE);
		}
		start = *addr;
	}
	result = ovl_map_insert(map, object, offset, start, start + length);
	ovl_map_unlock(map);
	return (result);
}

/*
 *	vm_map_clip_start:	[ internal use only ]
 *
 *	Asserts that the given entry begins at or after
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void
ovl_map_clip_start(map, entry, startaddr)
	register ovl_map_t		map;
	register ovl_map_entry_t	entry;
	register vm_offset_t	startaddr;
{ 													
	if (startaddr > entry->start) {			
		_ovl_map_clip_start(map, entry, startaddr); 
	}
}

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */
static void
_ovl_map_clip_start(map, entry, start)
	register ovl_map_t		map;
	register ovl_map_entry_t	entry;
	register vm_offset_t	start;
{
	register ovl_map_entry_t	new_entry;
	vm_offset_t new_adj;

	/*
	 *	See if we can simplify this entry first
	 */

//	ovl_map_simplify_entry(map, entry);
	ovl_tree_sanity(map, "clip_start entry");

	/*
	 *	Split off the front portion --
	 *	note that we must insert the new
	 *	entry BEFORE this one, so that
	 *	this entry has the specified starting
	 *	address.
	 */

	new_entry = ovl_map_entry_create(map);
	*new_entry = *entry;

	new_entry->end = start;
	new_adj = start - new_entry->start;
	if (entry->object.ovl_object) {
		entry->offset += new_adj;	/* shift start over */
	} else {
		entry->offset += (start - entry->start);
	}

	/* Does not change order for the RB tree */
	entry->start = start;

	ovl_map_entry_link(map, CIRCLEQ_PREV(entry, cl_entry), new_entry);

	if (entry->is_a_map || entry->is_sub_map) {
	 	ovl_map_reference(new_entry->object.share_map);
	} else {
		ovl_object_reference(new_entry->object.ovl_object);
	}

	ovl_tree_sanity(map, "clip_start leave");
}

/*
 *	ovl_map_clip_end:	[ internal use only ]
 *
 *	Asserts that the given entry ends at or before
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void
ovl_map_clip_end(map, entry, endaddr)
	register ovl_map_t		map;
	register ovl_map_entry_t	entry;
	register vm_offset_t	endaddr;
{ 									
	if (endaddr < entry->end) {
		_ovl_map_clip_end(map, entry, endaddr);
	}
}

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */
static void
_ovl_map_clip_end(map, entry, end)
	register ovl_map_t			map;
	register ovl_map_entry_t	entry;
	register vm_offset_t		end;
{
	register ovl_map_entry_t	new_entry;
	vm_offset_t new_adj;

	ovl_tree_sanity(map, "clip_end entry");

	/*
	 *	Create a new entry and insert it
	 *	AFTER the specified entry
	 */

	new_entry = ovl_map_entry_create(map);
	*new_entry = *entry;

	new_entry->start = entry->end = end;
	new_adj = end - entry->start;
	if (new_entry->object.ovl_object) {
		new_entry->offset += new_adj;
	} else {
		new_entry->offset += (end - entry->start);
	}

	ovl_rb_fixup(map, entry);

	ovl_map_entry_link(map, entry, new_entry);

	if (entry->is_a_map || entry->is_sub_map) {
	 	ovl_map_reference(new_entry->object.share_map);
	} else {
		ovl_object_reference(new_entry->object.ovl_object);
	}

	ovl_tree_sanity(map, "clip_end leave");
}

#define	OVL_MAP_RANGE_CHECK(map, start, end) {	\
		if (start < ovl_map_min(map))			\
			start = ovl_map_min(map);			\
		if (end > ovl_map_max(map))				\
			end = ovl_map_max(map);				\
		if (start > end)						\
			start = end;						\
}

/*
 *	ovl_map_submap:		[ kernel use only ]
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
ovl_map_submap(map, start, end, submap)
	register ovl_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
	ovl_map_t				submap;
{
	ovl_map_entry_t		entry;
	register int		result = KERN_INVALID_ARGUMENT;

	ovl_map_lock(map);

	OVL_MAP_RANGE_CHECK(map, start, end);

	if (ovl_map_lookup_entry(map, start, &entry)) {
		ovl_map_clip_start(map, entry, start);
	} else {
		entry = CIRCLEQ_NEXT(entry, cl_entry);
	}

	ovl_map_clip_end(map, entry, end);

	if ((entry->start == start) && (entry->end == end) &&
	    (!entry->is_a_map) &&
	    (entry->object.ovl_object == NULL)) {
		entry->is_a_map = FALSE;
		entry->is_sub_map = TRUE;
		ovl_map_reference(entry->object.sub_map = submap);
		result = KERN_SUCCESS;
	}
	ovl_map_unlock(map);

	return (result);
}

void
ovl_map_entry_delete(map, entry)
	register ovl_map_t			map;
	register ovl_map_entry_t	entry;
{
	ovl_map_entry_unlink(map, entry);
	map->size -= entry->end - entry->start;

	if (entry->is_a_map || entry->is_sub_map) {
		ovl_map_deallocate(entry->object.sub_map);
	} else {
		ovl_object_deallocate(entry->object.ovl_object);
	}

	ovl_map_entry_dispose(map, entry);
}

int
ovl_map_delete(map, start, end)
	register ovl_map_t		map;
	vm_offset_t				start;
	register vm_offset_t	end;
{
	register ovl_map_entry_t	entry;
	ovl_map_entry_t				first_entry;

	/*
	 *	Find the start of the region, and clip it
	 */

	if (!ovl_map_lookup_entry(map, start, &first_entry)) {
		entry = CIRCLEQ_NEXT(first_entry, cl_entry);
	} else {
		entry = first_entry;
		ovl_map_clip_start(map, entry, start);

		/*
		 *	Fix the lookup hint now, rather than each
		 *	time though the loop.
		 */

		OVL_SAVE_HINT(map, CIRCLEQ_PREV(entry, cl_entry));
	}

	/*
	 *	Save the free space hint
	 */

	if (map->first_free->start >= start)
		map->first_free = CIRCLEQ_PREV(entry, cl_entry);

	/*
	 *	Step through all entries in this region
	 */

	while ((entry != CIRCLEQ_FIRST(&map->cl_header)) && (entry->start < end)) {
		ovl_map_entry_t			next;
		register vm_offset_t	s, e;
		register ovl_object_t	object;

		ovl_map_clip_end(map, entry, end);

		next = CIRCLEQ_NEXT(entry, cl_entry);
		s = entry->start;
		e = entry->end;

		/*
		 *	Unwire before removing addresses from the pmap;
		 *	otherwise, unwiring will put the entries back in
		 *	the pmap.
		 */

		object = entry->object.ovl_object;

		if (object == overlay_object || object == omem_object) {
			/* do nothing */
		}

		/*
		 *	Delete the entry (which may delete the object)
		 *	only after removing all pmap entries pointing
		 *	to its pages.  (Otherwise, its page frames may
		 *	be reallocated, and any modify bits will be
		 *	set in the wrong object!)
		 */

		ovl_map_entry_delete(map, entry);
		entry = next;
	}
	return (KERN_SUCCESS);
}

int
ovl_map_remove(map, start, end)
	register ovl_map_t		map;
	register vm_offset_t	start;
	register vm_offset_t	end;
{
	register int			result;

	ovl_map_lock(map);
	OVL_MAP_RANGE_CHECK(map, start, end);
	result = ovl_map_delete(map, start, end);
	ovl_map_unlock(map);

	return (result);
}

static void
ovlmap_mapin(map, entry)
	ovl_map_t 	map;
	ovl_map_entry_t entry;
{
	ovl_object_t object;

	object = entry->object.ovl_object;
	/* check object */
	if (object != NULL) {
		/* mapin the map and the map entry object. */
		pmap_overlay_mapin(map, object);
	}
}

static void
ovlmap_mapout(map, entry)
	ovl_map_t 	map;
	ovl_map_entry_t entry;
{
	ovl_object_t object;

	if (map == NULL || entry == NULL) {
		return;
	}

	object = entry->object.ovl_object;
	if (object != NULL) {
		/* mapout the map and the map entry object. Hope we return alive! */
		pmap_overlay_mapout(map, object);
	}
}

void
ovlspace_mapout(ovl)
	struct ovlspace *ovl;
{
	ovl_map_t 	old_map;
	ovl_map_entry_t old_entry;

	old_map = &ovl->ovl_map;

	ovl_map_lock(old_map);
	if (!CIRCLEQ_EMPTY(&old_map->cl_header)) {
		CIRCLEQ_FOREACH(old_entry, &old_map->cl_header, cl_entry) {
			/* place old map into a temporary holding */
			ovlmap_mapout(old_map, old_entry);
		}
	}
	ovl_map_unlock(old_map);
}

void
ovlspace_mapin(ovl)
	struct ovlspace *ovl;
{
	struct ovl_map 		old_map;
	struct ovl_map_entry 	old_entry;
	ovl_map_t 		new_map;
	ovl_map_entry_t 	new_entry;
	int 			found;

	found = 1;
	
	/* Retrieve old map from it's temporary holding */
	ovlmap_mapin(&old_map, &old_entry);

	new_map = &old_map;
	if ((new_map != NULL) && (&old_entry != NULL)) {
		ovl_map_lock(new_map);
		/* sanity check */
		if (!CIRCLEQ_EMPTY(&new_map->cl_header)) {
			CIRCLEQ_FOREACH(new_entry, &new_map->cl_header, cl_entry) {
				if (new_entry == &old_entry) {
					new_entry = &old_entry;
					found = 0;
					/* success: we have reloaded the map and map entries */
					break;
				}
			}
		}
		ovl_map_unlock(new_map);
		if (found) {
			ovl->ovl_map = *new_map;
		}
	}
	/*
	 * TODO: Implement A fallback
	 *	- Something probably like vmspace_fork
	 */
}
