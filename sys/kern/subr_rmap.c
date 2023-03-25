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

/*
 * rmap revamp: Objective
 * Usage:
 * - Primarily for coremap & swapmap memory management
 * - Secondary as a generic.
 * Layout:
 * - Act like a client-server design or dual layer.
 * - e.g. the devel/vm_swap & vm_swapdrum.
 * 	- swapmap is allocated and expanded using swapextents
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

#include <vm/include/vm_text.h>

extern struct mapent _coremap[];

static struct mapent *rmcreate(struct map *, memaddr_t, size_t, int);

/*
 * Resource map handling routines.
 *
 * A resource map is an array of structures each of which describes a
 * segment of the address space of an available resource.  The segments
 * are described by their base address and length, and sorted in address
 * order.  Each resource map has a fixed maximum number of segments
 * allowed.  Resources are allocated by taking part or all of one of the
 * segments of the map.
 *
 * Returning of resources will require another segment if the returned
 * resources are not adjacent in the address space to an existing segment.
 * If the return of a segment would require a slot which is not available,
 * then one of the resource map segments is discarded after a warning is
 * printed.
 *
 * Returning of resources may also cause the map to collapse by coalescing
 * two existing segments and the returned space into a single segment.  In
 * this case the resource map is made smaller by copying together to fill
 * the resultant gap.
 *
 * N.B.: the current implementation uses a dense array and does not admit
 * the value ``0'' as a legal address or size, since that is used as a
 * delimiter.
 */

void
rminit(mp, addr, size, name, mtype, mapsize)
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
	mp->m_types = mtype;
	mp->m_limit = (struct mapent *)mp + mapsize;

	/* initially the first entry describes all free space */
	ep = rmcreate(mp, addr, size, 1);
}

/*
 * Allocates 'size' units from the given map, like rminit. But assumes the map is already initialized.
 */
void
rmallocate(mp, addr, size, mapsize)
	register struct map *mp;
	memaddr_t addr;
	size_t size;
	int mapsize;
{
	struct mapent *ep;

	/* mapsize had better be at least 2 */
	if (mapsize < 2 || addr <= 0 || size < 0) {
		panic("rmallocate %s", mp->m_name);
	}

	ep = rmcreate(mp, addr, size, mapsize);
	if (rmalloc(mp, size) != 0) {
		rmfree(mp, addr, size);
	}
}

/*
 * creates a mapent in map.
 */
