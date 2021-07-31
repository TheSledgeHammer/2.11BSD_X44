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

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/dk.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/syslog.h>

struct buf			dktab;
struct buf			dkutab[];
struct dkdevice 	*dk;

struct dkdriver dkdisks = {
		.d_open = dkdriver_open,
		.d_close = dkdriver_close
};

int
dkdriver_open(disk, dev, flags, mode, p)
	struct dkdevice *disk;
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register struct dkdriver *driver;
	int ret, part, mask, rv;

	driver = disk->dk_driver;

	while (disk->dk_flags & (DKF_OPENING | DKF_CLOSING)) {
		sleep(disk, PRIBIO);
	}

	if ((disk->dk_flags & DKF_ALIVE) == 0) {
		disk->dk_dev->d_type[unit] = 0;
		disk->dk_flags |= DKF_ALIVE;
	}
	if (disk->dk_openmask == 0) {
		disk->dk_flags |= DKF_OPENING;
		dkgetinfo(disk, dev);
		disk->dk_flags &= ~DKF_OPENING;
		wakeup(disk);
	}
	disk->dk_label = LABELDESC;
	part = disk->dk_label->d_npartitions;
	if (dkpart(dev) >= part) {
		return (ENXIO);
	}
	mask = 1 << dkpart(dev);
	dkoverlapchk(disk->dk_openmask, dev, disk->dk_label, disk->dk_name);

	switch(mode) {
	case S_IFCHR:
		disk->dk_copenmask |= mask;
		break;
	case S_IFBLK:
		disk->dk_bopenmask |= mask;
		break;
	}

	disk->dk_openmask = disk->dk_bopenmask | disk->dk_copenmask;

	rv = (*driver->d_open)(dev, flags, mode, p);
	if(rv != 0) {
		return (-1);
	}
	return (rv);
}

int
dkdriver_close(disk, dev, flags, mode, p)
	struct dkdevice *disk;
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register struct dkdriver *driver;
	int part, mask, rv;

	driver = disk->dk_driver;

	if (mode == S_IFCHR) {
		disk->dk_copenmask |= mask;
	} else if (mode == S_IFBLK) {
		disk->dk_bopenmask |= mask;
	} else {
		return (EINVAL);
	}
	disk->dk_openmask = disk->dk_bopenmask | disk->dk_copenmask;

	if (disk->dk_openmask == 0) {
		disk->dk_flags |= DKF_CLOSING;
	}
	disk->dk_flags &= ~(DKF_CLOSING | DKF_WANTED | DKF_ALIVE | DKF_ONLINE);

	rv = (*driver->d_close)(dev, flags, mode, p);
	if (rv != 0) {
		return (-1);
	}
	return (rv);
}

void
dkdriver_strategy(bp)
	struct buf *bp;
{
	register struct buf *dp;
	register struct dkdevice *disk;
	register struct dkdriver *driver;
	int s, unit;
	void rv;

	unit = dkunit(bp->b_dev);
	disk = &dk[unit];
	driver = disk->dk_driver;

	if(!(disk->dk_flags & DKF_ALIVE)) {
		bp->b_error = ENXIO;
		goto bad;
	}
	s = partition_check(bp, disk);
	if (s < 0) {
		goto bad;
	}
	if (s == 0) {
		goto done;
	}
	bp->b_cylin = bp->b_blkno / (disk->dk_label->d_ntracks * disk->dk_label->d_nsectors);

	dp = &dkutab[unit];
	s = splbio();
	disksort(dp, bp);
	if (dp->b_active == 0) {
		dkustart(unit);
		if (dktab.b_active == 0) {
			dkstart();
		}
	}
	splx(s);
	rv = (*driver->d_strategy)(bp);

	return (rv);

bad:
	bp->b_flags |= B_ERROR;

done:
	biodone(bp);
}

dkdriver_start(unit)
{

}

/*
 * Allocate iostat disk monitoring slots for a driver.  If slots already
 * allocated (*dkn >= 0) or not enough slots left to satisfy request simply
 * ignore it.
 */
void
dk_alloc(dkn, slots, name, wps)
	int  *dkn;	/* pointer to number for iostat */
	int  slots;	/* number of iostat slots requested */
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

void
dkdfltlbl(disk, lp, dev)
	struct dkdevice *disk;
	register struct disklabel *lp;
	dev_t	dev;
{
	register struct partition *pi;
	int error;

	if (disk_find(disk->dk_name) == NULL) {
		return;
	}

	pi = &lp->d_partitions[0];
	bzero(lp, sizeof(*lp));
	error = setdisklabel(disk->dk_label, lp, disk->dk_openmask);
	if(error != 0) {
		return;
	}
	bcopy(pi, disk->dk_parts, sizeof(lp->d_partitions));
}

void
dkgetinfo(disk, dev)
	register struct dkdevice *disk;
	dev_t   dev;
{
	struct disklabel locallabel;
	char   *msg;
	register struct disklabel *lp = &locallabel;

  /*
   * NOTE: partition 0 ('a') is used to read the label.  Therefore 'a' must
   * start at the beginning of the disk!  If there is no label or the label
   * is corrupted then 'a' will span the entire disk
  */
  	dkdfltlbl(disk, lp, dev);
  	msg = readdisklabel((dev & ~7) | 0, dkdriver_strategy, lp);    /* 'a' */
  	if (msg != 0) {
  		log(LOG_NOTICE, "dk%da is entire disk: %s\n", dkunit(dev), msg);
  		dkdfltlbl(disk, lp, dev);
  	}
  	disk->dk_label = LABELDESC;
  	bcopy(lp, (struct disklabel *)disk->dk_label, sizeof (struct disklabel));
  	bcopy(lp->d_partitions, disk->dk_parts, sizeof (lp->d_partitions));
  	return;
}
