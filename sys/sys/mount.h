/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mount.h	7.2.5 (2.11BSD GTE) 1997/6/29
 */

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#ifndef KERNEL
#include <sys/ucred.h>
#endif
#include <sys/queue.h>

typedef struct { long val[2]; } fsid_t;	/* file system id type */
/*
 * File identifier.
 * These are unique per filesystem on a single machine.
 */
#define	MAXFIDSZ	16

struct fid {
	u_short		fid_len;		/* length of data in bytes */
	u_short		fid_reserved;		/* force longword alignment */
	char		fid_data[MAXFIDSZ];	/* data (variable length) */
};

/*
 * file system statistics
 */

#define MNAMELEN 	90	/* length of buffer for returned name */

struct statfs {
	short	f_type;					/* type of filesystem (see below) */
	short	f_flags;				/* copy of mount flags */
	long	f_bsize;				/* fundamental file system block size */
	long	f_iosize;				/* optimal transfer block size */
	long	f_blocks;				/* total data blocks in file system */
	long	f_bfree;				/* free blocks in fs */
	long	f_bavail;				/* free blocks avail to non-superuser */
	long	f_files;				/* total file nodes in file system */
	long	f_ffree;				/* free file nodes in fs */
	fsid_t	f_fsid;					/* file system id */
	long	f_spare[9];				/* spare for later */
	char	f_mntonname[MNAMELEN];	/* directory on which mounted */
	char	f_mntfromname[MNAMELEN];/* mounted filesystem */
};

/*
 * File system types.  Since only UFS is supported the others are not
 * specified at this time.
 */
#define	MOUNT_NONE		0
#define	MOUNT_UFS		1	/* Fast Filesystem */
#define	MOUNT_MAXTYPE	1

#define	INITMOUNTNAMES { \
	"none",		/* 0 MOUNT_NONE */ \
	"ufs",		/* 1 MOUNT_UFS */ \
	0,				  \
}


/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
LIST_HEAD(vnodelst, vnode);
struct	mount
{
	TAILQ_ENTRY(mount) mnt_list;		/* mount list */
	struct vfsops	*mnt_op;			/* operations on fs */
	struct vfsconf	*mnt_vfc;			/* configuration info */
	struct vnode	*mnt_vnodecovered;	/* vnode we mounted on */
	struct vnodelst	mnt_vnodelist;		/* list of vnodes this mount */
	struct lock		mnt_lock;			/* mount structure lock */
	int				mnt_flag;			/* flags */
	int				mnt_maxsymlinklen;	/* max size of short symlink */
	struct statfs	mnt_stat;			/* cache of filesystem stats */
	qaddr_t			mnt_data;			/* private data */
	struct vfsconf	*mnt_vfc; 			/* configuration info */

	dev_t			m_dev;				/* device mounted */
	struct	fs 		m_filsys;			/* superblock data */
#define	m_flags	m_filsys.fs_flags
	struct	inode 	*m_inodp;			/* pointer to mounted on inode */
	struct	inode 	*m_qinod; 			/* QUOTA: pointer to quota file */
	memaddr	m_extern;					/* click address of mount table extension */

	const struct wapbl_ops 	*mnt_wapbl_op;		/* logging ops */
	struct wapbl			*mnt_wapbl;			/* log info */
	struct wapbl_replay		*mnt_wapbl_replay;	/* replay support XXX: what? */
};

struct	xmount
	{
	char	xm_mntfrom[MNAMELEN];		/* /dev/xxxx mounted from */
	char	xm_mnton[MNAMELEN];			/* directory mounted on - this is the
					 * full(er) version of fs_fsmnt.
					*/
	};

#define	XMOUNTDESC	(((btoc(sizeof (struct xmount)) - 1) << 8) | RW)

/*
 * Mount flags.
 */
#define	MNT_RDONLY		0x0001		/* read only filesystem */
#define	MNT_SYNCHRONOUS	0x0002		/* file system written synchronously */
#define	MNT_NOEXEC		0x0004		/* can't exec from filesystem */
#define	MNT_NOSUID		0x0008		/* don't honor setuid bits on fs */
#define	MNT_NODEV		0x0010		/* don't interpret special files */
#define	MNT_QUOTA		0x0020		/* quotas are enabled on filesystem */
#define	MNT_ASYNC		0x0040		/* file system written asynchronously */
#define	MNT_NOATIME		0x0080		/* don't update access times */

/*
 * Mask of flags that are visible to statfs().
*/
#define	MNT_VISFLAGMASK	0x0fff

/*
 * filesystem control flags.  The high 4 bits are used for this.  Since NFS
 * support will never be a problem we can avoid making the flags into a 'long.
*/
#define	MNT_UPDATE	0x1000		/* not a real mount, just an update */


/*
 * used to get configured filesystems information
 */
#define VFS_MAXNAMELEN 32
struct vfsconf {
	void *vfc_vfsops;
	char vfc_name[VFS_MAXNAMELEN];
	int vfc_index;
	int vfc_refcount;
	int vfc_flags;
};


/*
 * Flags for various system call interfaces.
 * 
 * These aren't used for anything in the system and are present only
 * for source code compatibility reasons.
*/
#define	MNT_WAIT	1
#define	MNT_NOWAIT	2

/*
 * Generic file handle
 */
struct fhandle {
	fsid_t	fh_fsid;	/* File system id of mount point */
	struct	fid fh_fid;	/* File sys specific id */
};
typedef struct fhandle	fhandle_t;


#ifdef KERNEL
/*
 * exported vnode operations
 */
int		dounmount (struct mount *, int, struct proc *);
struct	mount *getvfs (fsid_t *);      	/* return vfs given fsid */
int		vflush (struct mount *mp, struct vnode *skipvp, int flags);
int		vfs_export			    				/* process mount export info */
	  	(struct mount *, struct netexport *, struct export_args *);
struct	netcred *vfs_export_lookup	    		/* lookup host in fs export list */
	  	(struct mount *, struct netexport *, struct mbuf *);
int		vfs_lock (struct mount *);         /* lock a vfs */
int		vfs_mountedon (struct vnode *);    /* is a vfs mounted on vp */
void	vfs_unlock (struct mount *);       /* unlock a vfs */
int		vfs_busy (struct mount *);         /* mark a vfs  busy */
void	vfs_unbusy (struct mount *);       /* mark a vfs not busy */
extern	TAILQ_HEAD(mntlist, mount) mountlist;	/* mounted filesystem list */
extern	struct vfsops *vfssw[];					/* filesystem type table */

#else /* KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
int	fstatfs (int, struct statfs *);
int	getfh (const char *, fhandle_t *);
int	getfsstat (struct statfs *, long, int);
int	getmntinfo (struct statfs **, int);
int	mount (int, const char *, int, void *);
int	statfs (const char *, struct statfs *);
int	unmount (const char *, int);
__END_DECLS

#endif /* KERNEL */

#endif
