/*	$NetBSD: subr_disk.c,v 1.60 2004/03/09 12:23:07 yamt Exp $	*/
/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	8.5.5 (2.11BSD GTE) 1998/4/3
 */
/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ufs_disksubr.c	8.5 (Berkeley) 1/21/94
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/disklabel.h>
#include <sys/diskslice.h>
#include <sys/disk.h>
#include <sys/devsw.h>
#include <sys/user.h>

#include <machine/param.h>

/*
 * Initialize the disklist.  Called by main() before autoconfiguration.
 */
struct disklist_head 	disklist = TAILQ_HEAD_INITIALIZER(disklist);	/* TAILQ_HEAD */
int						disk_count = 0;	/* number of drives in global disklist */

/*
 * Searches the disklist for the disk corresponding to the
 * name provided.
 */
struct dkdevice *
disk_find(name)
	char *name;
{
	struct dkdevice *diskp;

	if ((name == NULL) || (disk_count <= 0))
		return (NULL);

	for (diskp = TAILQ_FIRST(&disklist); diskp != NULL; diskp = TAILQ_NEXT(diskp, dk_link)) {
		if (strcmp(diskp->dk_name, name) == 0) {
			return (diskp);
		}
	}
	return (NULL);
}

dev_t
disk_isvalid(bdev, cdev)
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
{
	dev_t dev1, dev2;

	if ((bdev != NULL) && (cdev != NULL)) {
		dev1 = bdevsw_lookup_major(bdev);
		dev2 = cdevsw_lookup_major(cdev);
		if (dev1 == dev2) {
			return (dev1);
		}
	} else {
		if ((bdev != NULL) && (cdev == NULL)) {
			dev1 = bdevsw_lookup_major(bdev);
			dev2 = NODEVMAJOR;
			return (dev1);
		}
		if ((cdev != NULL) && (bdev == NULL)) {
			dev1 = NODEVMAJOR;
			dev2 = cdevsw_lookup_major(cdev);
			return (dev2);
		}
	}
	return (NODEVMAJOR);
}

/*
 * Attach a disk.
 */
void
disk_attach(diskp, bdev, cdev)
	struct dkdevice *diskp;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
{
	int s, ret;
	dev_t pdev, dev;

	diskp->dk_label = (struct disklabel *)malloc(sizeof(struct disklabel *), M_DEVBUF, M_NOWAIT);
	diskp->dk_cpulabel = (struct cpu_disklabel *)malloc(sizeof(struct cpu_disklabel *), M_DEVBUF, M_NOWAIT);
	diskp->dk_slices = dsmakeslicestruct(BASE_SLICE, diskp->dk_label);

	if (diskp->dk_label == NULL) {
		panic("disk_attach: can't allocate storage for disklabel");
	} else {
		bzero(diskp->dk_label, sizeof(struct disklabel));
	}
	if (diskp->dk_cpulabel == NULL) {
		panic("disk_attach: can't allocate storage for cpu_disklabel");
	} else {
		bzero(diskp->dk_cpulabel, sizeof(struct cpu_disklabel));
	}
	if (diskp->dk_slices == NULL) {
		panic("disk_attach: can't allocate storage for diskslices");
	} else {
		bzero(diskp->dk_slices, sizeof(struct diskslices));
	}
#ifdef DISK_SLICES
	ret = 0;
#else /* !DISK_SLICES */
	dev = disk_isvalid(bdev, cdev);
	if (dev != NODEVMAJOR) {
		pdev = dkmakedev(major(dev), dkunit(dev), RAW_PART);
		ret = mbrinit(diskp, pdev, diskp->dk_label, &diskp->dk_slices);
	} else {
		ret = -1;
	}
#endif
	if (ret != 0) {
		panic("disk_attach: can't initialize diskslices");
	}

	/*
	 * Set the attached timestamp.
	 */
	s = splclock();
	diskp->dk_attachtime = mono_time;
	splx(s);

	/*
	 * Link into the disklist.
	 */
	TAILQ_INSERT_TAIL(&disklist, diskp, dk_link);
	++disk_count;
}

/*
 * Detach a disk.
 */
void
disk_detach(diskp)
	struct dkdevice *diskp;
{
	/*
	 * Remove from the disklist.
	 */
	if (--disk_count < 0) {
		panic("disk_detach: disk_count < 0");
	}
	TAILQ_REMOVE(&disklist, diskp, dk_link);

