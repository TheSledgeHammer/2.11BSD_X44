/*-
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
 *	@(#)disklabel.h	8.2 (Berkeley) 7/10/94
 * 	$FreeBSD$
 */

#ifndef _SYS_DISKTYPE_H_
#define _SYS_DISKTYPE_H_

/* The disk magic number */
#define BSD_MAGIC			0x82564557U

#define	BSD_NPARTS_MIN		8
#define	BSD_NPARTS_MAX		20

/* Size of bootblock area in sector-size neutral bytes */
#define BSD_BOOTBLOCK_SIZE	8192

/* partition containing whole disk */
#define	BSD_PART_RAW		2

/* partition normally containing swap */
#define	BSD_PART_SWAP		1

/* Drive-type specific data size (in number of 32-bit inegrals) */
#define	BSD_NDRIVEDATA		5

/* Number of spare 32-bit integrals following drive-type data */
#define	BSD_NSPARE			5

/* d_type values: */
#define	DTYPE_SMD			1		/* SMD, XSMD; VAX hp/up */
#define	DTYPE_MSCP			2		/* MSCP */
#define	DTYPE_DEC			3		/* other DEC (rk, rl) */
#define	DTYPE_SCSI			4		/* SCSI */
#define	DTYPE_ESDI			5		/* ESDI interface */
#define	DTYPE_ST506			6		/* ST506 etc. */
#define	DTYPE_HPIB			7		/* CS/80 on HP-IB */
#define	DTYPE_HPFL			8		/* HP Fiber-link */
#define	DTYPE_FLOPPY		10		/* floppy */
#define	DTYPE_CCD			11		/* concatenated disk */
#define	DTYPE_VND			12		/* vnode pseudo-disk */
#define	DTYPE_VINUM			13		/* vinum volume */
#define	DTYPE_DOC2K			14		/* Msys DiskOnChip */
#define	DTYPE_RAID			15		/* CMU RAIDFrame */
#define	DTYPE_JFS2			16		/* IBM JFS 2 */

/* d_subtype values: */
#define DSTYPE_INDOSPART	0x8			/* is inside dos partition */
#define DSTYPE_DOSPART(s)	((s) & 3)	/* dos partition number */
#define DSTYPE_GEOMETRY		0x10		/* drive params in label */

#ifdef DKTYPENAMES
static char *dktypenames[] = {
		"unknown",
		"SMD",
		"MSCP",
		"old DEC",
		"SCSI",
		"ESDI",
		"ST506",
		"HP-IB",
		"HP-FL",
		"type 9",
		"floppy",
		"ccd",
		"vnd",
		"Vinum",
		"DOC2K",
		"Raid",
		"?",
		"jfs",
		NULL
};
#define DKMAXTYPES	(sizeof(dktypenames) / sizeof(dktypenames[0]) - 1)
#endif

/*
 * Filesystem type and version.
 * Used to interpret other filesystem-specific
 * per-partition information.
 */
/*
 * 2.11BSD uses type 5 filesystems even though block numbers are 4 bytes
 * (rather than the packed 3 byte format) and the directory structure is
 * that of 4.3BSD (long filenames).
*/
#define	FS_UNUSED			0		/* unused */
#define	FS_SWAP				1		/* swap */
#define	FS_V6				2		/* Sixth Edition */
#define	FS_V7				3		/* Seventh Edition */
#define	FS_SYSV				4		/* System V */
#define	FS_V71K				5		/* V7 with 1K blocks (4.1, 2.9) */
#define	FS_V8				6		/* Eighth Edition, 4K blocks */
#define	FS_BSDFFS			7		/* 4.2BSD fast filesystem */
#define	FS_MSDOS			8		/* MSDOS filesystem */
#define	FS_BSDLFS			9		/* 4.4BSD log-structured filesystem */
#define	FS_OTHER			10		/* in use, but unknown/unsupported */
#define	FS_HPFS				11		/* OS/2 high-performance filesystem */
#define	FS_ISO9660			12		/* ISO 9660, normally CD-ROM */
#define	FS_BOOT				13		/* partition contains bootstrap */
#define	FS_VINUM			14		/* Vinum drive */
#define	FS_RAID				15		/* RAIDFrame drive */
#define	FS_FILECORE			16		/* Acorn Filecore Filing System */
#define	FS_EXT2FS			17		/* ext2fs */
#define	FS_NTFS				18		/* Windows/NT file system */
#define	FS_CCD				20		/* concatenated disk component */
#define	FS_JFS2				22		/* IBM JFS2 */
#define	FS_HAMMER			23		/* DragonFlyBSD Hammer FS */
#define	FS_HAMMER2			24		/* DragonFlyBSD Hammer2 FS */
#define	FS_UDF				25		/* UDF */
#define	FS_EFS				26		/* SGI's Extent File system */
#define	FS_ZFS				27		/* Sun's ZFS */
#define	FS_NANDFS			30		/* FreeBSD nandfs (NiLFS derived) */

#ifdef	FSTYPENAMES
static char *fstypenames[] = {
	"unused",
	"swap",
	"Version 6",
	"Version 7",
	"System V",
	"2.11BSD",
	"Eighth Edition",
	"4.2BSD",
	"MSDOS",
	"4.4LFS",
	"unknown",
	"HPFS",
	"ISO9660",
	"boot",
	"Vinum",
	"Raid",
	"Filecore",
	"EXT2FS",
	"NTFS",
	"?",
	"ccd",
	"jfs",
	"HAMMER",
	"HAMMER2",
	"UDF",
	"?",
	"EFS",
	"ZFS",
	"?",
	"?",
	"nandfs",
	NULL
};
#define FSMAXTYPES	(sizeof(fstypenames) / sizeof(fstypenames[0]) - 1)
#endif

/*
 * flags shared by various drives:
 */
#define	D_REMOVABLE		0x01	/* removable media */
#define	D_ECC			0x02	/* supports ECC */
#define	D_BADSECT		0x04	/* supports bad sector forw. */
#define	D_RAMDISK		0x08	/* disk emulator */
#define	D_CHAIN			0x10	/* can do back-back transfers */


#endif /* _SYS_DISKTYPE_H_ */
