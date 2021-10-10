/*
 * Copyright (c) 1987, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)disklabel.h	8.1.1 (2.11BSD) 1995/04/13
 */

#ifndef	_SYS_DISKLABEL_H_
#define	_SYS_DISKLABEL_H_

#ifndef _KERNEL
#include <sys/types.h>
#include <sys/null.h>
#include <sys/disktype.h>
#endif

/*
 * Disk description table, see disktab(5)
 */
#define	_PATH_DISKTAB	"/etc/disktab"
#define	DISKTAB			_PATH_DISKTAB

/*
 * Each disk has a label which includes information about the hardware
 * disk geometry, filesystem partitions, and drive specific information.
 * The label is in block 0 or 1, possibly offset from the beginning
 * to leave room for a bootstrap, etc.
 */

/* XXX these should be defined per controller (or drive) elsewhere, not here! */

#define LABELSECTOR		1					/* sector containing label */
#define LABELOFFSET		0					/* offset of label in sector */

#ifndef	LABELSECTOR
#define LABELSECTOR		0					/* sector containing label */
#endif

#ifndef	LABELOFFSET
#define LABELOFFSET		64					/* offset of label in sector */
#endif

#define DISKMAGIC		BSD_MAGIC			/* The disk magic number */

#ifndef MAXPARTITIONS
#define	MAXPARTITIONS	8
#endif

/* Size of bootblock area in sector-size neutral bytes */
#define BBSIZE			BSD_BOOTBLOCK_SIZE

#define	LABEL_PART		BSD_PART_RAW
#define	RAW_PART		BSD_PART_RAW
#define	SWAP_PART		BSD_PART_SWAP

#define NDDATA			BSD_NDRIVEDATA
#define NSPARE			BSD_NSPARE

/*
 * 2.11BSD's disklabels are different than 4.4BSD for a couple reasons:
 *
 *	1) D space is precious in the 2.11 kernel.  Many of the fields do
 *	   not need to be a 'long' (or even a 'short'), a 'short' (or 'char')
 *	   is more than adequate.  If anyone ever ports the FFS to a PDP11 
 *	   changing the label format will be the least of the problems.
 *
 *	2) There is no need to support a bootblock more than 512 bytes long.
 *	   The hardware (disk bootroms) only read the first sector, thus the
 *	   label is always at sector 1 (the second half of the first filesystem
 *	   block).
 *
 * Almost all of the fields have been retained but with reduced sizes.  This
 * is for future expansion and to ease the porting of the various utilities
 * which use the disklabel structure.  The 2.11 kernel uses very little other
 * than the partition tables.  Indeed only the partition tables are resident
 * in the kernel address space, the actual label block is allocated external to
 * the kernel and mapped in as needed.
*/

#ifndef LOCORE
struct disklabel {
	u_int32_t 		d_magic;			/* the magic number */
	u_int16_t  		d_type;				/* drive type */
	u_int16_t  		d_subtype;			/* controller/d_type specific */
	char	  		d_typename[16];		/* type name, e.g. "eagle" */
	/* 
	 * d_packname contains the pack identifier and is returned when
	 * the disklabel is read off the disk or in-core copy.
	 * d_boot0 is the (optional) name of the primary (block 0) bootstrap
	 * as found in /mdec.  This is returned when using
	 * getdiskbyname(3) to retrieve the values from /etc/disktab.
	 */
#if defined(KERNEL) || defined(STANDALONE)
	char			d_packname[16];		/* pack identifier */
#else
	union {
		char		un_d_packname[16];	/* pack identifier */
		char		*un_d_boot0;		/* primary bootstrap name */
		char 		*un_d_boot1;		/* secondary bootstrap name */
	} d_un; 
#define d_packname	d_un.un_d_packname
#define d_boot0		d_un.un_d_boot0
#define d_boot1		d_un.un_d_boot1
#endif	/* ! KERNEL or STANDALONE */
		/* disk geometry: */
	u_int32_t 		d_secsize;			/* # of bytes per sector */
	u_int32_t 		d_nsectors;			/* # of data sectors per track */
	u_int32_t 		d_ntracks;			/* # of tracks per cylinder */
	u_int32_t 		d_ncylinders;		/* # of data cylinders per unit */
	u_int32_t 		d_secpercyl;		/* # of data sectors per cylinder */
	u_int32_t 		d_secperunit;		/* # of data sectors per unit */
	/*
	 * Spares (bad sector replacements) below
	 * are not counted in d_nsectors or d_secpercyl.
	 * Spare sectors are assumed to be physical sectors
	 * which occupy space at the end of each track and/or cylinder.
	 */
	u_int16_t 		d_sparespertrack;	/* # of spare sectors per track */
	u_int16_t 		d_sparespercyl;		/* # of spare sectors per cylinder */

