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

/* dkdriver declaration */
#define DKDRIVER_DECL(name, strat, minphys, open, close, ioctl, dump, start, mklabel)	\
	struct dkdriver (name##_dkdrv) = { (#name), (strat), (minphys), (open), (close), (ioctl), (dump), (start), (mklabel) }

void
dkdriver_strategy(disk, bp)
	struct dkdevice *disk;
	struct buf *bp;
{
	register struct buf *dp;
	register struct dkdriver *driver;
	int s, unit;
	void rv;

	unit = dkunit(bp->b_dev);
	driver = disk[unit].dk_driver;

	if(!(disk->dk_flags & DKF_ALIVE)) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
	}
	s = partition_check(bp, disk);
	if (s < 0) {
		bp->b_flags |= B_ERROR;
	}
	if (s == 0) {
		biodone(bp);
	}
	bp->b_cylin = bp->b_blkno / (disk->dk_label->d_ntracks * disk->dk_label->d_nsectors);

	dp = bp[unit];
	s = splbio();
	disksort(dp, bp);
	if (dp->b_active == 0) {
		dkustart(unit);
		if (bp->b_active == 0) {
			dkstart();
		}
	}
	splx(s);
}

void
dkdriver_minphys(disk, bp)
	struct dkdevice *disk;
	struct buf *bp;
{
	register struct dkdriver *driver;
}

int
dkdriver_open(disk, dev, flags, mode, p)
	struct dkdevice *disk;
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register struct dkdriver *driver;
	int ret, part, mask, rv, unit;

	unit = dkunit(dev);
	driver = disk[unit].dk_driver;

	while (disk->dk_flags & (DKF_OPENING | DKF_CLOSING)) {
		sleep(disk, PRIBIO);
	}

	if ((disk->dk_flags & DKF_ALIVE) == 0) {
		//disk->dk_dev->d_type = 0;
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

	if(disk->dk_openmask == 0) {
		rv = (*driver->d_open)(dev, flags, mode, p);
		if (rv) {
			goto done;
		}
	}

done:
	lockmgr(&dksc->sc_iolock, LK_RELEASE, NULL);
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
	int unit, part, mask, rv;

	unit = dkunit(dev);
	driver = disk[unit].dk_driver;

	if (mode == S_IFCHR) {
		disk->dk_copenmask |= mask;
	} else if (mode == S_IFBLK) {
		disk->dk_bopenmask |= mask;
	} else {
		return (EINVAL);
	}
	switch(mode) {
	case S_IFCHR:
		disk->dk_copenmask |= mask;
		break;
	case S_IFBLK:
		disk->dk_bopenmask |= mask;
		break;
	}
	disk->dk_openmask = disk->dk_bopenmask | disk->dk_copenmask;

	if (disk->dk_openmask == 0) {
		disk->dk_flags |= DKF_CLOSING;
	}
	disk->dk_flags &= ~(DKF_CLOSING | DKF_WANTED | DKF_ALIVE | DKF_ONLINE);

	if((disk->dk_flags & DKF_CLOSING) == 0) {
		rv = (*driver->d_close)(dev, flags, mode, p);
		if (rv) {
			goto done;
		}
	}

done:
	return (rv);
}

int
dkdriver_ioctl(disk, dev, cmd, data, flag, p)
	struct dkdevice *disk;
	dev_t dev;
	u_long cmd;
	void *data;
	int flag;
	struct proc *p;
{
	register struct dkdriver *driver;
	int unit, error;

	unit = dkunit(dev);
	driver = disk[unit].dk_driver;

	error = disk_ioctl(disk, dev, cmd, data, flag, p);
	if (error != 0) {
		return (error);
	}

	error = ioctldisklabel(disk, driver->d_strategy, dev, cmd, data, flag);
	if (error != 0) {
		return (error);
	}

	return (0);
}

void
dkdriver_start(disk, bp, addr)
	struct dkdevice *disk;
	struct buf * bp;
	daddr_t addr;
{
	register struct dkdriver *driver;
	int unit, error;

	unit = dkunit(bp->b_dev);
	driver = disk[unit].dk_driver;

	if (disk->dk_busy < 2)
		disk->dk_busy++;
	/*
	if (disk->dk_busy > 1)
		goto done;
	 */
	while (disk->dk_busy > 0) {

		while (bp != NULL) {
			disk_busy(&disk);
			// simple_unlock(&dksc->sc_iolock);
			error = driver->d_start(bp, addr);
			simple_lock(&dksc->sc_iolock);
			if (error == EAGAIN || error == ENOMEM) {
				disk_unbusy(&disk, 0);
				//disk_wait(&dksc->sc_dkdev);
				break;
			}

			if (error != 0) {
				bp->b_error = error;
				bp->b_resid = bp->b_bcount;
				dkdriver_done1(disk, bp);
			}
		}
		disk->dk_busy--;
	}
}

int
dkdriver_mklabel(disk)
	struct dkdevice *disk;
{
	register struct dkdriver *driver;
	struct	disklabel *lp = disk->dk_label;

	lp->d_partitions[RAW_PART].p_fstype = FS_BSDFFS;
	strncpy(lp->d_packname, "default label", sizeof(lp->d_packname));
	lp->d_checksum = dkcksum(lp);
}

static void
dkdriver_done1(disk, bp)
	struct dkdevice *disk;
	struct buf *bp;
	//boolean_t lock;
{
	if (bp->b_error != 0) {
		struct cfdriver *cd = disk->dk_dev->dv_cfdata->cf_driver;
		diskerr(bp, cd->cd_name, "error", LOG_PRINTF, 0, disk->dk_label);
		printf("\n");
	}
	disk_unbusy(disk, bp->b_bcount - bp->b_resid, (bp->b_flags & B_READ));
	biodone(bp);
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
