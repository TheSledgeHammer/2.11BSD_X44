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

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_text.h>
/* ARGUNUSED */
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
	if (vm_vsexpand(ds, dmp, 0) == 0) {
		return (ENOMEM);
	}
	if (vm_vsexpand(ss, smp, 0) == 0) {
		/* flush text cache and try again */
		if (vm_xpurge() && vm_vsexpand(ss, smp, 0)) {
			return (0);
		}
		(void) vm_vsexpand(ods, dmp, 1);
		return (ENOMEM);
	}
	return (0);
}
/* ARGUNUSED */
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
			*ip = rmalloc(swapmap, blk);
			if (*ip == 0) {
				dmp->dm_size = vsbase;
				if (vm_vsexpand(dtoc(oldsize), dmp, 1) == 0) {
					panic("vsexpand");
				}
				return (0);
			}
			dmp->dm_alloc += blk;
		} else if (vssize == 0 || (vsbase >= vssize && canshrink)) {
			rmfree(swapmap, blk, *ip);
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

/*
 * Allocate swap space for a text segment,
 * in chunks of at most dmtext pages.
 */
int
vm_vsxalloc(xp)
	struct vm_text *xp;
{
	register long blk;
	register swblk_t *dp;
	swblk_t vsbase;
	if (ctod(xp->psx_size) > NXDAD * dmtext) {
		return (0);
	}
	dp = xp->psx_daddr;
	for (vsbase = 0; vsbase < ctod(xp->psx_size); vsbase += dmtext) {
		blk = ctod(xp->psx_size) - vsbase;
		if (blk > dmtext) {
			blk = dmtext;
		}
		if ((*dp++ = rmalloc(swapmap, blk)) == 0) {
			vm_vsxfree(xp, dtoc(vsbase));
			return (0);
		}
	}
	if (xp->psx_flag & XPAGV) {
		xp->psx_ptdaddr = rmalloc(swapmap, (long)ctod(clrnd(ctopt(xp->psx_size))));
		if (xp->psx_ptdaddr == 0) {
			vm_vsxfree(xp, (long)xp->psx_size);
			return (0);
		}
	}
	return (1);
}

/*
 * Free the swap space of a text segment which
 * has been allocated ts pages.
 */
void
vm_vsxfree(xp, ts)
	struct vm_text *xp;
	long ts;
{
	register long blk;
	register swblk_t *dp;
	swblk_t 		vsbase;

	ts = ctod(ts);
	dp = xp->psx_daddr;
	for (vsbase = 0; vsbase < ts; vsbase += dmtext) {
		blk = ts - vsbase;
		if (blk > dmtext) {
			blk = dmtext;
		}
		rmfree(swapmap, blk, *dp);
		*dp++ = 0;
	}
	if ((xp->psx_flag & XPAGV) && xp->psx_ptdaddr) {
		rmfree(swapmap, (long)ctod(clrnd(ctopt(xp->psx_size))), xp->psx_ptdaddr);
		xp->psx_ptdaddr = 0;
	}
}

/* ARGUNUSED */
/*
 * Given a base/size pair in virtual swap area,
 * return a physical base/size pair which is the
 * (largest) initial, physically contiguous block.
 */
void
vm_vstodb(vsbase, vssize, dmp, dbp, rev)
	register int vsbase, vssize;
	struct dmap *dmp;
	register struct dblock *dbp;
	int rev;
{
	register int blk = dmmin;
	register swblk_t *ip = dmp->dm_map;

	if (vsbase < 0 || vssize < 0 || vsbase + vssize > dmp->dm_size) {
		panic("vstodb");
	}
	while (vsbase >= blk) {
		vsbase -= blk;
		if (blk < dmmax) {
			blk *= 2;
		}
		ip++;
	}
	if (*ip + blk > nswap) {
		panic("vstodb *ip");
	}
	dbp->db_size = imin(vssize, blk - vsbase);
	dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
}
/* ARGUNUSED */
/*
 * Convert a virtual page number
 * to its corresponding disk block number.
 * Used in pagein/pageout to initiate single page transfers.
 * An assumption here is that dmmin >= CLBYTES/DEV_BSIZE
 * i.e. a page cluster must be stored contiguously in the swap area.
 */
swblk_t
vm_vtod(p, v, dmap, smap)
	register struct proc *p;
	unsigned int v;
	struct dmap *dmap, *smap;
{
	struct dblock db;
	int tp;

	if (isatsv(p, v)) {
		tp = ctod(vtotp(p, v));
		return (p->p_taddr[tp / dmtext] + tp % dmtext);		/* XXX */
	}
	if (isassv(p, v)) {
		vm_vstodb(ctod(vtosp(p, v)), ctod(1), smap, &db, 1);
	} else {
		vm_vstodb(ctod(vtodp(p, v)), ctod(1), dmap, &db, 0);
	}
	return (db->db_base);
}
