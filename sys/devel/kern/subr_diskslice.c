/*-
 * Copyright (c) 1994 Bruce D. Evans.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/diskslice.h>

void
set_adaptor(boot)
	u_long boot;
{
	int adaptor = (boot >> B_ADAPTORSHIFT) & B_ADAPTORMASK;
}

void
set_controller(boot)
	u_long boot;
{
	int controller = (boot >> B_CONTROLLERSHIFT) & B_CONTROLLERMASK;
}

void
set_slice(boot)
	u_long boot;
{
	int slice = (boot >> B_SLICESHIFT) & B_SLICEMASK;
}

void
set_partition(boot)
	u_long boot;
{
	int part = (boot >> B_PARTITIONSHIFT) & B_PARTITIONMASK;
}

void
set_unit(boot)
	u_long boot;
{
	int unit = (bootdev >> B_UNITSHIFT) & B_UNITMASK;
}


dev_t
makediskslice(dev, unit, slice, part)
	dev_t dev;
	int unit, slice, part;
{
	return (makedev(major(dev), dkmakeminor(unit, slice, part)));
}

char *
readdiskslice(dev, strat, sp)
	dev_t dev;
	int (*strat)();
	register struct diskslice *sp;
{
	char *msg = NULL;

	return (msg);
}

int
setdiskslice(osp, nsp, openmask)
	register struct diskslice *osp, *nsp;
	u_long openmask;
{
	register i;
	register struct partition *opp, *npp;
	struct disklabel *lp;

	lp = nsp->ds_label;
	if (lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC || dkcksum(lp) || dscksum(nsp) != 0) {
		return (EINVAL);
	}
	while ((i = ffs((long)openmask)) != 0) {
		i--;
		openmask &= ~(1 << i);
		if (nsp->ds_nslices <= i) {
			return (EBUSY);
		}
		opp = &osp->ds_slices[i];
		npp = &nsp->ds_slices[i];
		if (npp->p_offset != opp->p_offset || npp->p_size < opp->p_size) {
			return (EBUSY);
		}
		/*
		 * Copy internally-set partition information
		 * if new label doesn't include it.		XXX
		 */
		if (npp->p_fstype == FS_UNUSED && opp->p_fstype != FS_UNUSED) {
			npp->p_fstype = opp->p_fstype;
			npp->p_fsize = opp->p_fsize;
			npp->p_frag = opp->p_frag;
			npp->p_cpg = opp->p_cpg;
		}
	}
 	nsp->ds_checksum = 0;
 	nsp->ds_checksum = dscksum(nsp);
	*osp = *nsp;

	return (0);
}

int
writediskslice(dev, strat, sp)
	dev_t dev;
	int (*strat)();
	register struct diskslice *sp;
{
	struct buf *bp;
	struct diskslice *slp;
	struct disklabel *dlp;
	int slice;
	int error = 0;

	slice = dkslice(dev);
	if(sp->ds_slices[slice].dss_offset != 0) {
		if(sp->ds_slices[0].dss_offset != 0) {
			return (EXDEV); /* not quite right */
		}
		slice = 0;
	}

	bp = geteblk((int) sp->ds_secsize);
	bp->b_dev = makedev(major(dev), dkmakeminor(dkunit(dev), slice, dkpart(dev)));
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = sp->ds_secsize;
	bp->b_flags = B_READ;
	(*strat)(bp);
	if (error == biowait(bp)) {
		goto done;
	}
	for (slp = (struct diskslice*) bp->b_data; slp <= (struct diskslice*) ((char*) bp->b_data + sp->ds_secsize - sizeof(*slp)); slp = (struct diskslice*) ((char*) slp + sizeof(long))) {
		dlp = slp->ds_label;
		if(dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC && dkcksum(dlp) && dscksum(slp) == 0) {
			*slp = *sp;
			bp->b_flags = B_WRITE;
			(*strat)(bp);
			error = biowait(bp);
			goto done;
		}
	}
done:
	brelse(bp);
	return (error);
}

int
dscksum(sp)
	struct diskslice *sp;
{
	register u_short *start, *end;
	register u_short sum = 0;

	start = (u_short *)sp;
	end = (u_short *)&sp->ds_slices[sp->ds_nslices];
	while (start < end) {
		sum ^= *start++;
	}
	return (sum);
}