	/*
	 * Free the space used by the disklabel structures.
	 */
	free(diskp->dk_label, M_DEVBUF);
	free(diskp->dk_cpulabel, M_DEVBUF);
	dsgone(&diskp->dk_slices);
}

/*
 * Increment a disk's busy counter.  If the counter is going from
 * 0 to 1, set the timestamp.
 */
void
disk_busy(diskp)
	struct dkdevice *diskp;
{
	int s;

	/*
	 * XXX We'd like to use something as accurate as microtime(),
	 * but that doesn't depend on the system TOD clock.
	 */
	if (diskp->dk_busy++ == 0) {
		s = splclock();
		diskp->dk_timestamp = mono_time;
		splx(s);
	}
}

/*
 * Decrement a disk's busy counter, increment the byte count, total busy
 * time, and reset the timestamp.
 */
void
disk_unbusy(diskp, bcount)
	struct dkdevice *diskp;
	long bcount;
{
	int s;
	struct timeval dv_time, diff_time;

	if (diskp->dk_busy-- == 0) {
		printf("%s: dk_busy < 0\n", diskp->dk_name);
		panic("disk_unbusy");
	}

	s = splclock();
	dv_time = mono_time;
	splx(s);

	timersub(&dv_time, &diskp->dk_timestamp, &diff_time);
	timeradd(&diskp->dk_time, &diff_time, &diskp->dk_time);

	diskp->dk_timestamp = dv_time;
	if (bcount > 0) {
		diskp->dk_bytes += bcount;
		diskp->dk_bps++;
	}
}

/*
 * Reset the metrics counters on the given disk.  Note that we cannot
 * reset the busy counter, as it may case a panic in disk_unbusy().
 * We also must avoid playing with the timestamp information, as it
 * may skew any pending transfer results.
 */
void
disk_resetstat(diskp)
	struct dkdevice *diskp;
{
	int s = splbio(), t;

	diskp->dk_bps = 0;
	diskp->dk_bytes = 0;

	t = splclock();
	diskp->dk_attachtime = mono_time;
	splx(t);

	timerclear(&diskp->dk_time);

	splx(s);
}

/*
 * Seek sort for disks.  We depend on the driver which calls us using b_resid
 * as the current cylinder number.
 *
 * The argument ap structure holds a b_actf activity chain pointer on which we
 * keep two queues, sorted in ascending cylinder order.  The first queue holds
 * those requests which are positioned after the current cylinder (in the first
 * request); the second holds requests which came in after their cylinder number
 * was passed.  Thus we implement a one way scan, retracting after reaching the
 * end of the drive to the first request on the second queue, at which time it
 * becomes the first queue.
 *
 * A one-way scan is natural because of the way UNIX read-ahead blocks are
 * allocated.
 */

/*
 * For portability with historic industry practice, the
 * cylinder number has to be maintained in the `b_resid'
 * field.
 */
/*
 * bufq: the disk buffer queue.
 * buf: the buffer to sort.
 * flags: additional flags to set. BUFQ_DISKSORT is set by default.
 */
void
disksort(bufq, ap, flags)
	struct bufq_state   *bufq;
	struct buf          *ap;
	int 				flags;
{
	if (bufq == NULL) {
        bufq_alloc(bufq, (BUFQ_DISKSORT | flags));
    }
    if((ap = BUFQ_PEEK(bufq)) != NULL) {
        printf("buffer already allocated");
    } else {
        BUFQ_PUT(bufq, ap);
    }
}

/*
 * Disk error is the preface to plaintive error messages
 * about failing disk transfers.  It prints messages of the form

hp0g: hard error reading fsbn 12345 of 12344-12347 (hp0 bn %d cn %d tn %d sn %d)

 * if the offset of the error in the transfer and a disk label
 * are both available.  blkdone should be -1 if the position of the error
 * is unknown; the disklabel pointer may be null from drivers that have not
 * been converted to use them.  The message is printed with printf
 * if pri is LOG_PRINTF, otherwise it uses log at the specified priority.
 * The message should be completed (with at least a newline) with printf
 * or addlog, respectively.  There is no trailing space.
 */
