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
#include <devel/vm/include/vm.h>

#include <devel/sys/malloctypes.h>
#include <devel/ovl/include/ovl.h>

#undef RB_AUGMENT
#define	RB_AUGMENT(x)	ovl_rb_augment(x)

vm_offset_t				oentry_data;
vm_size_t				oentry_data_size;
ovl_map_entry_t 		oentry_free;
ovl_map_t 				omap_free;

void
ovl_map_startup()
{
    register int i;
    register ovl_map_entry_t mep;
    ovl_map_t mp;

    omap_free = mp = (ovl_map_t) oentry_data;
    i = MAX_OMAP;

    while (--i > 0) {
        CIRCLEQ_NEXT(mep, ovl_cl_entry) = (ovl_map_entry_t) (mp + 1);
        mp++;
    }
    CIRCLEQ_FIRST(&mp->ovl_header)++->ovl_cl_entry.cqe_next = NULL;

    oentry_free = mep = (ovl_map_entry_t) mp;
    i = (oentry_data_size - MAX_OMAP * sizeof * mp) / sizeof *mep;
    while (--i > 0) {
        CIRCLEQ_NEXT(mep, ovl_cl_entry) = mep + 1;
        mep++;
    }
    CIRCLEQ_NEXT(mep, ovl_cl_entry) = NULL;
}

/* Allocate ovlspace
 * segmented(TRUE/FALSE): allocation of the 3 process segments (data, stack, text)
 */
struct ovlspace *
ovlspace_alloc(min, max)
	vm_offset_t min, max;
{
	register struct ovlspace *ovl;

	OVERLAY_MALLOC(ovl, struct ovlspace *, sizeof(struct ovlspace *), M_OVLMAP, M_WAITOK);

	memset(ovl, 0, sizeof(struct ovlspace *));
	ovl_map_init(&ovl->ovl_map, min, max);
	pmap_pinit(&ovl->ovl_pmap);
	ovl->ovl_map.ovl_pmap = &ovl->ovl_pmap;
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
		pmap_release(&ovl->ovl_pmap);
		OVERLAY_FREE(ovl, M_OVLMAP);
	}
}

/*
 * Red black tree functions
 *
 * The caller must hold the related map lock.
 */
RB_GENERATE(ovl_map_rb_tree, ovl_map_entry, ovl_rb_entry, ovl_rb_compare);

static int
ovl_rb_compare(a, b)
	ovl_map_entry_t a, b;
{
	if (a->ovle_start < b->ovle_start)
		return(-1);
	else if (a->ovle_start > b->ovle_start)
		return (1);
	return(0);
}

static void
ovl_rb_augment(entry)
	struct ovl_map_entry *entry;
{
	entry->ovle_space = ovl_rb_subtree_space(entry);
}

static size_t
ovl_rb_space(map, entry)
    const struct ovl_map *map;
    const struct ovl_map_entry *entry;
{
    KASSERT(CIRCLEQ_NEXT(entry, ovl_cl_entry) != NULL);
    return (CIRCLEQ_NEXT(entry, ovl_cl_entry)->ovle_start - CIRCLEQ_FIRST(&map->ovl_header)->ovle_end);
}

static size_t
ovl_rb_subtree_space(entry)
	const struct ovl_map_entry *entry;
{
	caddr_t space, tmp;

	space = entry->ovle_ownspace;
	if (RB_LEFT(entry, ovl_rb_entry)) {
		tmp = RB_LEFT(entry, ovl_rb_entry)->ovle_space;
		if (tmp > space)
			space = tmp;
	}

	if (RB_RIGHT(entry, ovl_rb_entry)) {
		tmp = RB_RIGHT(entry, ovl_rb_entry)->ovle_space;
		if (tmp > space)
			space = tmp;
	}

	return (space);
}

static void
ovl_rb_fixup(map, entry)
	struct ovl_map *map;
	struct ovl_map_entry *entry;
{
	/* We need to traverse to the very top */
	do {
		entry->ovle_ownspace = ovl_rb_space(map, entry);
		entry->ovle_space = ovl_rb_subtree_space(entry);
	} while ((entry = RB_PARENT(entry, ovl_rb_entry)) != NULL);
}