	/*
	 * Alternate cylinders include maintenance, replacement,
	 * configuration description areas, etc.
	 */
	u_int32_t 		d_acylinders;		/* # of alt. cylinders per unit */

			/* hardware characteristics: */
	/*
	 * d_interleave, d_trackskew and d_cylskew describe perturbations
	 * in the media format used to compensate for a slow controller.
	 * Interleave is physical sector interleave, set up by the formatter
	 * or controller when formatting.  When interleaving is in use,
	 * logically adjacent sectors are not physically contiguous,
	 * but instead are separated by some number of sectors.
	 * It is specified as the ratio of physical sectors traversed
	 * per logical sector.  Thus an interleave of 1:1 implies contiguous
	 * layout, while 2:1 implies that logical sector 0 is separated
	 * by one sector from logical sector 1.
	 * d_trackskew is the offset of sector 0 on track N
	 * relative to sector 0 on track N-1 on the same cylinder.
	 * Finally, d_cylskew is the offset of sector 0 on cylinder N
	 * relative to sector 0 on cylinder N-1.
	 */
	u_int16_t 		d_rpm;				/* rotational speed */
	u_int16_t 		d_interleave;		/* hardware sector interleave */
	u_int16_t 		d_trackskew;		/* sector 0 skew, per track */
	u_int16_t 		d_cylskew;			/* sector 0 skew, per cylinder */
	u_int32_t 		d_headswitch;		/* head swith time, usec */
	u_int32_t 		d_trkseek;			/* track-to-track seek, msec */
	u_int32_t 		d_flags;			/* generic flags */
#define NDDATA 		BSD_NDRIVEDATA
	u_int32_t 		d_drivedata[NDDATA];/* drive-type specific information */
#define NSPARE 		BSD_NSPARE
	u_int32_t 		d_spare[NSPARE];	/* reserved for future use */
	u_int32_t 		d_magic2;			/* the magic number (again) */
	u_int16_t 		d_checksum;			/* xor of data incl. partitions */

			/* filesystem and partition information: */
	u_int16_t 		d_npartitions;		/* number of partitions in following */
	u_int32_t 		d_bbsize;			/* size of boot area at sn0, bytes */
	u_int32_t 		d_sbsize;			/* max size of fs superblock, bytes */
	struct	partition {					/* the partition table */
		u_int32_t 	p_size;				/* number of sectors in partition */
		u_int32_t 	p_offset;			/* starting sector */
		u_int32_t 	p_fsize;			/* filesystem basic fragment size */
		u_int8_t 	p_fstype;			/* filesystem type, see below */
		u_int8_t 	p_frag;				/* filesystem fragments per block */
		union {
			u_int16_t cpg;				/* UFS: FS cylinders per group */
			u_int16_t sgs;				/* LFS: FS segment shift */
		}__partition_u1;
#define	p_cpg	__partition_u1.cpg
#define	p_sgs	__partition_u1.sgs
	} d_partitions[MAXPARTITIONS];		/* actually may be more */
};

#else /* LOCORE */
/*
 * offsets for asm boot files.
 */
.set	d_secsize,40
.set	d_nsectors,44
.set	d_ntracks,48
.set	d_ncylinders,52
.set	d_secpercyl,56
.set	d_secperunit,60
.set	d_end_,276			/* size of disk label */
#endif /* LOCORE */

/*
 * Drive data for SMD.
 */