void
diskerr(bp, dname, what, pri, blkdone, lp)
	register struct buf *bp;
	char *dname;
	const char *what;
	int pri, blkdone;
	register struct disklabel *lp;
{
	int unit;
	int part;
	register void (*pr)(const char *, ...);
	char partname;
	int sn;

	unit = dkunit(bp->b_dev);
	part = dkpart(bp->b_dev);
	partname = 'a' + part;

	if (pri != LOG_PRINTF) {
		log(pri, "");
		pr = addlog;
	} else {
		pr = printf;
	}
	(*pr)("%s%d%c: %s %sing fsbn ", dname, unit, partname, what, bp->b_flags & B_READ ? "read" : "writ");
	sn = bp->b_blkno;
	if (bp->b_bcount <= DEV_BSIZE) {
		(*pr)("%d", sn);
	} else {
		if (blkdone >= 0) {
			sn += blkdone;
			(*pr)("%d of ", sn);
		}
		(*pr)("%d-%d", bp->b_blkno,
				bp->b_blkno + (bp->b_bcount - 1) / DEV_BSIZE);
	}
	if (lp && (blkdone >= 0 || bp->b_bcount <= lp->d_secsize)) {
		sn += lp->d_partitions[part].p_offset;
		(*pr)(" (%s%d bn %d; cn %d", dname, unit, sn, sn / lp->d_secpercyl);
		sn %= lp->d_secpercyl;
		(*pr)(" tn %d sn %d)", sn / lp->d_nsectors, sn % lp->d_nsectors);
	}
}

/*
 * Bounds checking against the media size, used for the raw partition.
 * The sector size passed in should currently always be DEV_BSIZE,
 * and the media size the size of the device in DEV_BSIZE sectors.
 */
int
bounds_check_with_mediasize(bp, secsize, mediasize)
	struct buf *bp;
	int secsize;
	u_int64_t mediasize;
{
	int sz;

	sz = howmany(bp->b_bcount, secsize);

	if (bp->b_blkno + sz > mediasize) {
		sz = mediasize - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		if (sz < 0) {
			/* If past end of disk, return EINVAL. */
			bp->b_error = EINVAL;
			goto bad;
		}
		/* Otherwise, truncate request. */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	return 1;

bad:
	bp->b_flags |= B_ERROR;
done:
	return 0;
}

/*
 * Determine the size of the transfer, and make sure it is
 * within the boundaries of the partition. Adjust transfer
 * if needed, and signal errors or early completion.
 */
int
bounds_check_with_label(dk, bp, wlabel)
	struct dkdevice *dk;
	struct buf *bp;
	int wlabel;
{
	struct disklabel *lp = dk->dk_label;
	struct partition *p = lp->d_partitions + dkpart(bp->b_dev);
	uint64_t p_size, p_offset, labelsector;
	int64_t sz;

	if (bp->b_blkno < 0) {
		/* Reject negative offsets immediately. */
		bp->b_error = EINVAL;
		return -1;
	}

	/* Protect against division by zero. XXX: Should never happen?!?! */
	if ((lp->d_secsize / DEV_BSIZE) == 0 || lp->d_secpercyl == 0) {
		bp->b_error = EINVAL;
		return -1;
	}

	p_size = (uint64_t) p->p_size << dk->dk_blkshift;
	p_offset = (uint64_t) p->p_offset << dk->dk_blkshift;
#if RAW_PART == 3
	labelsector = lp->d_partitions[2].p_offset;
#else
	labelsector = lp->d_partitions[RAW_PART].p_offset;
#endif
	labelsector = (labelsector + dk->dk_labelsector) << dk->dk_blkshift;

	sz = howmany((int64_t) bp->b_bcount, DEV_BSIZE);

	/*
	 * bp->b_bcount is a 32-bit value, and we rejected a negative
	 * bp->b_blkno already, so "bp->b_blkno + sz" cannot overflow.
	 */

	if (bp->b_blkno + sz > p_size) {
		sz = p_size - bp->b_blkno;
		if (sz == 0) {
			/* If exactly at end of disk, return EOF. */
			bp->b_resid = bp->b_bcount;
			return 0;
		}
		if (sz < 0) {
			/* If past end of disk, return EINVAL. */
			bp->b_error = EINVAL;
			return -1;
		}
		/* Otherwise, truncate request. */
		bp->b_bcount = sz << DEV_BSHIFT;
	}

	/* Overwriting disk label? */
	if (bp->b_blkno + p_offset <= labelsector
			&& bp->b_blkno + p_offset + sz > labelsector
			&& (bp->b_flags & B_READ) == 0 && !wlabel) {
		bp->b_error = EROFS;
		return -1;
	}

	/* calculate cylinder for disksort to order transfers with */
	bp->b_cylin = (bp->b_blkno + p->p_offset) / (lp->d_secsize / DEV_BSIZE)
			/ lp->d_secpercyl;
	return 1;
}

/* disk functions for easy access */
struct dkdriver *
disk_driver(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_driver);
}

