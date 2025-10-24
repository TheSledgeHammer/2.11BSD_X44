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

#include <sys/time.h>
#include <sys/queue.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/diskslice.h>
#include <sys/dkbad.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>

#include <machine/disklabel.h>

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
struct bufq_state;
struct disklabel;
struct diskslices;

struct disklist_head;
TAILQ_HEAD(disklist_head, dkdevice);			/* the disklist is a TAILQ */
struct dkdevice {
	TAILQ_ENTRY(dkdevice)	dk_link;			/* link in global disklist */
	struct	device 	 	dk_dev;					/* base device */
	char				*dk_name;				/* disk name */
	int					dk_bps;					/* xfer rate: bytes per second */
	int					dk_bopenmask;			/* block devices open */
	int					dk_copenmask;			/* character devices open */
	int					dk_openmask;			/* composite (bopen|copen) */
	int					dk_flags;				/* label state aka dk_state */
	int					dk_blkshift;			/* shift to convert DEV_BSIZE to blks */
	int					dk_byteshift;			/* shift to convert bytes to blks */
	int					dk_busy;				/* busy counter */
	u_int64_t			dk_seek;				/* total independent seek operations */
	u_int64_t			dk_bytes;				/* total bytes transfered */
	struct timeval		dk_attachtime;			/* time disk was attached */
	struct timeval		dk_timestamp;			/* timestamp of last unbusy */
	struct timeval		dk_time;				/* total time spent busy */
	struct dkdriver 	*dk_driver;				/* pointer to driver */
	daddr_t				dk_labelsector;			/* sector containing label */
	daddr_t				dk_badsector[127];		/* 126 plus trailing -1 marker */

	//struct mtx			*dk_openlock;
	struct disklabel 	*dk_label;				/* label */
	struct diskslices	*dk_slices;				/* slices */
	struct partition 	dk_parts[MAXPARTITIONS];/* in-kernel portion */
	struct cpu_disklabel *dk_cpulabel;
};

#define	DK_DISKNAMELEN	16

#ifdef _KERNEL

struct dkdriver {
	void				(*d_strategy)(struct buf *);
	void				(*d_minphys)(struct buf *);
	int					(*d_open)(dev_t, int, int, struct proc *);
	int					(*d_close)(dev_t, int, int, struct proc *);
	int					(*d_ioctl)(dev_t, u_long, void *, int, struct proc *);
	int					(*d_dump)(dev_t);
	void				(*d_start)(struct buf *);
	int					(*d_mklabel)(struct dkdevice *);
};

/* dkdriver ops */
void 	dkdriver_strategy(struct dkdevice *, struct buf *);
void 	dkdriver_minphys(struct dkdevice *, struct buf *);
int 	dkdriver_open(struct dkdevice *, dev_t, int, int, struct proc *);
int 	dkdriver_close(struct dkdevice *, dev_t, int, int, struct proc *);
int 	dkdriver_ioctl(struct dkdevice *, dev_t, u_long, void *, int, struct proc *);
int		dkdriver_dump(struct dkdevice *, dev_t);
void 	dkdriver_start(struct dkdevice *, struct buf *);
int		dkdriver_mklabel(struct dkdevice *);

#define DKOP_STRATEGY(diskp, bp)			((dkdriver_strategy)(diskp, bp))
#define DKOP_MINPHYS(diskp, bp)				((dkdriver_minphys)(diskp, bp))
#define DKOP_OPEN(diskp, dev, flag, fmt, p)		((dkdriver_open)(diskp, dev, flag, fmt, p))
#define DKOP_CLOSE(diskp, dev, flag, fmt, p)		((dkdriver_close)(diskp, dev, flag, fmt, p))
#define DKOP_IOCTL(diskp, dev, cmd, data, flag, p)	((dkdriver_ioctl)(diskp, dev, cmd, data, flag, p))
#define DKOP_DUMP(diskp, dev)				((dkdriver_dump)(diskp, dev))
#define DKOP_START(diskp, bp)				((dkdriver_start)(diskp, bp))
#define DKOP_MKLABEL(diskp) 				((dkdriver_mklabel)(diskp))

#endif /* _KERNEL */