#define	d_smdflags		d_drivedata[0]
#define	D_SSE			0x1					/* supports skip sectoring */
#define	d_mindist		d_drivedata[1]
#define	d_maxdist		d_drivedata[2]
#define	d_sdist			d_drivedata[3]

/*
 * Drive data for ST506.
 */
#define d_precompcyl	d_drivedata[0]
#define d_gap3			d_drivedata[1]		/* used only when formatting */

/*
 * Drive data for SCSI.
 */
#define	d_blind			d_drivedata[0]

#ifndef LOCORE
/*
 * Structure used to perform a format
 * or other raw operation, returning data
 * and/or register values.
 * Register identification and format
 * are device- and driver-dependent.
 */
struct format_op {
	char				*df_buf;
	int					df_count;		/* value-result */
	daddr_t				df_startblk;
	int					df_reg[8];		/* result */
};

/*
 * Structure used internally to retrieve
 * information about a partition on a disk.
 */
struct partinfo {
	struct disklabel 	*disklab;
	struct partition 	*part;
};

/*
 * Disk-specific ioctls.
 */
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
/* get and set disklabel; DIOCGPART used internally */
#define DIOCGDINFO		_IOR(d, 101, struct disklabel)	/* get */
#define DIOCSDINFO		_IOW(d, 102, struct disklabel)	/* set */
#define DIOCWDINFO		_IOW(d, 103, struct disklabel)	/* set, update disk */
#define DIOCGPART		_IOW(d, 104, struct partinfo)	/* get partition */

/* do format operation, read or write */
#define DIOCRFORMAT		_IOWR(d, 105, struct format_op)
#define DIOCWFORMAT		_IOWR(d, 106, struct format_op)

#define DIOCSSTEP		_IOW(d, 107, int)				/* set step rate */
#define DIOCSRETRIES	_IOW(d, 108, int)				/* set # of retries */
#define DIOCKLABEL		_IOW(d, 119, int)				/* keep/drop label on close? */
#define DIOCWLABEL		_IOW(d, 109, int)				/* write en/disable label */

#define DIOCSBAD		_IOW(d, 110, struct dkbad)		/* set kernel dkbad */
#define DIOCEJECT		_IOW('d', 112, int)				/* eject removable disk */
#define ODIOCEJECT		_IO('d', 112)					/* eject removable disk */
#define DIOCLOCK		_IOW('d', 113, int)				/* lock/unlock pack */

/* get default label, clear label */
#define	DIOCGDEFLABEL	_IOR('d', 114, struct disklabel)
#define	DIOCCLRLABEL	_IO('d', 115)

/* disk cache enable/disable */
#define	DIOCGCACHE		_IOR('d', 116, int)				/* get cache enables */
#define	DIOCSCACHE		_IOW('d', 117, int)				/* set cache enables */

#define	DKCACHE_READ	0x000001 						/* read cache enabled */
#define	DKCACHE_WRITE	0x000002 						/* write(back) cache enabled */
#define	DKCACHE_RCHANGE	0x000100 						/* read enable is changeable */
#define	DKCACHE_WCHANGE	0x000200 						/* write enable is changeable */
#define	DKCACHE_SAVE	0x010000 						/* cache parameters are savable/save them */
#define	DKCACHE_FUA		0x020000 						/* Force Unit Access supported */
#define	DKCACHE_DPO		0x040000 						/* Disable Page Out supported */

#endif /* LOCORE */

struct dkdevice;
char	*readdisklabel (struct dkdevice *, int);
int		setdisklabel (struct dkdevice *, struct disklabel *);
int		writedisklabel (struct dkdevice *, int);
int		ioctldisklabel(struct dkdevice *, int, dev_t, int, caddr_t, int);
int		dkcksum(struct disklabel *);
int		partition_check(struct buf *, struct dkdevice *);
int		dkoverlapchk(struct disklabel *, int, dev_t, size_t, char *);

#if !defined(KERNEL) && !defined(LOCORE)
#define	LABELDESC	(((btoc(sizeof (struct disklabel)) - 1) << 8))
#include <sys/cdefs.h>
#endif

#endif	/* _SYS_DISKLABEL_H_ */
