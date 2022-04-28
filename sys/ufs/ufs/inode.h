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
 *	@(#)inode.h	8.9 (Berkeley) 5/14/95
 */
#ifndef _UFS_UFS_INODE_H_
#define	_UFS_UFS_INODE_H_

#include <sys/queue.h>

#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>

/*
 * The inode is used to describe each active (or recently active) file in the
 * UFS filesystem. It is composed of two types of information. The first part
 * is the information that is needed only while the file is active (such as
 * the identity of the file and linkage to speed its lookup). The second part
 * is * the permanent meta-data associated with the file which is read in
 * from the permanent dinode from long term storage when the file becomes
 * active, and is put back when the file is no longer being used.
 */
struct inode {
	LIST_ENTRY(inode)  		i_hash;				/* Hash chain. */
	struct	vnode  			*i_vnode;			/* Vnode associated with this inode. */
	struct  ufsmount 		*i_ump; 			/* Mount point associated with this inode. */
	struct	vnode  			*i_devvp;			/* Vnode for block I/O. */
	u_int32_t 				i_flag;				/* flags, see below */
	dev_t	  				i_dev;				/* Device associated with the inode. */
	ino_t	  				i_number;			/* The identity of the inode. */

	union {										/* Associated filesystem. */
		struct	fs 			*fs;				/* FFS */
		struct	lfs 		*lfs;				/* LFS */
	} inode_u;
#define	i_fs				inode_u.fs
#define	i_lfs				inode_u.lfs

	struct	 dquot 			*i_dquot[MAXQUOTAS];/* Dquot structures. */
	u_quad_t 				i_modrev;			/* Revision level for NFS lease. */
	struct	 lockf 			*i_lockf;			/* Head of byte-level lock list. */
	struct	 lock 			i_lock;				/* Inode lock. */
	/*
	 * Side effects; used during directory lookup.
	 */
	int32_t	  				i_count;			/* Size of free slot in directory. */
	doff_t	  				i_endoff;			/* End of useful stuff in directory. */
	doff_t	  				i_diroff;			/* Offset in dir, where we found last entry. */
	doff_t	  				i_offset;			/* Offset of free space in directory. */
	ino_t	  				i_ino;				/* Inode number of found directory. */
	u_int32_t 				i_reclen;			/* Size of found directory entry. */
	int       				i_effnlink;  		/* i_nlink when I/O completes */

	/*
	 * Copies from the on-disk dinode itself.
	 *
	 * These fields are currently only used by FFS and LFS,
	 * do NOT use them with ext2fs.
	 */
	u_int16_t 				i_mode;				/* IFMT, permissions; see below. */
	int16_t   				i_nlink;			/* File link count. */
	u_int64_t 				i_size;				/* File byte count. */
	u_int32_t 				i_flags;			/* Status flags (chflags). */
	int32_t   				i_gen;				/* Generation number. */
	u_int32_t 				i_uid;				/* File owner. */
	u_int32_t 				i_gid;				/* File group. */

	/*
	 * The on-disk dinode itself.
	 */
	union {
		struct ufs1_dinode 	*ffs1_din;			/* 128 bytes of the on-disk dinode. */
		struct ufs2_dinode 	*ffs2_din;			/* 128 bytes of the on-disk dinode. */
	} i_din;

	//union {
	//	struct dirhash 		*dirhash; 			/* Hashing for large directories. */
//	} i_un;
//#define i_dirhash 			i_un.dirhash

};

#define	i_ffs1_atime		i_din.ffs1_din->di_atime
#define	i_ffs1_atimensec	i_din.ffs1_din->di_atimensec
#define	i_ffs1_blocks		i_din.ffs1_din->di_blocks
#define	i_ffs1_ctime		i_din.ffs1_din->di_ctime
#define	i_ffs1_ctimensec	i_din.ffs1_din->di_ctimensec
#define	i_ffs1_db			i_din.ffs1_din->di_db
#define	i_ffs1_flags		i_din.ffs1_din->di_flags
#define	i_ffs1_gen			i_din.ffs1_din->di_gen
#define	i_ffs1_gid			i_din.ffs1_din->di_gid
#define	i_ffs1_ib			i_din.ffs1_din->di_ib
#define	i_ffs1_mode			i_din.ffs1_din->di_mode
#define	i_ffs1_mtime		i_din.ffs1_din->di_mtime
#define	i_ffs1_mtimensec	i_din.ffs1_din->di_mtimensec
#define	i_ffs1_nlink		i_din.ffs1_din->di_nlink
#define	i_ffs1_rdev			i_din.ffs1_din->di_rdev
#define	i_ffs1_shortlink	i_din.ffs1_din->di_shortlink
#define	i_ffs1_size			i_din.ffs1_din->di_size
#define	i_ffs1_uid			i_din.ffs1_din->di_uid

#define	i_ffs2_atime		i_din.ffs2_din->di_atime
#define	i_ffs2_atimensec	i_din.ffs2_din->di_atimensec
#define	i_ffs2_birthtime	i_din.ffs2_din->di_birthtime
#define	i_ffs2_birthnsec	i_din.ffs2_din->di_birthnsec
#define	i_ffs2_blocks		i_din.ffs2_din->di_blocks
#define	i_ffs2_blksize		i_din.ffs2_din->di_blksize
#define	i_ffs2_ctime		i_din.ffs2_din->di_ctime
#define	i_ffs2_ctimensec	i_din.ffs2_din->di_ctimensec
#define	i_ffs2_db			i_din.ffs2_din->di_db
#define	i_ffs2_flags		i_din.ffs2_din->di_flags
#define	i_ffs2_gen			i_din.ffs2_din->di_gen
#define	i_ffs2_gid			i_din.ffs2_din->di_gid
#define	i_ffs2_ib			i_din.ffs2_din->di_ib
#define	i_ffs2_mode			i_din.ffs2_din->di_mode
#define	i_ffs2_mtime		i_din.ffs2_din->di_mtime
#define	i_ffs2_mtimensec	i_din.ffs2_din->di_mtimensec
#define	i_ffs2_nlink		i_din.ffs2_din->di_nlink
#define	i_ffs2_rdev			i_din.ffs2_din->di_rdev
#define	i_ffs2_size			i_din.ffs2_din->di_size
#define	i_ffs2_uid			i_din.ffs2_din->di_uid
#define	i_ffs2_kernflags	i_din.ffs2_din->di_kernflags
#define	i_ffs2_extsize		i_din.ffs2_din->di_extsize
#define	i_ffs2_extb			i_din.ffs2_din->di_extb

/* These flags are kept in i_flag. */
#define	IN_ACCESS			0x0001		/* Access time update request. */
#define	IN_CHANGE			0x0002		/* Inode change time update request. */
#define	IN_UPDATE			0x0004		/* Modification time update request. */
#define	IN_MODIFIED			0x0008		/* Inode has been modified. */
#define	IN_RENAME			0x0010		/* Inode is being renamed. */
#define	IN_SHLOCK			0x0020		/* File has shared lock. */
#define	IN_EXLOCK			0x0040		/* File has exclusive lock. */
#define	IN_HASHED			0x0080		/* Inode is on hash list */
#define	IN_LAZYMOD			0x0100		/* Modified, but don't write yet. */
#define IN_NOCOPYWRITE		0x0200		/* Special NOCOPY write */
#define	IN_UFS2				0x0400		/* UFS2 vs UFS1 */

#ifdef _KERNEL

static inline bool_t
I_IS_UFS1(const struct inode *ip)
{

	return ((ip->i_flag & IN_UFS2) == 0);
}

static inline bool_t
I_IS_UFS2(const struct inode *ip)
{

	return ((ip->i_flag & IN_UFS2) != 0);
}

/*
 * The DIP macro is used to access fields in the dinode that are
 * not cached in the inode itself.
 */
#define	DIP(ip, field)	(I_IS_UFS1(ip) ? 						\
		(ip)->i_din.ffs1_din->di_##field : 						\
		(ip)->i_din.ffs2_din->di_##field)

#define	DIP_SET(ip, field, val) do {							\
	if (I_IS_UFS1(ip)) {										\
		(ip)->i_din.ffs1_din->di_##field = (val); 				\
	} else {													\
		(ip)->i_din.ffs2_din->di_##field = (val); 				\
	}															\
} while (0)

