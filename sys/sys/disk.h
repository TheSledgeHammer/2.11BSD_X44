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

#include <sys/queue.h>
#include <sys/device.h>
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

struct disklist_head;
TAILQ_HEAD(disklist_head, dkdevice);				/* the disklist is a TAILQ */
struct dkdevice {
	TAILQ_ENTRY(dkdevice)	dk_link;				/* link in global disklist */
	struct	device 	 		dk_dev;					/* base device */
	char					*dk_name;				/* disk name */
	int						dk_bps;					/* xfer rate: bytes per second */
	int						dk_bopenmask;			/* block devices open */
	int						dk_copenmask;			/* character devices open */
	int						dk_openmask;			/* composite (bopen|copen) */
	int						dk_flags;				/* label state aka dk_state */
	int						dk_blkshift;			/* shift to convert DEV_BSIZE to blks */
	int						dk_byteshift;			/* shift to convert bytes to blks */
	int						dk_busy;				/* busy counter */
	u_int64_t				dk_seek;				/* total independent seek operations */
	u_int64_t				dk_bytes;				/* total bytes transfered */
	struct timeval			dk_attachtime;			/* time disk was attached */
	struct timeval			dk_timestamp;			/* timestamp of last unbusy */
	struct timeval			dk_time;				/* total time spent busy */
	struct dkdriver 		*dk_driver;				/* pointer to driver */
	daddr_t					dk_labelsector;			/* sector containing label */
	struct disklabel 		dk_label;				/* label */
	//struct diskslices		dk_slices;				/* slices */
	struct partition 		dk_parts[MAXPARTITIONS];/* in-kernel portion */
	//struct diskslice		dk_slice;				/* slice */
	//struct cpu_disklabel 	*dk_cpulabel;
};

struct dkdriver {
	void					(*d_strategy)(struct buf *);
	void					(*d_minphys)(struct buf *);
	int						(*d_open) (dev_t, int, int, struct proc *);
	int						(*d_close) (dev_t, int, int, struct proc *);
	int						(*d_ioctl) (dev_t, int, caddr_t, int, struct proc *);
	int						(*d_dump) (dev_t);
	void					(*d_start) (struct buf *, daddr_t);
	int						(*d_mklabel) (struct dkdevice *);
};

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

void				disk_init (void);
void				disk_attach (struct dkdevice *);
void				disk_detach (struct dkdevice *);
void				disk_busy (struct dkdevice *);
void				disk_unbusy (struct dkdevice *, long);
int					disk_ioctl (struct dkdevice *, dev_t, u_long, void *, int, struct proc *);
void				disk_resetstat (struct dkdevice *);
struct dkdevice 	*disk_find (char *);
void				disksort (struct buf *, struct buf *);
int					diskerr (struct dkdevice *, struct buf *, char *, int, int);
#endif
#endif /* _SYS_DISK_H_ */