static void
ovl_rb_insert(map, entry)
    struct ovl_map *map;
    struct ovl_map_entry *entry;
{
    caddr_t space = ovl_rb_space(map, entry);
    struct ovl_map_entry *tmp;

    entry->ovle_ownspace = entry->ovle_space = space;
    tmp = RB_INSERT(ovl_map_rb_tree, &(map)->ovl_root, entry);
#ifdef DIAGNOSTIC
    if (tmp != NULL)
		panic("ovl_rb_insert: duplicate entry?");
#endif
    ovl_rb_fixup(map, entry);
    if (CIRCLEQ_PREV(entry, ovl_cl_entry) != RB_ROOT(&map->ovl_root))
        ovl_rb_fixup(map, CIRCLEQ_PREV(entry, ovl_cl_entry));
}

static void
ovl_rb_remove(map, entry)
    struct ovl_map *map;
    struct ovl_map_entry *entry;
{
    struct ovl_map_entry *parent;

    parent = RB_PARENT(entry, ovl_rb_entry);
    RB_REMOVE(ovl_map_rb_tree, &(map)->ovl_root, entry);
    if (CIRCLEQ_PREV(entry, ovl_cl_entry) != CIRCLEQ_FIRST(&map->ovl_header))
        ovl_rb_fixup(map, CIRCLEQ_PREV(entry, ovl_cl_entry));
    if (parent)
        ovl_rb_fixup(map, parent);
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
	struct ovl_map *map;
	const char *name;
{
	struct ovl_map_entry *tmp, *trtmp;
	int n = 0, i = 1;

	RB_FOREACH(tmp, ovl_map_rb_tree, &map->ovl_root) {
		if (tmp->ovle_ownspace != ovl_rb_space(map, tmp)) {
			printf("%s: %d/%d ownspace %lx != %lx %s\n",
			    name, n + 1, map->ovl_nentries, (u_long)tmp->ovle_ownspace, (u_long)ovl_rb_space(map, tmp),
				CIRCLEQ_NEXT(tmp, ovl_cl_entry) == CIRCLEQ_FIRST(&map->ovl_header) ? "(last)" : "");
			goto error;
		}
	}
	trtmp = NULL;
	RB_FOREACH(tmp, ovl_map_rb_tree, &map->ovl_root) {
		if (tmp->ovle_space != ovl_rb_subtree_space(tmp)) {
			printf("%s: space %lx != %lx\n", name, (u_long)tmp->ovle_space, (u_long)ovl_rb_subtree_space(tmp));
			goto error;
		}
		if (trtmp != NULL && trtmp->ovle_start >= tmp->ovle_start) {
			printf("%s: corrupt: 0x%lx >= 0x%lx\n", name, trtmp->ovle_start, tmp->ovle_start);
			goto error;
		}
		n++;

		trtmp = tmp;
	}

	if (n != map->ovl_nentries) {
		printf("%s: nentries: %d vs %d\n", name, n, map->ovl_nentries);
		goto error;
	}

	for (tmp = CIRCLEQ_FIRST(&map->ovl_header)->ovl_cl_entry.cqe_next; tmp && tmp != CIRCLEQ_FIRST(&map->ovl_header);
	    tmp = CIRCLEQ_NEXT(tmp, ovl_cl_entry), i++) {
		trtmp = RB_FIND(ovl_map_rb_tree, &map->ovl_root, tmp);
		if (trtmp != tmp) {
			printf("%s: lookup: %d: %p - %p: %p\n", name, i, tmp, trtmp, RB_PARENT(tmp, ovl_rb_entry));
			goto error;
		}
	}

	return (0);
error:
	return (-1);
}

/* Circular List Functions */
static size_t
ovl_cl_space(map, entry)
    const struct ovl_map *map;
    const struct ovl_map_entry *entry;
{
    size_t space, tmp;
    space = entry->ovle_ownspace;

    if(CIRCLEQ_FIRST(&map->ovl_header)) {
        tmp = CIRCLEQ_FIRST(&map->ovl_header)->ovle_space;
        if(tmp > space) {
            space = tmp;
        }
    }

    if(CIRCLEQ_LAST(&map->ovl_header)) {
        tmp = CIRCLEQ_LAST(&map->ovl_header)->ovle_space;
        if(tmp > space) {
            space = tmp;
        }
    }

    return (space);
}

static void
ovl_cl_insert(map, entry)
    struct ovl_map *map;
    struct ovl_map_entry *entry;
{
    struct ovl_map_entry *head, *tail;
    head = CIRCLEQ_FIRST(&map->ovl_header);
    tail = CIRCLEQ_LAST(&map->ovl_header);

