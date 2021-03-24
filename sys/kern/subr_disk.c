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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/user.h>

#include <machine/param.h>

struct disklist_head disklist;	/* TAILQ_HEAD */
int	disk_count;					/* number of drives in global disklist */

/*
 * Initialize the disklist.  Called by main() before autoconfiguration.
 */
void
disk_init()
{
	TAILQ_INIT(&disklist);
	disk_count = 0;
}

/*
 * Searches the disklist for the disk corresponding to the
 * name provided.
 */
struct disk *
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

/*
 * Attach a disk.
 */
void
disk_attach(diskp)
	struct dkdevice *diskp;
{
	diskp->dk_label = malloc(sizeof(struct disklabel), M_DEVBUF, M_NOWAIT);
	diskp->dk_cpulabel = malloc(sizeof(struct cpu_disklabel), M_DEVBUF, M_NOWAIT);
	if ((diskp->dk_label == NULL) || (diskp->dk_cpulabel == NULL)) {
			panic("disk_attach: can't allocate storage for disklabel");
	}

	memset(diskp->dk_label, 0, sizeof(struct disklabel));
	memset(diskp->dk_cpulabel, 0, sizeof(struct cpu_disklabel));

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
#define	b_cylinder	b_resid

void
disksort(ap, bp)
	register struct buf *ap, *bp;
{
	register struct buf *bq;

	/* If the queue is empty, then it's easy. */
	if (ap->b_actf == NULL) {
		bp->b_actf = NULL;
		ap->b_actf = bp;
		return;
	}

	/*
	 * If we lie after the first (currently active) request, then we
	 * must locate the second request list and add ourselves to it.
	 */
	bq = ap->b_actf;
	if (bp->b_cylinder < bq->b_cylinder) {
		while (bq->b_actf) {
			/*
			 * Check for an ``inversion'' in the normally ascending
			 * cylinder numbers, indicating the start of the second
			 * request list.
			 */
			if (bq->b_actf->b_cylinder < bq->b_cylinder) {
				/*
				 * Search the second request list for the first
				 * request at a larger cylinder number.  We go
				 * before that; if there is no such request, we
				 * go at end.
				 */
				do {
					if (bp->b_cylinder < bq->b_actf->b_cylinder) {
						goto insert;
					}
					if (bp->b_cylinder == bq->b_actf->b_cylinder
							&& bp->b_blkno < bq->b_actf->b_blkno) {
						goto insert;
					}
					bq = bq->b_actf;
				} while (bq->b_actf);
				goto insert;
				/* after last */
			}
			bq = bq->b_actf;
		}
		/*
		 * No inversions... we will go after the last, and
		 * be the first request in the second request list.
		 */
		goto insert;
	}
	/*
	 * Request is at/after the current request...
	 * sort in the first request list.
	 */
	while (bq->b_actf) {
		/*
		 * We want to go after the current request if there is an
		 * inversion after it (i.e. it is the end of the first
		 * request list), or if the next request is a larger cylinder
		 * than our request.
		 */
		if (bq->b_actf->b_cylinder < bq->b_cylinder
				|| bp->b_cylinder < bq->b_actf->b_cylinder
				|| (bp->b_cylinder == bq->b_actf->b_cylinder
						&& bp->b_blkno < bq->b_actf->b_blkno)) {
			goto insert;
		}
		bq = bq->b_actf;
	}
	/*
	 * Neither a second list nor a larger request... we go at the end of
	 * the first list, which is the same as the end of the whole schebang.
	 */
insert:	bp->b_actf = bq->b_actf;
	bq->b_actf = bp;
}

/*
 * Attempt to read a disk label from a device using the indicated stategy
 * routine.  The label must be partly set up before this: secpercyl and
 * anything required in the strategy routine (e.g., sector size) must be
 * filled in before calling us.  Returns NULL on success and an error
 * string on failure.
 */
char *
readdisklabel(dev, strat, lp)
	dev_t dev;
	int (*strat)();
	register struct disklabel *lp;
{
	register struct buf *bp;
	struct disklabel *dlp;
	char *msg = NULL;

	if (lp->d_secperunit == 0) {
		lp->d_secperunit = 0x1fffffff;
	}
	lp->d_npartitions = 1;
	if (lp->d_partitions[0].p_size == 0) {
		lp->d_partitions[0].p_size = 0x1fffffff;
	}
	lp->d_partitions[0].p_offset = 0;

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ;
	bp->b_cylinder = LABELSECTOR / lp->d_secpercyl;
	(*strat)(bp);
	if (biowait(bp)) {
		msg = "I/O error";
	} else for (dlp = (struct disklabel *)bp->b_data; dlp <= (struct disklabel *)((char *)bp->b_data + DEV_BSIZE - sizeof(*dlp)); dlp = (struct disklabel *)((char *)dlp + sizeof(long))) {
		if (dlp->d_magic != DISKMAGIC || dlp->d_magic2 != DISKMAGIC) {
			if (msg == NULL) {
				msg = "no disk label";
			}
		} else if (dlp->d_npartitions > MAXPARTITIONS || dkcksum(dlp) != 0) {
			msg = "disk label corrupted";
		} else {
			*lp = *dlp;
			msg = NULL;
			break;
		}
	}
	bp->b_flags = B_INVAL | B_AGE;
	brelse(bp);
	return (msg);
}

/*
 * Check new disk label for sensibility before setting it.
 */
int
setdisklabel(olp, nlp, openmask)
	register struct disklabel *olp, *nlp;
	u_long openmask;
{
	register i;
	register struct partition *opp, *npp;

	if (nlp->d_magic != DISKMAGIC || nlp->d_magic2 != DISKMAGIC || dkcksum(nlp) != 0) {
		return (EINVAL);
	}
	while ((i = ffs((long)openmask)) != 0) {
		i--;
		openmask &= ~(1 << i);
		if (nlp->d_npartitions <= i) {
			return (EBUSY);
		}
		opp = &olp->d_partitions[i];
		npp = &nlp->d_partitions[i];
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
 	nlp->d_checksum = 0;
 	nlp->d_checksum = dkcksum(nlp);
	*olp = *nlp;
	return (0);
}

int
writedisklabel(dev, strat, lp)
	dev_t dev;
	int (*strat)();
	register struct disklabel *lp;
{
	struct buf *bp;
	struct disklabel *dlp;
	int labelpart;
	int error = 0;

	labelpart = dkpart(dev);
	if (lp->d_partitions[labelpart].p_offset != 0) {
		if (lp->d_partitions[0].p_offset != 0) {
			return (EXDEV); /* not quite right */
		}
		labelpart = 0;
	}
	bp = geteblk((int) lp->d_secsize);
	bp->b_dev = makedev(major(dev), dkminor(dkunit(dev), labelpart));
	bp->b_blkno = LABELSECTOR;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_READ;
	(*strat)(bp);
	if (error == biowait(bp)) {
		goto done;
	}
	for (dlp = (struct disklabel*) bp->b_data; dlp <= (struct disklabel*) ((char*) bp->b_data + lp->d_secsize - sizeof(*dlp)); dlp = (struct disklabel*) ((char*) dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC && dkcksum(dlp) == 0) {
			*dlp = *lp;
			bp->b_flags = B_WRITE;
			(*strat)(bp);
			error = biowait(bp);
			goto done;
		}
	}
	error = ESRCH;
done:
	brelse(bp);
	return (error);
}

/*
 * Compute checksum for disk label.
 */
int
dkcksum(lp)
	struct disklabel *lp;
{
	register u_short *start, *end;
	register u_short sum = 0;

	start = (u_short *)lp;
	end = (u_short *)&lp->d_partitions[lp->d_npartitions];
	while (start < end) {
		sum ^= *start++;
	}
	return (sum);
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
	char *dname, *what;
	int pri, blkdone;
	register struct disklabel *lp;
{
	int unit = dkunit(bp->b_dev), part = dkpart(bp->b_dev);
	register void (*pr) (const char *, ...);
	char partname = 'a' + part;
	int sn;

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
 * This was extracted from the MSCP driver so it could be shared between
 * all disk drivers which implement disk labels.
*/
int
partition_check(bp, dk)
	struct	buf *bp;
	struct	dkdevice *dk;
{
	struct partition *pi;
	daddr_t sz;

	pi = &dk->dk_parts[dkpart(bp->b_dev)];

	/* Valid block in device partition */
	sz = (bp->b_bcount + 511) >> 9;
	if (bp->b_blkno < 0 || bp->b_blkno + sz > pi->p_size) {
		sz = pi->p_size - bp->b_blkno;
		/* if exactly at end of disk, return an EOF */
		if (sz == 0) {
			bp->b_resid = bp->b_bcount;
			goto done;
		}
		/* or truncate if part of it fits */
		if (sz < 0) {
			bp->b_error = EINVAL;
			goto bad;
		}
		bp->b_bcount = dbtob(sz); /* compute byte count */
	}
	/*
	 * Check for write to write-protected label area.  This does not include
	 * sector 0 which is the boot block.
	 */
	if (bp->b_blkno + pi->p_offset <= LABELSECTOR && bp->b_blkno + pi->p_offset + sz > LABELSECTOR && !(bp->b_flags & B_READ) && !(dk->dk_flags & DKF_WLABEL)) {
		bp->b_error = EROFS;
		goto bad;
	}
	return(1);		/* success */
bad:
	return(-1);
done:
	return(0);
}

/*
 * This is new for 2.11BSD.  It is a common routine that checks for
 * opening a partition that overlaps other currently open partitions.
 *
 * NOTE: if 'c' is not the entire drive (as is the case with the old,
 * nonstandard, haphazard and overlapping partition tables) the warning
 * message will be erroneously issued in some valid situations.
*/

#define	RAWPART		2	/* 'c' */  /* XXX */

int
dkoverlapchk(lp, openmask, dev, label, name)
	struct disklabel *lp;
	int				openmask;
	dev_t			dev;
	size_t			label;
	char			*name;
{
	int unit = dkunit(dev);
	int part = dkpart(dev);
	int partmask = 1 << part;
	int i;
	daddr_t start, end;
	register struct partition *pp;

	if ((openmask & partmask) == 0 && part != RAWPART) {
		pp = &lp->d_partitions[part];
		start = pp->p_offset;
		end = pp->p_offset + pp->p_size;
		i = 0;
		for (pp = lp->d_partitions; i < lp->d_npartitions; pp++, i++) {
			if (pp->p_offset + pp->p_size <= start|| pp->p_offset >= end || i == RAWPART) {
				continue;
			}
			if (openmask & (1 << i)) {
				log(LOG_WARNING, "%s%d%c: overlaps open part (%c)\n", name, unit, part + 'a', i + 'a');
			}
		}
	}
	return (0);
}