static struct mapent *
rmcreate(mp, addr, size, mapsize)
	register struct map *mp;
	memaddr_t addr;
	size_t size;
	int mapsize;
{
	struct mapent *ep;

	ep = (struct mapent *)(mp + mapsize);
	ep->m_size = size;
	ep->m_addr = addr;

	/* the remaining slots are unused (indicated by m_addr == 0) */
	while (++ep < mp->m_limit) {
		ep->m_addr = 0;
	}
	return (ep);
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
	size_t      size;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	register struct mapent *bp;
	register memaddr_t addr;
	int retry;
	swblk_t first, rest;

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
	for (bp = mp->m_map; bp->m_size; ++bp)
		if (bp->m_size >= size) {
			/*
			 * Allocate from the map.  If we allocated the entire
			 * piece, move the rest of the map to the left.
			 */
			addr = bp->m_addr;
			bp->m_size -= size;
			if (bp->m_size) {
				bp->m_addr += size;
			} else for (ep = bp;; ++ep) {
				*ep = *++bp;
				if (!bp->m_size) {
					break;
				}
			}
			if(mp == coremap) {
				freemem -= size;
			}
			return (addr);
		} else if ((bp->m_size - first) < size) {
		    goto retry;
		}
		
	/* no entries big enough */
	if (!retry++) {
		if (mp == swapmap && nswdev > 1 && (first = dmmax - bp->m_addr % dmmax) < size) {
retry:
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
			//rmfree(mp, size, addr);
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
	if (!addr)
		panic("rmfree: addr = 0");
	if (mp == coremap) {
		if (runin) {
			runin = 0;
			wakeup((caddr_t)&runin);
		}
		freemem += size;
	}

	/*
	 * locate the piece of the map which starts after the
	 * returned space (or the end of the map).
	 */
	for (bp = mp->m_map; bp->m_size && bp->m_addr <= addr; ++bp);

	/* if there is a piece on the left abutting us, combine with it. */
	ep = bp - 1;
	if (bp != mp->m_map && ep->m_addr + ep->m_size >= addr) {
#ifdef DIAGNOSTIC
		/* any overlap is an internal error */
		if (ep->m_addr + ep->m_size > addr)
			panic("rmfree overlap #1");
#endif
		/* add into piece on the left by increasing its size. */
		ep->m_size += size;

		/*
		 * if the combined piece abuts the piece on the right now,
		 * compress it in also, by shifting the remaining pieces
		 * of the map over.
		 */
		if (bp->m_size && addr + size >= bp->m_addr) {
#ifdef DIAGNOSTIC
			if (addr + size > bp->m_addr)
				panic("rmfree overlap #2");
#endif
			ep->m_size += bp->m_size;
			do {
				*++ep = *++bp;
			} while (bp->m_size);
		}
		return;
	}

	/* if doesn't abut on the left, check for abutting on the right. */
	if (bp->m_size && addr + size >= bp->m_addr) {
#ifdef DIAGNOSTIC
		if (addr + size > bp->m_addr)
			panic("mfree overlap #3");
#endif
		bp->m_addr = addr;
		bp->m_size += size;
		return;
	}

	/* doesn't abut.  Make a new entry and check for map overflow. */
	for (start = bp; bp->m_size; ++bp);
	if (++bp > mp->m_limit)
		/*
		 * too many segments; if this happens, the correct fix
		 * is to make the map bigger; you can't afford to lose
		 * chunks of the map.  If you need to implement recovery,
		 * use the above "for" loop to find the smallest entry
		 * and toss it.
		 */
		printf("%s: overflow, lost %u clicks at 0%o\n",
		    mp->m_name, size, addr);
	else {
		for (ep = bp - 1; ep >= start; *bp-- = *ep--);
		start->m_addr = addr;
		start->m_size = size;
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
	register int next;
	struct mapent *madd[3];
	size_t sizes[3];
	int found, retry;
	register memaddr_t addr;
	swblk_t first, rest;

	sizes[0] = d_size; sizes[1] = s_size; sizes[2] = u_size;
	retry = 0;

again:
	/*
	 * note, this has to work for d_size and s_size of zero,
	 * since init() comes in that way.
	 */
	madd[0] = madd[1] = madd[2] = remap = NULL;
	for (found = 0, bp = mp->m_map; bp->m_size; ++bp) {
		for (next = 0; next < 3; ++next) {
			if (!madd[next] && sizes[next] <= bp->m_size) {
				madd[next] = bp;
				bp->m_size -= sizes[next];
				if (!bp->m_size && !remap) {
					remap = bp;
				}
				if (++found == 3) {
					goto resolve;
				}
			}
		}
	}

	/* couldn't get it all; restore the old sizes, try again */
	for (next = 0; next < 3; ++next) {
		if (madd[next]) {
			madd[next]->m_size += sizes[next];
		}
	}
	if (!retry++) {
	    if(mp == swapmap) {
	        for (next = 0; next < 3; ++next) {
	            if(nswdev > 1 && (first = dmmax - bp->m_addr % dmmax) < sizes[next]) {
	                if (bp->m_size - first < sizes[next]) {
					    continue;
				    }
	                addr = bp->m_addr + first;
    				rest = bp->m_size - first - sizes[next];
    				bp->m_size = first;
    				if (rest) {
					    printf("short of swap\n");
					    //rmfree(swapmap, rest, addr + sizes[next]);
					    vm_xumount(NODEV);
				    }
				    goto again;
	            }
	        }
	    }
	    if (mp == coremap) {
	        //rmfree(mp, sizes[2], addr); /* smallest to largest; */
			//rmfree(mp, sizes[1], addr); /* free up minimum space */
			//rmfree(mp, sizes[0], addr);
			vm_xuncore(sizes[2]);	/* smallest to largest; */
			vm_xuncore(sizes[1]);	/* free up minimum space */
			vm_xuncore(sizes[0]);
			goto again;
		}
	}

	return (0);

resolve:
	/* got it all, update the addresses. */
	for (next = 0; next < 3; ++next) {
		bp = madd[next];
		a[next] = bp->m_addr;
		bp->m_addr += sizes[next];
	}

	if (mp == coremap) {
		freemem -= d_size + s_size + u_size;
	}

	/* remove any entries of size 0; addr of 0 terminates */
	if (remap) {
		for (bp = remap + 1;; ++bp) {
			if (bp->m_size || !bp->m_addr) {
				*remap++ = *bp;
				if (!bp->m_addr) {
					break;
				}
			}
		}
	}
	return (a[2]);
}

/*
 * 2.11BSD kmem rmap info
 */
void
rmapinit(void)
{
	printf("phys mem  = %D\n", ctob((long)physmem));
	printf("avail mem in coremap = %D\n", ctob((long)_coremap[0].m_size));
	printf("user mem  = %D\n", ctob((long)MAXMEM));
}