#define	SHORTLINK(ip) (I_IS_UFS1(ip) ? 							\
		(caddr_t)(ip)->i_din.ffs1_din->di_db : 					\
		(caddr_t)(ip)->i_din.ffs2_din->di_db)

/*
 * Structure used to pass around logical block paths generated by
 * ufs_getlbns and used by truncate and bmap code.
 */
struct indir {
	int32_t		in_lbn;		/* Logical block number. */
	int			in_off;		/* Offset in buffer. */
	int			in_exists;	/* Flag if the block exists. */
};

/* Convert between inode pointers and vnode pointers. */
#define VTOI(vp)	((struct inode *)(vp)->v_data)
#define ITOV(ip)	((ip)->i_vnode)

#define	ITIMES(ip, t1, t2) {									\
	if ((ip)->i_flag & (IN_ACCESS | IN_CHANGE | IN_UPDATE)) {	\
		(ip)->i_flag |= IN_MODIFIED;							\
		if ((ip)->i_flag & IN_ACCESS)							\
			DIP_SET((ip), atime, (t1)->tv_sec);					\
		if ((ip)->i_flag & IN_UPDATE) {							\
			DIP_SET((ip), mtime, (t2)->tv_sec);					\
			ip->i_modrev++;										\
		}														\
		if ((ip)->i_flag & IN_CHANGE)							\
			DIP_SET((ip), ctime, time.tv_sec);					\
		(ip)->i_flag &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE);	\
	}															\
}

/* This overlays the fid structure (see mount.h). */
struct ufid {
	u_int16_t ufid_len;	/* Length of structure. */
	u_int16_t ufid_pad;	/* Force 32-bit alignment. */
	ino_t	  ufid_ino;	/* File number (ino). */
	int32_t	  ufid_gen;	/* Generation number. */
};
#endif /* KERNEL */
#endif /* !_UFS_UFS_INODE_H_ */
