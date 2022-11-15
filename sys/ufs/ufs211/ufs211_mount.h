/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mount.h	7.2.5 (2.11BSD GTE) 1997/6/29
 */

#ifndef _UFS211_MOUNT_H_
#define	_UFS211_MOUNT_H_

#define UFS211_MNAMELEN 	90				    			/* length of buffer for returned name */

struct ufs211_statfs {
	short					f_type;							/* type of filesystem (see below) */
	u_short					f_flags;						/* copy of mount flags */
	short					f_bsize;						/* fundamental file system block size */
	short					f_iosize;						/* optimal transfer block size */
	daddr_t					f_blocks;						/* total data blocks in file system */
	daddr_t					f_bfree;						/* free blocks in fs */
	daddr_t					f_bavail;						/* free blocks avail to non-superuser */
	ino_t					f_files;						/* total file nodes in file system */
	ino_t					f_ffree;						/* free file nodes in fs */
	long					f_fsid[2];						/* file system id */
	long					f_spare[5];						/* spare for later */
	char					f_mntonname[UFS211_MNAMELEN];	/* directory on which mounted */
	char					f_mntfromname[UFS211_MNAMELEN];	/* mounted filesystem */
};

struct ufs211_mount {
	struct	mount 			*m_mountp;						/* filesystem vfs structure */
	dev_t	    			m_dev;		    				/* device mounted */
	struct	vnode 			*m_devvp;						/* block device mounted vnode */
	struct	ufs211_fs 		*m_filsys;	        			/* superblock data */
#define	m_flags		    	m_filsys->fs_flags
	struct	ufs211_inode 	*m_inodp;	        			/* pointer to mounted on inode */
	struct	ufs211_inode 	*m_qinod; 	    				/* QUOTA: pointer to quota file */
	ufs211_size_t	    	m_extern;	        			/* click address of mount table extension */
	struct buf				*m_bufp;

	struct vnode 			*m_quotas[MAXQUOTAS];			/* pointer to quota files */
	struct ucred 			*m_cred[MAXQUOTAS];				/* quota file access cred */
	time_t					m_bwarn[MAXQUOTAS];				/* block quota time limit */
	time_t					m_iwarn[MAXQUOTAS];				/* inode quota time limit */
	char					m_qflags[MAXQUOTAS];			/* quota specific flags */
};

struct ufs211_xmount {
	char					xm_mntfrom[UFS211_MNAMELEN];	/* /dev/xxxx mounted from */
	char					xm_mnton[UFS211_MNAMELEN];		/* directory mounted on - this is the full(er) version of fs_fsmnt. */
};

#define	XMOUNTDESC		(((btoc(sizeof (struct ufs211_xmount)) - 1) << 8) | RW)

/* Convert mount ptr to ufs211 mount ptr. */
#define VFSTOUFS211(mp)	((struct ufs211_mount *)((mp)->mnt_data))
#endif