    size_t space = ovl_rb_space(map, entry);
    entry->ovle_ownspace = entry->ovle_space = space;

    if(head->ovle_space == ovl_cl_space(map, entry)) {
        CIRCLEQ_INSERT_HEAD(&map->ovl_header, head, ovl_cl_entry);
    }
    if(tail->ovle_space == ovl_cl_space(map, entry)) {
        CIRCLEQ_INSERT_TAIL(&map->ovl_header, tail, ovl_cl_entry);
    }
}

static void
ovl_cl_remove(map, entry)
    struct ovl_map *map;
    struct ovl_map_entry *entry;
{
    struct ovl_map_entry *head, *tail;
    head = CIRCLEQ_FIRST(&map->ovl_header);
    tail = CIRCLEQ_LAST(&map->ovl_header);

    if(head && ovl_cl_space(map, entry)) {
        CIRCLEQ_REMOVE(&map->ovl_header, head, ovl_cl_entry);
    }
    if(tail && ovl_cl_space(map, entry)) {
        CIRCLEQ_REMOVE(&map->ovl_header, tail, ovl_cl_entry);
    }
}

ovl_map_t
ovl_map_create(pmap, min, max)
	pmap_t		pmap;
	vm_offset_t	min, max;
{
	register ovl_map_t	result;
	extern ovl_map_t	omem_map;
	if(omem_map == NULL) {
		result = omap_free;
		if (result == NULL) {
			panic("ovl_map_create: out of maps");
		}
		omap_free = (ovl_map_t) CIRCLEQ_FIRST(&result->ovl_header)->ovl_cl_entry.cqe_next;
	} else {
		OVERLAY_MALLOC(result, struct ovl_map, sizeof(struct ovl_map), M_OVLMAP, M_WAITOK);
	}

	result->ovl_pmap = pmap;
	return (result);
}

void
ovl_map_init(map, min, max)
	ovl_map_t map;
	vm_offset_t	min, max;
{
	CIRCLEQ_INIT(&map->ovl_header);
	RB_INIT(&map->ovl_root);
	map->ovl_nentries = 0;
	map->ovl_size = 0;
	map->ovl_ref_count = 1;
	map->ovl_is_main_map = TRUE;
	map->min_offset = min;
	map->max_offset = max;
	map->ovl_hint = &map->ovl_header;
	map->ovl_timestamp = 0;
	lockinit(&map->ovl_lock, PVM, "thrd_sleep", 0, 0);
	simple_lock_init(&map->ovl_ref_lock);
	simple_lock_init(&map->ovl_hint_lock);
}

ovl_map_entry_t
ovl_map_entry_create(map)
	ovl_map_t		map;
{
	ovl_map_entry_t	entry;
#ifdef DEBUG
	extern ovl_map_t	overlay_map, omem_map;
	isspecial = (map == overlay_map || map == omem_map);
	if (isspecial || !isspecial) {
		panic("ovl_map_entry_create: bogus map");
	}
#endif
	OVERLAY_MALLOC(entry, struct ovl_map_entry, sizeof(struct ovl_map_entry), M_OVLMAPENT, M_WAITOK);
	if(entry == oentry_free) {
		oentry_free = CIRCLEQ_NEXT(oentry_free, ovl_cl_entry);
	}
	if (entry == NULL)
		panic("ovl_map_entry_create: out of map entries");

	return(entry);
}

void
ovl_map_entry_dispose(map, entry)
	ovl_map_t		map;
	ovl_map_entry_t	entry;
{
#ifdef DEBUG
	extern ovl_map_t	overlay_map, omem_map;
	isspecial = (map == overlay_map || map == omem_map);
	if (isspecial || !isspecial) {
		panic("ovl_map_entry_dispose: bogus map");
	}
#endif
	OVERLAY_FREE(entry, M_OVLMAPENT);
	CIRCLEQ_NEXT(entry, ovl_cl_entry) = oentry_free;
	oentry_free = entry;
}

#define	ovl_map_entry_link(map, after_where, entry) { 							\
		(map)->ovl_nentries++; 													\
	    (entry)->ovl_cl_entry.cqe_prev = (after_where);                   		\
	    (entry)->ovl_cl_entry.cqe_next = (after_where)->ovl_cl_entry.cqe_next; 	\
	    (entry)->ovl_cl_entry.cqe_prev->ovl_cl_entry.cqe_next = (entry);       	\
	    (entry)->ovl_cl_entry.cqe_next->ovl_cl_entry.cqe_prev = (entry);       	\
		ovl_cl_insert((map), (entry));                   						\
		ovl_rb_insert((map), (entry));  										\
}

