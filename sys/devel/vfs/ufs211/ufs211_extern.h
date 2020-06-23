/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)ufs211_extern.h	1.00
 */

typedef u_short	 ufs211_ino_t;
typedef long	 ufs211_daddr_t;
typedef long	 ufs211_off_t;
typedef short	 ufs211_dev_t;
typedef u_short	 ufs211_mode_t;
typedef	u_int	 ufs211_size_t;

int				 updlock;					/* lock for sync */
ufs211_daddr_t	 rablock;					/* block to be read ahead */

struct ufs211_args {
	struct mount 	    *ufs211_vfs;
	struct vnode 	    *ufs211_rootvp;	    /* block device mounted vnode */
};

#define	UFS211_DEV_BSIZE	1024
#define	UFS211_DEV_BSHIFT	10			/* log2(DEV_BSIZE) */
#define	UFS211_DEV_BMASK	0x3ffL		/* DEV_BSIZE - 1 */
#define	UFS211_MAXBSIZE		1024

/* i_flag */
#define	UFS211_ILOCKED	0x1			/* inode is locked */
#define	UFS211_IUPD		0x2			/* file has been modified */
#define	UFS211_IACC		0x4			/* inode access time to be updated */
#define	UFS211_IMOUNT	0x8			/* inode is mounted on */
#define	UFS211_IWANT	0x10		/* some process waiting on lock */
#define	UFS211_ITEXT	0x20		/* inode is pure text prototype */
#define	UFS211_ICHG		0x40		/* inode has been changed */
#define	UFS211_ISHLOCK	0x80		/* file has shared lock */
#define	UFS211_IEXLOCK	0x100		/* file has exclusive lock */
#define	UFS211_ILWAIT	0x200		/* someone waiting on file lock */
#define	UFS211_IMOD		0x400		/* inode has been modified */
#define	UFS211_IRENAME	0x800		/* inode is being renamed */
#define	UFS211_IPIPE	0x1000		/* inode is a pipe */
#define	UFS211_IRCOLL	0x2000		/* read select collision on pipe */
#define	UFS211_IWCOLL	0x4000		/* write select collision on pipe */
#define	UFS211_IXMOD	0x8000		/* inode is text, but impure (XXX) */

/* i_mode */
#define	UFS211_IFMT		0170000		/* type of file */
#define	UFS211_IFCHR	0020000		/* character special */
#define	UFS211_IFDIR	0040000		/* directory */
#define	UFS211_IFBLK	0060000		/* block special */
#define	UFS211_IFREG	0100000		/* regular */
#define	UFS211_IFLNK	0120000		/* symbolic link */
#define	UFS211_IFSOCK	0140000		/* socket */
#define	UFS211_ISUID	04000		/* set user id on execution */
#define	UFS211_ISGID	02000		/* set group id on execution */
#define	UFS211_ISVTX	01000		/* save swapped text even after use */
#define	UFS211_IREAD	0400		/* read, write, execute permissions */
#define	UFS211_IWRITE	0200
#define	UFS211_IEXEC	0100

__BEGIN_DECLS
int ufs211_lookup (struct vop_lookup_args *);
int ufs211_create (struct vop_create_args *);
int ufs211_open (struct vop_open_args *);
int ufs211_close (struct vop_close_args *);
int ufs211_access (struct vop_access_args *);
int ufs211_getattr (struct vop_getattr_args *);
int ufs211_setattr (struct vop_setattr_args *);
int ufs211_read (struct vop_read_args *);
int ufs211_write (struct vop_write_args *);
int ufs211_fsync (struct vop_fsync_args *);
int ufs211_remove (struct vop_remove_args *);
int ufs211_rename (struct vop_rename_args *);
int ufs211_readdir (struct vop_readdir_args *);
int ufs211_inactive (struct vop_inactive_args *);
int ufs211_reclaim (struct vop_reclaim_args *);
int ufs211_bmap (struct vop_bmap_args *);
int ufs211_strategy (struct vop_strategy_args *);
int ufs211_print (struct vop_print_args *);
int ufs211_advlock (struct vop_advlock_args *);
int ufs211_pathconf (struct vop_pathconf_args *);
int ufs211_link (struct vop_link_args *);
int ufs211_symlink (struct vop_symlink_args *);
int ufs211_readlink (struct vop_readlink_args *);
int ufs211_mkdir (struct vop_mkdir_args *);
int ufs211_rmdir (struct vop_rmdir_args *);
int ufs211_mknod (struct vop_mknod_args *);
int	ufs211spec_close (struct vop_close_args *);
int	ufs211spec_read (struct vop_read_args *);
int ufs211spec_write (struct vop_write_args *);
#ifdef FIFO
int	ufs211fifo_read (struct vop_read_args *);
int	ufs211fifo_write (struct vop_write_args *);
int	ufs211fifo_close (struct vop_close_args *);
#endif
__END_DECLS
