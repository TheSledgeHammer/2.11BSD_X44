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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/dkbad.h>
#include <sys/user.h>

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
	void (*strat)(struct buf *);
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
	bp->b_cylin = LABELSECTOR / lp->d_secpercyl;
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
 * Check new disk label for sensibility before setting it.  'olp' must point
 * to a kernel resident (or mapped in) label.  'nlp' points to the new label
 * usually present on the stack (having been passed in via ioctl from an
 * application).
 */
int
setdisklabel(olp, nlp, openmask)
	register struct disklabel *olp, *nlp;
	u_long openmask;
{
	register int i;
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
 	bcopy(nlp, olp, sizeof (struct disklabel));
	return (0);
}

/*
 * Write disk label back to device after modification.
 */
int
writedisklabel(dev, strat, lp)
	dev_t dev;
	void (*strat)(struct buf *);
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
	if ((error = biowait(bp))) {
		goto done;
	}
	for (dlp = (struct disklabel*) bp->b_data; dlp <= (struct disklabel*) ((char*) bp->b_data + lp->d_secsize - sizeof(*dlp)); dlp = (struct disklabel*) ((char*) dlp + sizeof(long))) {
		if (dlp->d_magic == DISKMAGIC && dlp->d_magic2 == DISKMAGIC && dkcksum(dlp) == 0) {
			bcopy(lp, dlp, sizeof (struct disklabel));
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
 * It was noticed that the ioctl processing of disklabels was the same
 * for every disk driver.  Disk drivers should call this routine after
 * handling ioctls (if any) particular to themselves.
*/
int
ioctldisklabel(disk, strat, dev, cmd, data, flag)
	struct dkdevice *disk;
	void (*strat)(struct buf *);
	dev_t	dev;
	int		cmd;
	void 	*data;
	int		flag;
{
  register struct disklabel *lp;
  int error;
  disk->dk_label = LABELDESC;
  bcopy((struct disklabel *)disk->dk_label, lp, sizeof (*lp));

  switch (cmd) {
  	case DIOCSBAD:
  		if ((flag & FWRITE) == 0) {
  			return (EBADF);
  		}
  		disk->dk_cpulabel->bad = *(struct dkbad *)data;
  		disk->dk_label->d_flags |= D_BADSECT;
  		dkbadintern(disk);
  		return (0);

	case DIOCGDINFO:
		bcopy(lp, (struct disklabel*) data, sizeof(*lp));
		return (0);

	case DIOCGPART:
		((struct partinfo*) data)->disklab = (struct disklabel*) disk->dk_label;
		((struct partinfo*) data)->part = &disk->dk_parts[dkpart(dev)];
		return (0);

	case DIOCKLABEL:
		if (*(int *) data) {
			disk->dk_flags |= DKF_KLABEL;
		} else {
			disk->dk_flags &= ~DKF_KLABEL;
		}
		return (0);

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0) {
			return (EBADF);
		}
		if (*(int*) data) {
			disk->dk_flags |= DKF_WLABEL;
		} else {
			disk->dk_flags &= ~DKF_WLABEL;
		}
		return (0);

	case DIOCSDINFO:
		if ((flag & FWRITE) == 0) {
			return (EBADF);
		}
		error = setdisklabel(lp, (struct disklabel*) data,
				disk->dk_flags & DKF_WLABEL ? 0 : disk->dk_openmask);
		if (error == 0) {
			disk->dk_label = LABELDESC;
			bcopy((struct disklabel*) disk->dk_label, lp, sizeof(*lp));
			bcopy(&lp->d_partitions, &disk->dk_parts, sizeof(lp->d_partitions));
		}
		return (error);

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0) {
			return (EBADF);
		}
		error = setdisklabel(lp, (struct disklabel*) data,
				disk->dk_flags & DKF_WLABEL ? 0 : disk->dk_openmask);
		if (error) {
			return (error);
		}
		disk->dk_label = LABELDESC;
		bcopy((struct disklabel*) disk->dk_label, lp, sizeof(*lp));
		bcopy(&lp->d_partitions, &disk->dk_parts, sizeof(lp->d_partitions));

		flag = disk->dk_flags;
		disk->dk_flags |= (DKF_ALIVE | DKF_WLABEL);
		error = writedisklabel(dev & ~7, strat, lp);
		disk->dk_flags = flag;
		return (error);

	case DIOCGSECTORSIZE:
		*(u_int *)data = disk->dk_label->d_secsize;
		return (0);

	case DIOCGMEDIASIZE:
		*(off_t *)data = (off_t)disk->dk_label->d_secsize * disk->dk_label->d_secperunit;
		return (0);

	default:
		return (ENOIOCTL);
	}

	return (EINVAL);
}

void
dkbadintern(disk)
	struct dkdevice *disk;
{
	struct dkbad *bt;
	struct disklabel *lp;
	int i;

	bt = &disk->dk_cpulabel->bad;
	lp = disk->dk_label;
	i = 0;

	for (; i < MAXBAD; i++) {
		if (bt->bt_bad[i].bt_cyl == 0xffff) {
			break;
		}
		disk->dk_badsector[i] = bt->bt_bad[i].bt_cyl * lp->d_secpercyl
				+ (bt->bt_bad[i].bt_trksec >> 8) * lp->d_nsectors
				+ (bt->bt_bad[i].bt_trksec & 0xff);
	}
	for (; i < MAXBAD + 1; i++) {
		disk->dk_badsector[i] = -1;
	}
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
 * This is new for 2.11BSD.  It is a common routine that checks for
 * opening a partition that overlaps other currently open partitions.
 *
 * NOTE: if 'c' is not the entire drive (as is the case with the old,
 * nonstandard, haphazard and overlapping partition tables) the warning
 * message will be erroneously issued in some valid situations.
*/
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

	if ((openmask & partmask) == 0 && part != RAW_PART) {
		pp = &lp->d_partitions[part];
		start = pp->p_offset;
		end = pp->p_offset + pp->p_size;
		i = 0;
		for (pp = lp->d_partitions; i < lp->d_npartitions; pp++, i++) {
			if (pp->p_offset + pp->p_size <= start|| pp->p_offset >= end || i == RAW_PART) {
				continue;
			}
			if (openmask & (1 << i)) {
				log(LOG_WARNING, "%s%d%c: overlaps open part (%c)\n", name, unit, part + 'a', i + 'a');
			}
		}
	}
	return (0);
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
	return (1);		/* success */
bad:
	return (-1);
done:
	return (0);
}