#define	ovl_map_entry_unlink(map, entry) { 										\
		(map)->ovl_nentries--; 													\
		ovl_cl_remove((map), (entry)); 			        						\
		ovl_rb_remove((map), (entry));  										\
}

void
ovl_map_reference(map)
	register ovl_map_t	map;
{
	if (map == NULL)
		return;

	simple_lock(&map->ovl_ref_lock);
#ifdef DEBUG
	if (map->ovl_ref_count == 0)
		panic("ovl_map_reference: zero ref_count");
#endif
	map->ovl_ref_count++;
	simple_unlock(&map->ovl_ref_lock);
}

void
ovl_map_deallocate(map)
	register ovl_map_t	map;
{
	if (map == NULL)
		return;

	simple_lock(&map->ovl_ref_lock);
	if (map->ovl_ref_count-- > 0) {
		simple_unlock(&map->ovl_ref_lock);
		return;
	}

	/*
	 *	Lock the map, to wait out all other references
	 *	to it.
	 */

	ovl_map_lock_drain_interlock(map);

	(void)ovl_map_delete(map, map->min_offset, map->max_offset);

	//pmap_destroy(map->ovl_pmap);

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

	if ((start < map->min_offset) || (end > map->max_offset) || (start >= end))
		return(KERN_INVALID_ADDRESS);

	prev_entry = temp_entry;

	/*
	 *	Assert that the next entry doesn't overlap the
	 *	end point.
	 */

    if((CIRCLEQ_NEXT(prev_entry, ovl_cl_entry) != CIRCLEQ_FIRST(&map->ovl_header)) &&
    		(CIRCLEQ_NEXT(prev_entry, ovl_cl_entry)->ovle_start < end)) {
        return (KERN_NO_SPACE);
    }

	/*
	 *	See if we can avoid creating a new entry by
	 *	extending one of our neighbors.
	 */
	if (object == NULL) {
		if ((prev_entry != &map->ovl_header) &&
				(prev_entry->ovle_end == start) &&
				(map->ovl_is_main_map) &&
				(prev_entry->ovle_inheritance == VM_INHERIT_DEFAULT) &&
				(prev_entry->ovle_protection == VM_PROT_DEFAULT) &&
				(prev_entry->ovle_max_protection == VM_PROT_DEFAULT)) {

			map->ovl_size += (end - prev_entry->ovle_end);
			prev_entry->ovle_end = end;
			return (KERN_SUCCESS);
		}
	}

	/*
	 *	Create a new entry
	 */
	new_entry = ovl_map_entry_create(map);
	new_entry->ovle_start = start;
	new_entry->ovle_end = end;

	new_entry->ovle_object.ovl_object = object;
	new_entry->ovle_offset = offset;

	if (map->ovl_is_main_map) {
		new_entry->ovle_inheritance = VM_INHERIT_DEFAULT;
		new_entry->ovle_protection = VM_PROT_DEFAULT;
		new_entry->ovle_max_protection = VM_PROT_DEFAULT;
	}

	/*
	 *	Insert the new entry into the list
	 */
	ovl_map_entry_link(map, prev_entry, new_entry);
	map->ovl_size += new_entry->ovle_end - new_entry->ovle_start;

	/*
	 *	Update the free space hint
	 */

	if ((map->ovl_first_free == prev_entry) && (prev_entry->ovle_end >= new_entry->ovle_start))
		map->ovl_first_free = new_entry;

	return(KERN_SUCCESS);
}

