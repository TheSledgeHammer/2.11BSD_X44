/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
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
 *	@(#)iso.h	8.6 (Berkeley) 5/10/95
 */

#define ISODCL(from, to) (to - from + 1)

struct iso_volume_descriptor {
	char type[ISODCL(1,1)]; /* 711 */
	char id[ISODCL(2,6)];
	char version[ISODCL(7,7)];
	char data[ISODCL(8,2048)];
};

/* volume descriptor types */
#define ISO_VD_PRIMARY 	1
#define ISO_VD_END 		255

#define ISO_STANDARD_ID "CD001"
#define ISO_ECMA_ID     "CDW01"

struct iso_primary_descriptor {
	char type					[ISODCL (  1,   1)]; /* 711 */
	char id						[ISODCL (  2,   6)];
	char version				[ISODCL (  7,   7)]; /* 711 */
	char unused1				[ISODCL (  8,   8)];
	char system_id				[ISODCL (  9,  40)]; /* achars */
	char volume_id				[ISODCL ( 41,  72)]; /* dchars */
	char unused2				[ISODCL ( 73,  80)];
	char volume_space_size		[ISODCL ( 81,  88)]; /* 733 */
	char unused3				[ISODCL ( 89, 120)];
	char volume_set_size		[ISODCL (121, 124)]; /* 723 */
	char volume_sequence_number	[ISODCL (125, 128)]; /* 723 */
	char logical_block_size		[ISODCL (129, 132)]; /* 723 */
	char path_table_size		[ISODCL (133, 140)]; /* 733 */
	char type_l_path_table		[ISODCL (141, 144)]; /* 731 */
	char opt_type_l_path_table	[ISODCL (145, 148)]; /* 731 */
	char type_m_path_table		[ISODCL (149, 152)]; /* 732 */
	char opt_type_m_path_table	[ISODCL (153, 156)]; /* 732 */
	char root_directory_record	[ISODCL (157, 190)]; /* 9.1 */
	char volume_set_id			[ISODCL (191, 318)]; /* dchars */
	char publisher_id			[ISODCL (319, 446)]; /* achars */
	char preparer_id			[ISODCL (447, 574)]; /* achars */
	char application_id			[ISODCL (575, 702)]; /* achars */
	char copyright_file_id		[ISODCL (703, 739)]; /* 7.5 dchars */
	char abstract_file_id		[ISODCL (740, 776)]; /* 7.5 dchars */
	char bibliographic_file_id	[ISODCL (777, 813)]; /* 7.5 dchars */
	char creation_date			[ISODCL (814, 830)]; /* 8.4.26.1 */
	char modification_date		[ISODCL (831, 847)]; /* 8.4.26.1 */
	char expiration_date		[ISODCL (848, 864)]; /* 8.4.26.1 */
	char effective_date			[ISODCL (865, 881)]; /* 8.4.26.1 */
	char file_structure_version	[ISODCL (882, 882)]; /* 711 */
	char unused4				[ISODCL (883, 883)];
	char application_data		[ISODCL (884, 1395)];
	char unused5				[ISODCL (1396, 2048)];
};
#define ISO_DEFAULT_BLOCK_SIZE		2048

struct iso_directory_record {
	char length					[ISODCL (1, 1)]; /* 711 */
	char ext_attr_length		[ISODCL (2, 2)]; /* 711 */
	u_char extent				[ISODCL (3, 10)]; /* 733 */
	u_char size					[ISODCL (11, 18)]; /* 733 */
	char date					[ISODCL (19, 25)]; /* 7 by 711 */
	char flags					[ISODCL (26, 26)];
	char file_unit_size			[ISODCL (27, 27)]; /* 711 */
	char interleave				[ISODCL (28, 28)]; /* 711 */
	char volume_sequence_number	[ISODCL (29, 32)]; /* 723 */
	char name_len				[ISODCL (33, 33)]; /* 711 */
	char name					[1];			/* XXX */
};
/* can't take sizeof(iso_directory_record), because of possible alignment
   of the last entry (34 instead of 33) */
#define ISO_DIRECTORY_RECORD_SIZE	33

struct iso_extended_attributes {
	u_char owner			[ISODCL (1, 4)]; /* 723 */
	u_char group			[ISODCL (5, 8)]; /* 723 */
	u_char perm				[ISODCL (9, 10)]; /* 9.5.3 */
	char ctime				[ISODCL (11, 27)]; /* 8.4.26.1 */
	char mtime				[ISODCL (28, 44)]; /* 8.4.26.1 */
	char xtime				[ISODCL (45, 61)]; /* 8.4.26.1 */
	char ftime				[ISODCL (62, 78)]; /* 8.4.26.1 */
	char recfmt				[ISODCL (79, 79)]; /* 711 */
	char recattr			[ISODCL (80, 80)]; /* 711 */
	u_char reclen			[ISODCL (81, 84)]; /* 723 */
	char system_id			[ISODCL (85, 116)]; /* achars */
	char system_use			[ISODCL (117, 180)];
	char version			[ISODCL (181, 181)]; /* 711 */
	char len_esc			[ISODCL (182, 182)]; /* 711 */
	char reserved			[ISODCL (183, 246)];
	u_char len_au			[ISODCL (247, 250)]; /* 723 */
};

/* CD-ROM Format type */
enum ISO_FTYPE  { ISO_FTYPE_DEFAULT, ISO_FTYPE_9660, ISO_FTYPE_RRIP, ISO_FTYPE_ECMA };

#ifndef	ISOFSMNT_ROOT
#define	ISOFSMNT_ROOT	0
#endif

struct iso_mnt {
	int im_flags;
	int im_joliet_level;

	struct mount *im_mountp;
	dev_t im_dev;
	struct vnode *im_devvp;

	int logical_block_size;
	int im_bshift;
	int im_bmask;

	int volume_space_size;
	struct netexport im_export;

	char root[ISODCL (157, 190)];
	int root_extent;
	int root_size;
	enum ISO_FTYPE  iso_ftype;

	int rr_skip;
	int rr_skip0;
};

#define VFSTOISOFS(mp)			((struct iso_mnt *)((mp)->mnt_data))

#define blkoff(imp, loc)		((loc) & (imp)->im_bmask)
#define lblktosize(imp, blk)	((blk) << (imp)->im_bshift)
#define lblkno(imp, loc)		((loc) >> (imp)->im_bshift)
#define blksize(imp, ip, lbn)	((imp)->logical_block_size)

int cd9660_mount(struct mount *, char *, caddr_t, struct nameidata *, struct proc *);
int cd9660_start(struct mount *, int, struct proc *);
int cd9660_unmount(struct mount *, int, struct proc *);
int cd9660_root(struct mount *, struct vnode **);
int cd9660_quotactl(struct mount *, int, uid_t, caddr_t, struct proc *);
int cd9660_statfs(struct mount *, struct statfs *, struct proc *);
int cd9660_sync(struct mount *, int, struct ucred *, struct proc *);
int cd9660_vget(struct mount *, ino_t, struct vnode **);
int cd9660_vget_internal(struct mount *, ino_t, struct vnode **, int, struct iso_directory_record *);
int cd9660_fhtovp(struct mount *, struct fid *, struct mbuf *, struct vnode **, int *, struct ucred **);
int cd9660_vptofh(struct vnode *, struct fid *);
int cd9660_init(struct vfsconf *);
int cd9660_sysctl(int *, u_int, void *, size_t *, void *, size_t, struct proc *);
int cd9660_mountroot(void);

extern struct vnodeops cd9660_vnodeops;
extern struct vnodeops cd9660_specops;
#ifdef FIFO
extern struct vnodeops cd9660_fifoops;
#endif

/* 7.1.1: unsigned char */
static __inline int
isonum_711(p)
	const u_char *p;
{
	return (p[0]);
}

/* 7.1.2: signed char */
static __inline int
isonum_712(p)
	const char *p;
{
	return ((signed char)p[0]);
}

/* 7.2.1: unsigned little-endian 16-bit value.  NOT USED IN KERNEL. */
static __inline uint16_t
isonum_721(p)
	const u_char *p;
{
	return (p[0] | p[1] << 8);
}

/* 7.2.2: unsigned big-endian 16-bit value.  NOT USED IN KERNEL. */
static __inline uint16_t
isonum_722(p)
	const u_char *p;
{
	return (p[1] | p[0] << 8);
}

static __inline uint16_t
isonum_723(p)
	const u_char *p;
{
	return (p[0] | p[1] << 8);
}

/* 7.3.1: unsigned little-endian 32-bit value.  NOT USED IN KERNEL. */
static __inline uint32_t
isonum_731(p)
	const u_char *p;
{
	return (p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24);
}

/* 7.3.2: unsigned big-endian 32-bit value.  NOT USED IN KERNEL. */
static __inline uint32_t
isonum_732(p)
	const u_char *p;
{
	return (p[3] | p[2] << 8 | p[1] << 16 | p[0] << 24);
}

/* 7.3.3: unsigned both-endian (little, then big) 32-bit value */
static __inline uint32_t
isonum_733(p)
	const u_char *p;
{
	return (p[0] |(p[1] << 8)|(p[2] << 16)|(p[3] << 24));
}

int isochar(const u_char *, const u_char *, int, u_char *);
int isofncmp(const u_char *, int, const u_char *, int, int);
void isofntrans(u_char *, int, u_char *, u_short *, int, int, int, int);
ino_t isodirino(struct iso_directory_record *, struct iso_mnt *);

/*
 * Associated files have a leading '='.
 */
#define	ASSOCCHAR	'='
