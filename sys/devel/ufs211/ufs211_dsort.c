/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_dsort.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * generalized seek sort for disk
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dk.h>
#include <sys/user.h>

void
ufs211_disksort(dp, bp)
	register struct buf *dp, *bp;
{
	register struct buf *ap;
	struct buf *tp;

	ap = dp->b_actf;
	if (ap == NULL) {
		dp->b_actf = bp;
		dp->b_actb = bp;
		bp->av_forw = NULL;
		return;
	}
	tp = NULL;
	for (; ap != NULL; ap = ap->av_forw) {
		if ((bp->b_flags&B_READ) && (ap->b_flags&B_READ) == 0) {
			if (tp == NULL)
				tp = ap;
			break;
		}
		if ((bp->b_flags&B_READ) == 0 && (ap->b_flags&B_READ))
			continue;
		if (ap->b_cylin <= bp->b_cylin)
			if (tp == NULL || ap->b_cylin >= tp->b_cylin)
				tp = ap;
	}
	if (tp == NULL)
		tp = dp->b_actb;
	bp->av_forw = tp->av_forw;
	tp->av_forw = bp;
	if (tp == dp->b_actb)
		dp->b_actb = bp;
}


/*
 * Allocate iostat disk monitoring slots for a driver.  If slots already
 * allocated (*dkn >= 0) or not enough slots left to satisfy request simply
 * ignore it.
 */
void
dk_alloc(dkn, slots, name, wps)
	int *dkn;	/* pointer to number for iostat */
	int slots;	/* number of iostat slots requested */
	char *name;	/* name of device */
	long wps;	/* words per second transfer rate */
{
	int i;
	register char **np;
	register int *up;
	register long *wp;

	if (*dkn < 0 && dk_n + slots <= DK_NDRIVE) {
		/*
		 * Allocate and initialize the slots
		 */
		*dkn = dk_n;
		np = &dk_name[dk_n];
		up = &dk_unit[dk_n];
		wp = &dk_wps[dk_n];
		dk_n += slots;

		for (i = 0; i < slots; i++) {
			*np++ = name;
			*up++ = i;
			*wp++ = wps;
		}
	}
}