#define	SAVE_HINT(map, value) 					\
		simple_lock(&(map)->ovl_hint_lock); 	\
		(map)->ovl_hint = (value); 				\
		simple_unlock(&(map)->ovl_hint_lock);

boolean_t
ovl_map_lookup_entry(map, address, entry)
    register ovl_map_t	    map;
    register vm_offset_t	address;
    ovl_map_entry_t		    *entry;		/* OUT */
{
    register ovl_map_entry_t cur;
    boolean_t use_tree = FALSE;

	simple_lock(&map->ovl_hint_lock);
	cur = map->ovl_hint;
	simple_unlock(&map->ovl_hint_lock);

    if(address < cur->ovle_start) {
        if(ovl_map_search_prev_entry(map, address, cur)) {
            SAVE_HINT(map, cur);
            KDASSERT((*entry)->ovle_start <= address);
            KDASSERT(address < (*entry)->ovle_end);
            return (TRUE);
        } else {
            use_tree = TRUE;
            goto search_tree;
        }
    }
    if (address > cur->ovle_start){
        if(ovl_map_search_next_entry(map, address, cur)) {
            SAVE_HINT(map, cur);
            KDASSERT((*entry)->ovle_start <= address);
            KDASSERT(address < (*entry)->ovle_end);
            return (TRUE);
        } else {
            use_tree = TRUE;
            goto search_tree;
        }
    }

search_tree:

    ovl_tree_sanity(map, __func__);

    if (use_tree) {
        struct ovl_map_entry *prev = CIRCLEQ_FIRST(&map->ovl_header);
        cur = RB_ROOT(&map->ovl_root);

        while (cur) {
            if(address >= cur->ovle_start) {
                if (address < cur->ovle_end) {
                    *entry = cur;
                    SAVE_HINT(map, cur);
                    KDASSERT((*entry)->ovle_start <= address);
                    KDASSERT(address < (*entry)->ovle_end);
                    return (TRUE);
                }
                prev = cur;
                cur = RB_RIGHT(cur, ovl_rb_entry);
            } else {
                cur = RB_LEFT(cur, ovl_rb_entry);
            }
        }
        *entry = prev;
        goto failed;
    }

failed:
    SAVE_HINT(map, entry);
    KDASSERT((*entry) == CIRCLEQ_FIRST(&map->ovl_header) || (*entry)->ovle_end <= address);
    KDASSERT(CIRCLEQ_NEXT(*entry, ovl_cl_entry) == CIRCLEQ_FIRST(&map->ovl_header) || address < CIRCLEQ_NEXT(*entry, ovl_cl_entry)->ovle_start);
    return (FALSE);
}

