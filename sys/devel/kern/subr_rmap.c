/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)subr_rmap.c	1.2 (2.11BSD GTE) 12/24/92
 */
/*-
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 *	@(#)subr_rmap.c	8.2 (Berkeley) 01/21/94
 */


#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/dmap.h>		/* XXX */
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>

#include <devel/vm/include/vm_text.h>

extern struct mapent _coremap[];
extern struct mapent _swapmap[];

void
rminit(mp, size, addr, name, mtype, mapsize)
	register struct map *mp;
	memaddr_t addr;
	size_t size;
	char *name;
	int mtype, mapsize;
{
	struct mapent *ep;

	/* mapsize had better be at least 2 */
	if (mapsize < 2 || addr <= 0 || size < 0) {
		panic("rminit %s", name);
	}
	mp->m_name = name;
	mp->m_type = mtype;
	mp->m_limit = (struct mapent *)mp[mapsize];

	/* initially the first entry describes all free space */
	ep = (struct mapent *)(mp + 1);
	ep->m_size = size;
	ep->m_addr = addr;

	/* the remaining slots are unused (indicated by m_addr == 0) */
	while (++ep < mp->m_limit) {
		ep->m_addr = 0;
	}
}

/*
 * Allocate 'size' units from the given map.  Return the base of the
 * allocated space.  In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 *
 * Algorithm is first-fit.
 */
memaddr_t
rmalloc(mp, size)
	struct map *mp;
	register size_t size;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	register struct mapent *bp;
	swblk_t first, rest;
	memaddr_t addr;
	int retry;

	if (!size) {
		panic("rmalloc: size = 0");
	}
	if(mp == swapmap && size > dmmax) {
		panic("rmalloc");
	}

	/*
	 * Search for a piece of the resource map which has enough
	 * free space to accomodate the request.
	 */
	retry = 0;
again:
	for (bp = mp->m_map; bp->m_size; ++bp) {
		if (bp->m_size >= size) {
			/*
			 * Allocate from the map.  If we allocated the entire
			 * piece, move the rest of the map to the left.
			 */
			addr = bp->m_addr;
			bp->m_size -= size;
			if (bp->m_size) {
				bp->m_addr += size;
			} else {
				for (ep = bp;; ++ep) {
					*ep = *++bp;
					if (!bp->m_size) {
						break;
					}
				}
			}
			return (addr);
		}
	}
	/* no entries big enough */
	if (!retry++) {
		if (mp == swapmap && nswdev > 1	&& (first = dmmax - bp->m_addr % dmmax) < size) {
			if (bp->m_size - first < size) {
				continue;
			}
			addr = bp->m_addr + first;
			rest = bp->m_size - first - size;
			bp->m_size = first;
			if (rest) {
				printf("short of swap\n");
				//rmfree(swapmap, rest, addr+size);
				vm_xumount(NODEV);
			}
			goto again;
		} else if (mp == coremap) {
			vm_xuncore(size);
			goto again;
		}
	}
	return (0);
}

/*
 * Free the previously allocated size units at addr into the specified
 * map.  Sort addr into map and combine on one or both ends if possible.
 */
void
rmfree(mp, size, addr)
	struct map *mp;
	size_t size;
	register memaddr_t addr;
{
	register struct mapent *bp, *ep;
	struct mapent *start;

	if (!size)
		return;
	/* the address must not be 0, or the protocol has broken down. */
	if (!addr) {
		panic("rmfree: addr = 0");
	}
	if (mp == coremap) {
		if (runin) {
			runin = 0;
			wakeup((caddr_t) & runin);
		}
	}

}

/*
 * Allocate resources for the three segments of a process (data, stack
 * and u.), attempting to minimize the cost of failure part-way through.
 * Since the segments are located successively, it is best for the sizes
 * to be in decreasing order; generally, data, stack, then u. will be
 * best.  Returns NULL on failure, address of u. on success.
 */
memaddr_t
rmalloc3(mp, d_size, s_size, u_size, a)
	struct map *mp;
	size_t d_size, s_size, u_size;
	memaddr_t a[3];
{
	register struct mapent *bp, *remap;
	struct mapent *madd[3];
	swblk_t first, rest;
	register int next;
	size_t sizes[3];
	int found, retry;
	register memaddr_t addr;

	sizes[0] = d_size;
	sizes[1] = s_size;
	sizes[2] = u_size;
	retry = 0;

again:
	/*
	 * note, this has to work for d_size and s_size of zero,
	 * since init() comes in that way.
	 */
	madd[0] = madd[1] = madd[2] = remap = NULL;

	return (a[2]);
}