/* states */
#define	DKF_OPENING			0x0001				/* drive is being opened */
#define	DKF_CLOSING			0x0002				/* drive is being closed */
#define	DKF_WANTED			0x0004				/* drive is being waited for */
#define	DKF_ALIVE			0x0008				/* drive is alive */
#define	DKF_ONLINE			0x0010				/* drive is online */
#define	DKF_WLABEL			0x0020				/* label area is being written */
#define	DKF_SEEK			0x0040				/* drive is seeking */
#define	DKF_SWAIT			0x0080				/* waiting for seek to complete */
#define DKF_RLABEL  		0x0100				/* label being read */
#define	DKF_OPEN			0x0200				/* label read, drive open */
#define	DKF_OPENRAW			0x0400				/* open without label */
#define DKF_KLABEL			0x0800				/* retain label after 'full' close */

#ifdef DISKSORT_STATS
/*
 * Stats from disksort().
 */
struct disksort_stats {
	long					ds_newhead;			/* # new queue heads created */
	long					ds_newtail;			/* # new queue tails created */
	long					ds_midfirst;		/* # insertions into sort list */
	long					ds_endfirst;		/* # insertions at end of sort list */
	long					ds_newsecond;		/* # inversions (2nd lists) created */
	long					ds_midsecond;		/* # insertions into 2nd list */
	long					ds_endsecond;		/* # insertions at end of 2nd list */
};
#endif

/*
 * stats for disk.
 */
struct disk_stats {
	char 			*ds_name;
	int 			ds_busy;
	int				ds_bps;
	u_int64_t 		ds_seek;
	u_int64_t 		ds_bytes;
	struct timeval	ds_attachtime;
	struct timeval	ds_timestamp;
	struct timeval	ds_time;
};

/*
 * Bad sector lists per fixed disk
 */
struct dkbadsectors {
	SLIST_ENTRY(dkbadsectors)	dbs_next;
	daddr_t			dbs_min;		/* min. sector number */
	daddr_t			dbs_max;		/* max. sector number */
	struct timeval	dbs_failedat;	/* first failure at */
};

struct dkbadsecinfo {
	u_int32_t		dbsi_bufsize;	/* size of region pointed to */
	u_int32_t		dbsi_skip;		/* how many to skip past */
	u_int32_t		dbsi_copied;	/* how many got copied back */
	u_int32_t		dbsi_left;		/* remaining to copy */
	caddr_t			dbsi_buffer;	/* region to copy disk_badsectors to */
};

/* encoding of disk minor numbers, should be elsewhere... but better
 * here than in ufs_disksubr.c
 *
 * Note: the controller number in bits 6 and 7 of the minor device are NOT
 *	 removed.  It is the responsibility of the driver to extract or mask
 *	 these bits.
*/

#define dkunit(dev)					(minor(dev) >> 3)
#define dkpart(dev)					(minor(dev) & 07)
#define dkminor(unit, part)			(((unit) << 3) | (part))
#define dkmakedev(maj, unit, part) 	(makedev(maj, dkminor(unit, part)))

#ifdef _KERNEL
struct bdevsw;
struct cdevsw;

extern	int 		disk_count;			/* number of disks in global disklist */

void				disk_init(void);
void				disk_attach(struct dkdevice *, const struct bdevsw *, const struct cdevsw *);
void				disk_detach(struct dkdevice *);
void				disk_busy(struct dkdevice *);
void				disk_unbusy(struct dkdevice *, long);
void				disk_resetstat(struct dkdevice *);
struct dkdevice 	*disk_find(char *);
void				disksort(struct bufq_state *, struct buf *, int);
void				diskerr(struct buf *, char *, const char *, int, int, struct disklabel *);
int					bounds_check_with_mediasize(struct buf *, int, u_int64_t);
int					bounds_check_with_label(struct dkdevice *, struct buf *, int);

struct dkdriver 	*disk_driver(struct dkdevice *, dev_t);
struct disklabel	*disk_label(struct dkdevice *, dev_t);
struct diskslices	*disk_slices(struct dkdevice *, dev_t);
struct partition	*disk_partition(struct dkdevice *, dev_t);
struct device		*disk_device(struct dkdevice *, dev_t);
struct dkdevice     *disk_find_by_dev(dev_t);
char 				*devtoname(dev_t);
struct dkdevice     *disk_find_by_slice(struct diskslices *);
#endif
#endif /* _SYS_DISK_H_ */
