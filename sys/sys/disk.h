/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)disk.h	8.1.2 (2.11BSD GTE) 1995/05/21
 *
 * from: $Header: disk.h,v 1.5 92/11/19 04:33:03 torek Exp $ (LBL)
 */

#ifndef	_SYS_DISK_H_
#define	_SYS_DISK_H_

#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>

/*
 * Disk device structures.
 *
 * Note that this is only a preliminary outline.  The final disk structures
 * may be somewhat different.
 *
 * Note:  the 2.11BSD version is very different.  The 4.4 version served
 *        as the inspiration. I needed something similar but for slightly 
 *	  different purposes.
 */

/*
 * Disk device structures.  Rather than replicate driver specific variations
 * of the following in each driver it was made common here.
 *
 * Some of the flags are specific to various drivers. For example ALIVE and 
 * ONLINE apply to MSCP devices more than to SMD devices while the SEEK flag
 * applies to the SMD (xp) driver but not to the MSCP driver.  The rest
 * of the flags as well as the open partition bitmaps are usable by any disk
 * driver.  One 'dkdevice' structure is needed for each disk drive supported
 * by a driver.
 *
 * The entire disklabel is not resident in the kernel address space.  Only
 * the partition table is directly accessible by the kernel.  The MSCP driver
 * does not care (or know) about the geometry of the disk.  Not holding
 * the entire label in the kernel saved quite a bit of D space.  Other drivers
 * which need geometry information from the label will have to map in the
 * label and copy out the geometry data they require.  This is unlikely to
 * cause much overhead since labels are read and written infrequently - when
 * mounting a drive, assigning a label, running newfs, etc.
 */
struct buf;
struct disklabel;
//struct cpu_disklabel;

struct dkdevice {
	struct	device 	 	dk_dev;					/* base device */
	struct	dkdevice 	*dk_next;				/* list of disks; not yet used */
	char				*dk_name;				/* disk name */
	int					dk_bps;					/* xfer rate: bytes per second */
	int					dk_bopenmask;			/* block devices open */
	int					dk_copenmask;			/* character devices open */
	int					dk_openmask;			/* composite (bopen|copen) */
	int					dk_flags;				/* label state aka dk_state */
	int					dk_blkshift;			/* shift to convert DEV_BSIZE to blks */
	int					dk_byteshift;			/* shift to convert bytes to blks */
	struct	dkdriver 	*dk_driver;				/* pointer to driver */
	daddr_t				dk_labelsector;			/* sector containing label */
	struct 	disklabel 	dk_label;				/* label */
	struct	partition 	dk_parts[MAXPARTITIONS];/* inkernel portion */
};

struct dkdriver {
	void 	(*d_strategy) (struct buf *);
#ifdef notyet
	int		(*d_open) (dev_t dev, int ifmt, int, struct proc *);
	int		(*d_close) (dev_t dev, int, int ifmt, struct proc *);
	int		(*d_ioctl) (dev_t dev, int cmd, caddr_t data, int fflag, struct proc *);
	int		(*d_dump) (dev_t);
	void	(*d_start) (struct buf *, daddr_t);
	int		(*d_mklabel) (struct dkdevice *);
#endif
};

/* states */
#define	DKF_CLOSING	0		/* drive is being closed */
#define	DKF_OPENING	1		/* drive is being opened */
#define	DKF_WANTED	2		/* drive is being waited for */
#define DKF_RLABEL  3		/* label being read */
#define	DKF_OPEN	4		/* label read, drive open */
#define	DKF_OPENRAW	5		/* open without label */
#define	DKF_WLABEL	6		/* label area is being written, */
#define	DKF_ALIVE	7		/* drive is alive */
#define	DKF_ONLINE	8		/* drive is online */
#define	DKF_SEEK	9		/* drive is seeking */
#define	DKF_SWAIT	10		/* waiting for seek to complete */

#ifdef DISKSORT_STATS
/*
 * Stats from disksort().
 */
struct disksort_stats {
	long	ds_newhead;			/* # new queue heads created */
	long	ds_newtail;			/* # new queue tails created */
	long	ds_midfirst;		/* # insertions into sort list */
	long	ds_endfirst;		/* # insertions at end of sort list */
	long	ds_newsecond;		/* # inversions (2nd lists) created */
	long	ds_midsecond;		/* # insertions into 2nd list */
	long	ds_endsecond;		/* # insertions at end of 2nd list */
};
#endif

#define	DIOCGSECTORSIZE	_IOR('d', 128, u_int)
	/*
	 * Get the sector size of the device in bytes.  The sector size is the
	 * smallest unit of data which can be transferred from this device.
	 * Usually this is a power of 2 but it might not be (i.e. CDROM audio).
	 */

#define	DIOCGMEDIASIZE	_IOR('d', 129, off_t)	/* Get media size in bytes */
	/*
	 * Get the size of the entire device in bytes.  This should be a
	 * multiple of the sector size.
	 */

/* encoding of disk minor numbers, should be elsewhere... but better
 * here than in ufs_disksubr.c
 *
 * Note: the controller number in bits 6 and 7 of the minor device are NOT
 *	 removed.  It is the responsibility of the driver to extract or mask
 *	 these bits.
*/

#define dkunit(dev)			(minor(dev) >> 3)
#define dkpart(dev)			(minor(dev) & 07)
#define dkminor(unit, part)	(((unit) << 3) | (part))

#ifdef KERNEL
extern	int disk_count;			/* number of disks in global disklist */

void	disk_init (void);
void	disk_attach (struct dkdevice *);
void	disk_detach (struct dkdevice *);
void	disk_busy (struct dkdevice *);
void	disk_unbusy (struct dkdevice *, long);
void	disk_resetstat (struct dkdevice *);
struct	disk *disk_find (char *);

struct device;
void	dk_establish (struct dkdevice *, struct device *);

void	disksort (struct buf *, struct buf *);
char	*readdisklabel (struct dkdevice *, int);
int		setdisklabel (struct dkdevice *, struct disklabel *);
int		writedisklabel (struct dkdevice *, int);
int		diskerr (struct dkdevice *, struct buf *, char *, int, int);
#endif
#endif /* _SYS_DISK_H_ */
