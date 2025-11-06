/*
 * Copyright (c) 1982, 1989, 1993
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
 *	@(#)dinode.h	8.9 (Berkeley) 3/29/95
 */

#ifndef _UFS_UFS_DINODE_H_
#define	_UFS_UFS_DINODE_H_

/*
 * The root inode is the root of the file system.  Inode 0 can't be used for
 * normal purposes and historically bad blocks were linked to inode 1, thus
 * the root inode is 2.  (Inode 1 is no longer used for this purpose, however
 * numerous dump tapes make this assumption, so we are stuck with it).
 */
#define	ROOTINO		((ino_t)2)
#define UFS_ROOTINO	ROOTINO

/*
 * The Whiteout inode# is a dummy non-zero inode number which will
 * never be allocated to a real file.  It is used as a place holder
 * in the directory entry which has been tagged as a DT_W entry.
 * See the comments about ROOTINO above.
 */
#define	WINO		((ino_t)1)
#define UFS_WINO	WINO
/*
 * A dinode contains all the meta-data associated with a UFS file.
 * This structure defines the on-disk format of a dinode. Since
 * this structure describes an on-disk structure, all its fields
 * are defined by types with precise widths.
 */
typedef uint32_t 	ufs_ino_t;
typedef	int32_t		ufs1_daddr_t;
typedef	int64_t		ufs2_daddr_t;
typedef int64_t 	ufs_lbn_t;
typedef int64_t 	ufs_time_t;

#define	NXADDR		2				/* External addresses in inode. */
#define	NDADDR		12				/* Direct addresses in inode. */
#define	NIADDR		3				/* Indirect addresses in inode. */
#define UFS_NDADDR	 NDADDR
#define UFS_NIADDR	 NIADDR

struct ufs1_dinode {
	uint16_t		di_mode;		/*   0: IFMT, permissions; see below. */
	int16_t			di_nlink;		/*   2: File link count. */
	union {
		uint16_t 	oldids[2];		/*   4: FFS: old user and group ids. */
		int32_t	  	inumber;		/*   4: LFS: inode number. 32-Bit */
	} di_u;
	uint64_t		di_size;		/*   8: File byte count. */
	int32_t			di_atime;		/*  16: Last access time. */
	int32_t			di_atimensec;	/*  20: Last access time. */
	int32_t			di_mtime;		/*  24: Last modified time. */
	int32_t			di_mtimensec;	/*  28: Last modified time. */
	int32_t			di_ctime;		/*  32: Last inode change time. */
	int32_t			di_ctimensec;	/*  36: Last inode change time. */
	int32_t			di_db[NDADDR];	/*  40: Direct disk blocks. */
	int32_t			di_ib[NIADDR];	/*  88: Indirect disk blocks. */
	uint32_t		di_flags;		/* 100: Status flags (chflags). */
	uint32_t		di_blocks;		/* 104: Blocks actually held. */
	int32_t			di_gen;			/* 108: Generation number. */
	uint32_t		di_uid;			/* 112: File owner. */
	uint32_t		di_gid;			/* 116: File group. */
	int32_t			di_spare[2];	/* 120: Reserved; currently unused */
};

struct ufs2_dinode {
	uint16_t	    di_mode;	    /*   0: IFMT, permissions; see below. */
	int16_t		    di_nlink;	    /*   2: File link count. */
	union {
		int64_t	  	inumber;		/*   4: LFS: inode number. 64-Bit */
	} di_u;
	uint32_t	    di_uid;		    /*   4: File owner. */
	uint32_t	    di_gid;		    /*   8: File group. */
	uint32_t	    di_blksize;	    /*  12: Inode blocksize. */
	uint64_t	    di_size;	    /*  16: File byte count. */
	uint64_t	    di_blocks;	    /*  24: Bytes actually held. */
	int64_t		    di_atime;	    /*  32: Last access time. */
	int64_t		    di_mtime;	    /*  40: Last modified time. */
	int64_t		    di_ctime;	    /*  48: Last inode change time. */
	int64_t		    di_birthtime;	/*  56: Inode creation time. */
	int32_t		    di_mtimensec;	/*  64: Last modified time. */
	int32_t		    di_atimensec;	/*  68: Last access time. */
	int32_t		    di_ctimensec;	/*  72: Last inode change time. */
	int32_t		    di_birthnsec;	/*  76: Inode creation time. */
	int32_t		    di_gen;		    /*  80: Generation number. */
	uint32_t	    di_kernflags;	/*  84: Kernel flags. */
	uint32_t	    di_flags;	    /*  88: Status flags (chflags). */
	int32_t		    di_extsize;	    /*  92: External attributes block. */
	ufs2_daddr_t    di_extb[NXADDR];/* 96: External attributes block. */
	ufs2_daddr_t    di_db[NDADDR];  /* 112: Direct disk blocks. */
	ufs2_daddr_t    di_ib[NIADDR];  /* 208: Indirect disk blocks. */
	uint64_t	    di_modrev;	    /* 232: i_modrev for NFSv4 */
	int64_t		    di_spare[2];	/* 240: Reserved; currently unused */
};

/*
 * The di_db fields may be overlaid with other information for
 * file types that do not have associated disk storage. Block
 * and character devices overlay the first data block with their
 * dev_t value. Short symbolic links place their path in the
 * di_db area.
 */
#define	di_inumber			di_u.inumber
#define	di_ogid				di_u.oldids[1]
#define	di_ouid				di_u.oldids[0]
#define	di_rdev				di_db[0]
#define	di_shortlink		di_db
#define UFS1_MAXSYMLINKLEN	((NDADDR + NIADDR) * sizeof(ufs1_daddr_t))
#define UFS2_MAXSYMLINKLEN	((NDADDR + NIADDR) * sizeof(ufs2_daddr_t))

#define MAXSYMLINKLEN(ip) 		\
	((ip)->i_ump->um_fstype == UFS1) ? UFS1_MAXSYMLINKLEN : UFS2_MAXSYMLINKLEN

/* File permissions. */
#define	IEXEC		0000100		/* Executable. */
#define	IWRITE		0000200		/* Writeable. */
#define	IREAD		0000400		/* Readable. */
#define	ISVTX		0001000		/* Sticky bit. */
#define	ISGID		0002000		/* Set-gid. */
#define	ISUID		0004000		/* Set-uid. */

/* File types. */
#define	IFMT		0170000		/* Mask of file type. */
#define	IFIFO		0010000		/* Named pipe (fifo). */
#define	IFCHR		0020000		/* Character device. */
#define	IFDIR		0040000		/* Directory file. */
#define	IFBLK		0060000		/* Block device. */
#define	IFREG		0100000		/* Regular file. */
#define	IFLNK		0120000		/* Symbolic link. */
#define	IFSOCK		0140000		/* UNIX domain socket. */
#define	IFWHT		0160000		/* Whiteout. */

#endif /* !_UFS_UFS_DINODE_H_ */
