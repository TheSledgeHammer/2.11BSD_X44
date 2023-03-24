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

#include <vm/include/vm.h>
#include <vm/include/vm_text.h>

/*
 * Allocate swap space for a text segment,
 * in chunks of at most dmtext pages.
 */
int
vm_vsxalloc(xp)
	vm_text_t xp;
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
	vm_text_t xp;
	long ts;
{
	register long blk;
	register swblk_t *dp;
	swblk_t vsbase;

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
