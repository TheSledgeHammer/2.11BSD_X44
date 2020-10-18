/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution is only permitted until one year after the first shipment
 * of 4.4BSD by the Regents.  Otherwise, redistribution and use in source and
 * binary forms are permitted provided that: (1) source distributions retain
 * this entire copyright notice and comment, and (2) distributions including
 * binaries display the following acknowledgement:  This product includes
 * software developed by the University of California, Berkeley and its
 * contributors'' in the documentation or other materials provided with the
 * distribution and in all advertising materials mentioning features or use
 * of this software.  Neither the name of the University nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)vm_drum.c	7.8 (Berkeley) 6/28/90
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/dmap.h>
#include <sys/map.h>
#include <sys/user.h>

#include <arch/i386/include/param.h>

/*
 * Expand the swap area for both the data and stack segments.
 * If space is not available for both, retract and return ENOMEM.
 */
int
vm_swpexpand(ds, ss, dmp, smp)
	segsz_t ds, ss;
	register struct dmap *dmp, *smp;
{
	register struct dmap *tmp;
	register int ts;
	segsz_t ods;

	if (dmp->dm_alloc > ctod(ds)) {
		tmp = dmp; ts = ds;
		dmp = smp; ds = ss;
		smp = tmp; ss = ts;
	}
	ods = dtoc(dmp->dm_size);
	if (vm_vsexpand(ds, dmp, 0) == 0)
		return (ENOMEM);
	if (vm_vsexpand(ss, smp, 0) == 0) {
		/* flush text cache and try again */
		if (xpurge() && vm_vsexpand(ss, smp, 0))
			return (0);
		(void) vm_vsexpand(ods, dmp, 1);
		return (ENOMEM);
	}
	return (0);
}

/*
 * Expand or contract the virtual swap segment mapped
 * by the argument diskmap so as to just allow the given size.
 *
 * FOR NOW CANT RELEASE UNLESS SHRINKING TO ZERO, SINCE PAGEOUTS MAY
 * BE IN PROGRESS... TYPICALLY NEVER SHRINK ANYWAYS, SO DOESNT MATTER MUCH
 */
int
vm_vsexpand(vssize, dmp, canshrink)
	register segsz_t 		vssize;
	register struct dmap 	*dmp;
	int 					canshrink;
{
	register long blk = dmmin;
	register int vsbase = 0;
	register swblk_t *ip = dmp->dm_map;
	segsz_t oldsize = dmp->dm_size;
	segsz_t oldalloc = dmp->dm_alloc;

	vssize = ctod(vssize);
	while (vsbase < oldalloc || vsbase < vssize) {
		if (ip - dmp->dm_map >= NDMAP)
			panic("vmdrum NDMAP");
		if (vsbase >= oldalloc) {
			*ip = rmalloc(swapmap, blk); 	/* replace with vm_segment_alloc ?? */
			if (*ip == 0) {
				dmp->dm_size = vsbase;
				if (vm_vsexpand(dtoc(oldsize), dmp, 1) == 0)
					panic("vsexpand");
				return (0);
			}
			dmp->dm_alloc += blk;
		} else if (vssize == 0 || (vsbase >= vssize && canshrink)) {
			rmfree(swapmap, blk, *ip); 		/* replace with vm_segment_free ?? */
			*ip = 0;
			dmp->dm_alloc -= blk;
		}
		vsbase += blk;
		if (blk < dmmax)
			blk *= 2;
		ip++;
	}
	dmp->dm_size = vssize;
	return (1);
}
