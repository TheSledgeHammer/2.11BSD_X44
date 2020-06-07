/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)map.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * Resource Allocation Maps.
 *
 * Associated routines manage allocation of an address space using
 * an array of segment descriptors.
 *
 * Malloc and mfree allocate and free the resource described
 * by the resource map.  If the resource map becomes too fragmented
 * to be described in the available space, then some of the resource
 * is discarded.  This may lead to critical shortages,
 * but is better than not checking (as the previous versions of
 * these routines did) or giving up and calling panic().
 *
 * N.B.: The address 0 in the resource address space is not available
 * as it is used internally by the resource map routines.
 */

struct map {
	struct mapent	*m_map;		/* start of the map */
	struct mapent	*m_limit;	/* address of last slot in map */
	char   			*m_name;	/* name of resource */
/* we use m_name when the map overflows, in warning messages */
};

struct mapent {
	size_t 	m_size;				/* size of this segment of the map */
	memaddr	m_addr;				/* resource-space addr of start of segment */
};

#define RMALLOC(mp, cast, size) {							\
	(mp) = (cast)rmalloc(mp, size);							\
};

#define RMALLOC3(mp, cast, d_size, s_size, u_size, a) {		\
	(mp) = (cast)rmalloc3(mp, d_size, s_size, u_size, a);	\
};

#define RMFREE(mp, size, addr) { 							\
	rmfree(mp, size, (caddr_t)addr); 						\
};

#ifdef _KERNEL
#define	ARGMAPSIZE	16
struct map *kmemmap, *mbmap;
struct map *coremap[1];																	/* space for core allocation */
struct map *swapmap[1];																	/* space for swap allocation */
int	nswapmap;
#else
struct map *coremap[1];																	/* space for core allocation */
struct map *swapmap[1];																	/* space for swap allocation */
#endif
#ifdef _KERNEL
long 	rmalloc (struct map *mp, long size); 											/* Allocate units from the given map. */
void 	rmfree (struct map *mp, long size, long addr); 									/* Free the previously allocated units at addr into the specified map.*/
long 	rmalloc3 (struct map *mp, long d_size, long s_size, long u_size, long a[3]);	/* Allocate resources for the three segments of a process.*/
void	rminit (struct map *mp, long size, long addr, char *name, int mapsize);			/* Initialized resource malloc */
#endif
