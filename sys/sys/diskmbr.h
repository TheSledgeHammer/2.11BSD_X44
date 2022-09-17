/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
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
 * $FreeBSD$
 */

#ifndef _SYS_DISKMBR_H_
#define	_SYS_DISKMBR_H_

#define	DOSBBSECTOR			0		/* DOS boot block relative sector number */
#define	DOSDSNOFF			440		/* WinNT/2K/XP Drive Serial Number offset */
#define	DOSPARTOFF			446
#define	DOSPARTSIZE			16
#define	NDOSPART			4
#define	NEXTDOSPART			32
#define	DOSMAGICOFFSET		510
#define	DOSMAGIC			0xAA55

#define MSECTOR_SIZE		512		/* MSDOS sector size in bytes */
#define MDIR_SIZE			32		/* MSDOS directory size in bytes */
#define MAX_CLUSTER			8192	/* largest cluster size */
#define MAX_PATH			128		/* largest MSDOS path length */
#define MAX_DIR_SECS		64		/* largest directory (in sectors) */

#define NEW					1
#define OLD					0

#define	DOSPTYP_EXT			0x05	/* DOS extended partition */
#define	DOSPTYP_FAT16		0x06	/* FAT16 partition */
#define	DOSPTYP_NTFS		0x07	/* NTFS partition */
#define	DOSPTYP_FAT32		0x0b	/* FAT32 partition */
#define	DOSPTYP_FAT32LBA	0x0c	/* FAT32 with LBA partition */
#define	DOSPTYP_EXTLBA		0x0f	/* DOS extended partition */
#define	DOSPTYP_PPCBOOT		0x41	/* PReP/CHRP boot partition */
#define	DOSPTYP_LDM			0x42	/* Win2k dynamic extended partition */
#define	DOSPTYP_DFLYBSD		0x6c	/* DragonFlyBSD partition type */
#define	DOSPTYP_LINSWP		0x82	/* Linux swap partition */
#define	DOSPTYP_LINUX		0x83	/* Linux partition */
#define	DOSPTYP_LINLVM		0x8e	/* Linux LVM partition */
#define	DOSPTYP_386BSD		0xa5	/* 386BSD partition type */
#define	DOSPTYP_APPLE_UFS	0xa8	/* Apple Mac OS X boot */
#define	DOSPTYP_APPLE_BOOT	0xab	/* Apple Mac OS X UFS */
#define	DOSPTYP_HFS			0xaf	/* HFS/HFS+ partition type */
#define	DOSPTYP_PMBR		0xee	/* GPT Protective MBR */
#define	DOSPTYP_EFI			0xef	/* EFI FAT parition */
#define	DOSPTYP_VMFS		0xfb	/* VMware VMFS partition */
#define	DOSPTYP_VMKDIAG		0xfc	/* VMware vmkDiagnostic partition */
#define	DOSPTYP_LINRAID		0xfd	/* Linux raid partition */

/* DOS partition table -- located in boot block */

struct dos_partition {
	unsigned char	dp_flag;	/* bootstrap flags */
	unsigned char	dp_shd;		/* starting head */
	unsigned char	dp_ssect;	/* starting sector */
	unsigned char	dp_scyl;	/* starting cylinder */
	unsigned char	dp_typ;		/* partition type */
	unsigned char	dp_ehd;		/* end head */
	unsigned char	dp_esect;	/* end sector */
	unsigned char	dp_ecyl;	/* end cylinder */
	uint32_t		dp_start;	/* absolute starting sector number */
	uint32_t		dp_size;	/* partition size in sectors */
};
struct dos_partition dos_partitions[NDOSPART];

#ifdef CTASSERT
CTASSERT(sizeof (struct dos_partition) == DOSPARTSIZE);
#endif

struct directory {
	unsigned char name[8];		/* file name */
	unsigned char ext[3];		/* file extension */
	unsigned char attr;			/* attribute byte */
	unsigned char reserved[10];	/* ?? */
	unsigned char time[2];		/* time stamp */
	unsigned char date[2];		/* date stamp */
	unsigned char start[2];		/* starting cluster number */
	unsigned char size[4];		/* size of the file */
};

struct bootsector {
	unsigned char jump[3];		/* Jump to boot code */
	unsigned char banner[8];	/* OEM name & version */
	unsigned char secsiz[2];	/* Bytes per sector hopefully 512 */
	unsigned char clsiz;		/* Cluster size in sectors */
	unsigned char nrsvsect[2];	/* Number of reserved (boot) sectors */
	unsigned char nfat;			/* Number of FAT tables hopefully 2 */
	unsigned char dirents[2];	/* Number of directory slots */
	unsigned char psect[2];		/* Total sectors on disk */
	unsigned char descr;		/* Media descriptor=first byte of FAT */
	unsigned char fatlen[2];	/* Sectors in FAT */
	unsigned char nsect[2];		/* Sectors/track */
	unsigned char nheads[2];	/* Heads */
	unsigned char nhs[4];		/* number of hidden sectors */
	unsigned char bigsect[4];	/* big total sectors */
	unsigned char junk[476];	/* who cares? */
};

#define	DPSECT(s) 	((s) & 0x3f)				/* isolate relevant bits of sector */
#define	DPCYL(c, s) ((c) + (((s) & 0xc0)<<2))	/* and those that are cylinder */

/*
 * Diskmbr-specific ioctls.
 */
#define DIOCSMBR 	_IOW('M', 129, u_char[512])
#endif /* !_SYS_DISKMBR_H_ */