struct disklabel *
disk_label(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_label);
}

struct diskslices *
disk_slices(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_slices);
}

struct partition *
disk_partition(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_parts);
}

struct device *
disk_device(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (&disk[dkunit(dev)].dk_dev);
}

struct dkdevice *
disk_find_by_dev(dev)
   dev_t dev;
{
    struct dkdevice *diskp, *disk;
    
    TAILQ_FOREACH(diskp, &disklist, dk_link) {
    	disk = &diskp[dkunit(dev)];
        if (disk == diskp) {
            return (diskp);
        }
    }
    return (NULL);
}

struct dkdevice *
disk_find_by_slice(slicep)
   struct diskslices *slicep;
{
    struct dkdevice *diskp;

    TAILQ_FOREACH(diskp, &disklist, dk_link) {
        if(diskp->dk_slices == slicep) {
            return (diskp);
        }
    }
    return (NULL);
}

char *
devtoname(dev)
    dev_t dev;
{
	 struct dkdevice *diskp;
	 struct device   *dvp;

	 diskp = disk_find_by_dev(dev);
	 if (diskp != NULL) {
		 dvp = disk_device(diskp, dev);
		 if (dvp != NULL) {
			 return (dvp->dv_xname);
		 }
	 }
	 return (NULL);
}

int
sysctl_disknames(where, sizep)
	char *where;
	size_t *sizep;
{
	struct dkdevice *diskp;
	char buf[DK_DISKNAMELEN + 1];
	size_t needed, left, slen;
	int error, first;

	first = 1;
	error = 0;
	needed = 0;
	left = *sizep;

	TAILQ_FOREACH(diskp, &disklist, dk_link) {
		if (where == NULL) {
			needed += strlen(diskp->dk_name) + 1;
		} else {
			bzero(buf, sizeof(buf));
			if (first) {
				strncpy(buf, diskp->dk_name, sizeof(buf));
				first = 0;
			} else {
				buf[0] = ' ';
				strncpy(buf + 1, diskp->dk_name, sizeof(buf) - 1);
			}
			buf[DK_DISKNAMELEN] = '\0';
			slen = strlen(buf);
			if (left < slen + 1) {
				break;
			}
			/* +1 to copy out the trailing NUL byte */
			error = copyout(buf, where, slen + 1);
			if (error) {
				break;
			}
			where += slen;
			needed += slen;
			left -= slen;
		}
	}

	*sizep = needed;
	return (error);
}

int
sysctl_diskstats(name, namelen, where, sizep)
	int *name;
	u_int namelen;
	char *where;
	size_t *sizep;
{
	struct disk_stats sdisk;
	struct dkdevice *diskp;
	size_t left, slen;
	int error;

	if (where == NULL) {
		*sizep = disk_count * sizeof(struct disk_stats);
		return (0);
	}

	if (namelen == 0) {
		slen = sizeof(sdisk);
	} else {
		slen = name[0];
	}

	error = 0;
	left = *sizep;
	memset(&sdisk, 0, sizeof(sdisk));
	*sizep = 0;

	TAILQ_FOREACH(diskp, &disklist, dk_link) {
		if (left < sizeof(struct disk_stats)) {
			break;
		}
		strncpy(sdisk.ds_name, diskp->dk_name, sizeof(sdisk.ds_name));
		sdisk.ds_bps = diskp->dk_bps;
		sdisk.ds_seek = diskp->dk_seek;
		sdisk.ds_bytes = diskp->dk_bytes;
		sdisk.ds_attachtime = diskp->dk_attachtime;
		sdisk.ds_timestamp = diskp->dk_timestamp;
		sdisk.ds_time = diskp->dk_time;
		sdisk.ds_busy = diskp->dk_busy;

		error = copyout(&sdisk, where, min(slen, sizeof(sdisk)));
		if (error) {
			break;
		}
		where += slen;
		*sizep += slen;
		left -= slen;
	}
	return (error);
}

