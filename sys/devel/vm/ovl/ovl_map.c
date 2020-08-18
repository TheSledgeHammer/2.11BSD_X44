/*
 * ovl_map.c
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include "vm/ovl/ovl_map.h"
#include "vm/ovl/ovl.h"

#undef RB_AUGMENT
#define	RB_AUGMENT(x)	ovl_rb_augment(x)

vm_offset_t			kovl_data;
ovl_map_entry_t 	kovl_entry_free;
ovl_map_t 			kovl_free;

//vm_map_t			overlay_map;

vm_offset_t			voverlay_start;
vm_offset_t 		voverlay_end;


//#ifdef OVL
extern struct pmap	overlay_pmap_store;
#define overlay_pmap (&overlay_pmap_store)
//#endif

vm_offset_t
pmap_map_overlay(virt, start, end, prot)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int			prot;
{
	while (start < end) {
		pmap_enter(overlay_pmap, virt, start, prot, FALSE);
		virt += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	return (virt);
}

/*
 * Red black tree functions
 *
 * The caller must hold the related map lock.
 */
RB_GENERATE(ovl_map_rb_tree, ovl_map_entry, orb_entry, ovl_rb_compare);

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
    KASSERT(CIRCLEQ_NEXT(entry, ocl_entry) != NULL);
    return (CIRCLEQ_NEXT(entry, ocl_entry)->ovle_start - CIRCLEQ_FIRST(&map->ovl_header)->ovle_end);
}

static size_t
ovl_rb_subtree_space(entry)
	const struct ovl_map_entry *entry;
{
	caddr_t space, tmp;

	space = entry->ovle_ownspace;
	if (RB_LEFT(entry, orb_entry)) {
		tmp = RB_LEFT(entry, orb_entry)->ovle_space;
		if (tmp > space)
			space = tmp;
	}

	if (RB_RIGHT(entry, orb_entry)) {
		tmp = RB_RIGHT(entry, orb_entry)->ovle_space;
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
		entry->ovle_ownspace = vm_rb_space(map, entry);
		entry->ovle_space = vm_rb_subtree_space(entry);
	} while ((entry = RB_PARENT(entry, orb_entry)) != NULL);
}

static void
ovl_rb_insert(map, entry)
    struct ovl_map *map;
    struct ovl_map_entry *entry;
{
    caddr_t space = vm_rb_space(map, entry);
    struct ovl_map_entry *tmp;

    entry->ovle_ownspace = entry->ovle_space = space;
    tmp = RB_INSERT(vm_map_rb_tree, &(map)->ovl_root, entry);
#ifdef DIAGNOSTIC
    if (tmp != NULL)
		panic("vm_rb_insert: duplicate entry?");
#endif
    vm_rb_fixup(map, entry);
    if (CIRCLEQ_PREV(entry, ocl_entry) != RB_ROOT(&map->ovl_root))
        vm_rb_fixup(map, CIRCLEQ_PREV(entry, ocl_entry));
}

static void
ovl_rb_remove(map, entry)
    struct ovl_map *map;
    struct ovl_map_entry *entry;
{
    struct ovl_map_entry *parent;

    parent = RB_PARENT(entry, orb_entry);
    RB_REMOVE(vm_map_rb_tree, &(map)->ovl_root, entry);
    if (CIRCLEQ_PREV(entry, ocl_entry) != CIRCLEQ_FIRST(&map->ovl_header))
        vm_rb_fixup(map, CIRCLEQ_PREV(entry, ocl_entry));
    if (parent)
        vm_rb_fixup(map, parent);
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
		if (tmp->ovle_ownspace != vm_rb_space(map, tmp)) {
			printf("%s: %d/%d ownspace %lx != %lx %s\n",
			    name, n + 1, map->nentries, (u_long)tmp->ovle_ownspace, (u_long)vm_rb_space(map, tmp),
				CIRCLEQ_NEXT(tmp, ocl_entry) == CIRCLEQ_FIRST(&map->ovl_header) ? "(last)" : "");
			goto error;
		}
	}
	trtmp = NULL;
	RB_FOREACH(tmp, ovl_map_rb_tree, &map->ovl_root) {
		if (tmp->ovle_space != vm_rb_subtree_space(tmp)) {
			printf("%s: space %lx != %lx\n", name, (u_long)tmp->ovle_space, (u_long)vm_rb_subtree_space(tmp));
			goto error;
		}
		if (trtmp != NULL && trtmp->ovle_start >= tmp->ovle_start) {
			printf("%s: corrupt: 0x%lx >= 0x%lx\n", name, trtmp->ovle_start, tmp->ovle_start);
			goto error;
		}
		n++;

		trtmp = tmp;
	}

	if (n != map->nentries) {
		printf("%s: nentries: %d vs %d\n", name, n, map->nentries);
		goto error;
	}

	for (tmp = CIRCLEQ_FIRST(&map->ovl_header)->ocl_entry.cqe_next; tmp && tmp != CIRCLEQ_FIRST(&map->ovl_header);
	    tmp = CIRCLEQ_NEXT(tmp, ocl_entry), i++) {
		trtmp = RB_FIND(vm_map_rb_tree, &map->ovl_root, tmp);
		if (trtmp != tmp) {
			printf("%s: lookup: %d: %p - %p: %p\n", name, i, tmp, trtmp, RB_PARENT(tmp, orb_entry));
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
        CIRCLEQ_INSERT_HEAD(&map->ovl_header, head, ocl_entry);
    }
    if(tail->ovle_space == ovl_cl_space(map, entry)) {
        CIRCLEQ_INSERT_TAIL(&map->ovl_header, tail, ocl_entry);
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

    if(head && vm_cl_space(map, entry)) {
        CIRCLEQ_REMOVE(&map->ovl_header, head, ocl_entry);
    }
    if(tail && vm_cl_space(map, entry)) {
        CIRCLEQ_REMOVE(&map->ovl_header, tail, ocl_entry);
    }
}

ovl_map_t
ovl_map_create(pmap, min, max)
	pmap_t		pmap;
	vm_offset_t	min, max;
{
	register ovl_map_t	result;
	extern ovl_map_t	kmem_ovl;

	result->pmap = pmap;
	return (result);
}

void
ovl_map_init(map, min, max)
	struct ovl_map *map;
	vm_offset_t	min, max;
{
	CIRCLEQ_INIT(&map->ovl_header);
	RB_INIT(&map->ovl_root);
	map->nentries = 0;
	map->size = 0;
	map->ref_count = 1;
	//map->is_main_map = TRUE;
	map->min_offset = min;
	map->max_offset = max;
	map->hint = &map->ovl_header;
	map->timestamp = 0;
	lockinit(&map->lock, PVM, "thrd_sleep", 0, 0);
	simple_lock_init(&map->ref_lock);
	simple_lock_init(&map->hint_lock);
}


/* ovl_swapin, ovl_swapout? */