static boolean_t
ovl_map_search_next_entry(map, address, entry)
    register ovl_map_t	    map;
    register vm_offset_t	address;
    ovl_map_entry_t		    entry;		/* OUT */
{
    register ovl_map_entry_t	cur;
    register ovl_map_entry_t	first = CIRCLEQ_FIRST(&map->ovl_header);
    register ovl_map_entry_t	last = CIRCLEQ_LAST(&map->ovl_header);

    CIRCLEQ_FOREACH(entry, &map->ovl_header, ovl_cl_entry) {
        if(address >= first->ovle_start) {
            cur = CIRCLEQ_NEXT(first, ovl_cl_entry);
            if((cur != last) && (cur->ovle_end > address)) {
                cur = entry;
                SAVE_HINT(map, cur);
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

static boolean_t
ovl_map_search_prev_entry(map, address, entry)
    register ovl_map_t	    map;
    register vm_offset_t	address;
    ovl_map_entry_t		    entry;		/* OUT */
{
    register ovl_map_entry_t	cur;
    register ovl_map_entry_t	first = CIRCLEQ_FIRST(&map->ovl_header);
    register ovl_map_entry_t	last = CIRCLEQ_LAST(&map->ovl_header);

    CIRCLEQ_FOREACH(entry, &map->ovl_header, ovl_cl_entry) {
        if(address >= last->ovle_start) {
            cur = CIRCLEQ_PREV(last, ovl_cl_entry);
            if((cur != first) && (cur->ovle_end > address)) {
                cur = entry;
                SAVE_HINT(map, cur);
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
	register ovl_map_entry_t entry, next, tmp;
	register vm_offset_t end;

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
		if ((entry = map->ovl_first_free) != CIRCLEQ_FIRST(&map->ovl_header))
			start = entry->ovle_end;
	} else {
		if (ovl_map_lookup_entry(map, start, &tmp))
			start = tmp->ovle_end;
		entry = tmp;
	}

	/* If there is not enough space in the whole tree, we fail */
	tmp = RB_ROOT(&map->ovl_root);
	if (tmp == NULL || tmp->ovle_space < length) {
		return (1);
	}

	/*
	 * Look through the rest of the map, trying to fit a new region in
	 * the gap between existing regions, or after the very last region.
	 */
	for (;; start = (entry = next)->ovle_end) {
		/*
		 * Find the end of the proposed new region.  Be sure we didn't
		 * go beyond the end of the map, or wrap around the address;
		 * if so, we lose.  Otherwise, if this is the last entry, or
		 * if the proposed new region fits before the next entry, we
		 * win.
		 */
		end = start + length;
		if (end > map->max_offset || end < start)
			return (1);
        next = CIRCLEQ_NEXT(entry, ovl_cl_entry);
        if (next == CIRCLEQ_FIRST(&map->ovl_header) || next->ovle_start >= end)
            break;
	}
	SAVE_HINT(map, entry);
	*addr = start;
	return (0);
}

int
ovl_map_find(map, object, offset, addr, length, find_space)
	ovl_map_t	map;
	vm_object_t	object;
	vm_offset_t	offset;
	vm_offset_t	*addr;		/* IN/OUT */
	vm_size_t	length;
	boolean_t	find_space;
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
	}
	 else
		entry = CIRCLEQ_NEXT(entry, ovl_cl_entry);

	ovl_map_clip_end(map, entry, end);

	if ((entry->ovle_start == start) && (entry->ovle_end == end) &&
	    (!entry->ovle_is_a_map) &&
	    (entry->ovle_object.ovl_object == NULL)) {
		entry->ovle_is_a_map = FALSE;
		entry->ovle_is_sub_map = TRUE;
		ovl_map_reference(entry->ovle_object.ovl_sub_map = submap);
		result = KERN_SUCCESS;
	}
	ovl_map_unlock(map);

	return(result);
}

void
ovl_map_entry_delete(map, entry)
	register ovl_map_t			map;
	register ovl_map_entry_t	entry;
{
	ovl_map_entry_unlink(map, entry);
	map->ovl_size -= entry->ovle_end - entry->ovle_start;

	if (entry->ovle_is_a_map || entry->ovle_is_sub_map)
		ovl_map_deallocate(entry->ovle_object.ovl_sub_map);
	else
		ovl_map_deallocate(entry->ovle_object.ovl_object);

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

	if (!ovl_map_lookup_entry(map, start, &first_entry))
		entry = CIRCLEQ_NEXT(first_entry, ovl_cl_entry);
	else {
		entry = first_entry;
		//vm_map_clip_start(map, entry, start);

		/*
		 *	Fix the lookup hint now, rather than each
		 *	time though the loop.
		 */

		SAVE_HINT(map, CIRCLEQ_PREV(entry, ovl_cl_entry));
	}

	/*
	 *	Save the free space hint
	 */

	if (map->ovl_first_free->ovle_start >= start)
		map->ovl_first_free = CIRCLEQ_PREV(entry, ovl_cl_entry);

	/*
	 *	Step through all entries in this region
	 */

	while ((entry != CIRCLEQ_FIRST(&map->ovl_header)) && (entry->ovle_start < end)) {
		ovl_map_entry_t			next;
		register vm_offset_t	s, e;
		register ovl_object_t	object;

		//ovl_map_clip_end(map, entry, end);

		next = CIRCLEQ_NEXT(entry, ovl_cl_entry);
		s = entry->ovle_start;
		e = entry->ovle_end;

		/*
		 *	Unwire before removing addresses from the pmap;
		 *	otherwise, unwiring will put the entries back in
		 *	the pmap.
		 */

		object = entry->ovle_object.ovl_object;

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
	return(KERN_SUCCESS);
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