/* dkdriver */
static dev_t disk_pdev(dev_t);
static void  disk_bufio(struct dkdevice *, struct buf *);

void
dkdriver_strategy(diskp, bp)
	struct dkdevice *diskp;
	struct buf *bp;
{
	disk_bufio(diskp, bp);
	(*diskp->dk_driver->d_strategy)(bp);
}

void
dkdriver_minphys(diskp, bp)
	struct dkdevice *diskp;
	struct buf *bp;
{
	disk_bufio(diskp, bp);
	(*diskp->dk_driver->d_minphys)(bp);
}

int
dkdriver_open(diskp, dev, flag, fmt, p)
	struct dkdevice *diskp;
	dev_t dev;
	int flag, fmt;
	struct proc	*p;
{
	dev_t pdev;
	int error;

	pdev = disk_pdev(dev);
	if (!diskp) {
		return (ENXIO);
	}
#ifdef DISK_SLICES
	error = dsopen(diskp, pdev, fmt, flag, diskp->dk_label);
	if (error != 0) {
		return (error);
	}
	if (!dsisopen(diskp->dk_slices)) {
		goto out;
	}
out:
#endif
	error = (*diskp->dk_driver->d_open)(pdev, flag, fmt, p);
	return (error);
}

int
dkdriver_close(diskp, dev, flag, fmt, p)
	struct dkdevice *diskp;
	dev_t dev;
	int flag, fmt;
	struct proc	*p;
{
	dev_t pdev;
	int error;

	pdev = disk_pdev(dev);
	if (!diskp) {
		return (ENXIO);
	}
#ifdef DISK_SLICES
	dsclose(pdev, fmt, diskp->dk_slices);
	if (!dsisopen(diskp->dk_slices)) {
		goto out;
	}
out:
#endif
	error = (*diskp->dk_driver->d_close)(pdev, flag, fmt, p);
	return (error);
}

int
dkdriver_ioctl(diskp, dev, cmd, data, flag, p)
	struct dkdevice *diskp;
	dev_t 			dev;
	u_long			cmd;
	void 			*data;
	int				flag;
	struct proc 	*p;
{
	dev_t pdev;
	int error;

	pdev = disk_pdev(dev);
	if (!diskp) {
		return (ENXIO);
	}
#ifdef DISK_SLICES
	error = dsioctl(diskp, pdev, cmd, data, flag, &diskp->dk_slices);
	if (error == ENOIOCTL) {
		goto out;
	} else {
		return (error);
	}
#endif
out:
	error = (*diskp->dk_driver->d_ioctl)(pdev, cmd, data, flag, p);
	return (error);
}

int
dkdriver_dump(diskp, dev)
	struct dkdevice *diskp;
	dev_t 			dev;
{
	struct disklabel *lp;
	dev_t pdev;
	int error;

	pdev = disk_pdev(dev);
	if (!diskp) {
		return (ENXIO);
	}
	if (!diskp->dk_slices) {
		return (ENXIO);
	}
	lp = dsgetlabel(dev, diskp->dk_slices);
	if (!lp) {
		return (ENXIO);
	}
	error = (*diskp->dk_driver->d_dump)(pdev);
	return (error);
}

void
dkdriver_start(diskp, bp)
	struct dkdevice *diskp;
	struct buf *bp;
{
	disk_bufio(diskp, bp);
	(*diskp->dk_driver->d_start)(bp);
}

int
dkdriver_mklabel(diskp)
	struct dkdevice *diskp;
{
	int error;

	if (!diskp) {
		return (ENXIO);
	}
	return (error);
}

static dev_t
disk_pdev(dev)
	dev_t dev;
{
	dev_t pdev;
#ifdef DISK_SLICES
	pdev = dkmakedev(major(dev), dkunit(dev), RAW_PART);
#else
	pdev = dev;
#endif
	return (pdev);
}

static void
disk_bufio(diskp, bp)
	struct dkdevice *diskp;
	struct buf *bp;
{
	dev_t pdev;

	pdev = disk_pdev(bp->b_dev);
	if (!diskp) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
#ifdef DISK_SLICES
	if (dscheck(bp, diskp->dk_slices) <= 0) {
		biodone(bp);
		return;
	}
#endif
	biodone(bp);
}


