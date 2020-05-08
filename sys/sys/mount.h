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
#include <sys/lock.h>
#include <net/radix.h>
#include <sys/socket.h>		/* XXX for AF_MAX */

typedef struct { long val[2]; } fsid_t;	/* file system id type */
/*
 * File identifier.
 * These are unique per filesystem on a single machine.
 */
#define	MAXFIDSZ	16

struct fid {
	u_short		fid_len;			/* length of data in bytes */
	u_short		fid_reserved;		/* force longword alignment */
	char		fid_data[MAXFIDSZ];	/* data (variable length) */
};

/*
 * file system statistics
 */
#define MFSNAMELEN	16	/* length of fs type name, including null */
#define MNAMELEN 	90	/* length of buffer for returned name */

struct statfs {
	short	f_type;						/* type of filesystem (see below) */
	short	f_flags;					/* copy of mount flags */
	long	f_bsize;					/* fundamental file system block size */
	long	f_iosize;					/* optimal transfer block size */
	long	f_blocks;					/* total data blocks in file system */
	long	f_bfree;					/* free blocks in fs */
	long	f_bavail;					/* free blocks avail to non-superuser */
	long	f_files;					/* total file nodes in file system */
	long	f_ffree;					/* free file nodes in fs */
	fsid_t	f_fsid;						/* file system id */
	uid_t	f_owner;					/* user that mounted the filesystem */
	long	f_spare[9];					/* spare for later */
	char	f_fstypename[MFSNAMELEN]; 	/* fs type name */
	char	f_mntonname[MNAMELEN];		/* directory on which mounted */
	char	f_mntfromname[MNAMELEN];	/* mounted filesystem */
};

/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
LIST_HEAD(vnodelst, vnode);

struct	mount {
	CIRCLEQ_ENTRY(mount) mnt_list;			/* mount list */
	struct vfsops		*mnt_op;			/* operations on fs */
	struct vfsconf		*mnt_vfc;			/* configuration info */
	struct vnode		*mnt_vnodecovered;	/* vnode we mounted on */
	struct vnodelst		mnt_vnodelist;		/* list of vnodes this mount */
	struct lock			mnt_lock;			/* mount structure lock */
	int					mnt_flag;			/* flags */
	int					mnt_maxsymlinklen;	/* max size of short symlink */
	struct statfs		mnt_stat;			/* cache of filesystem stats */
	qaddr_t				mnt_data;			/* private data */

	dev_t				m_dev;				/* device mounted */
	memaddr				m_extern;			/* click address of mount table extension */
};

struct	xmount {
	char				xm_mntfrom[MNAMELEN];	/* /dev/xxxx mounted from */
	char				xm_mnton[MNAMELEN];		/* directory mounted on - this is the full(er) version of fs_fsmnt. */
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
#define	MNT_UNION		0x0100		/* union with underlying filesystem */

/*
 * Mask of flags that are visible to statfs().
*/
#define	MNT_VISFLAGMASK	0x0fff

/*
 * filesystem control flags.  The high 4 bits are used for this.  Since NFS
 * support will never be a problem we can avoid making the flags into a 'long.
*/
#define	MNT_UPDATE		0x1000		/* not a real mount, just an update */
#define	MNT_DELEXPORT	0x00020000	/* delete export host lists */
#define	MNT_RELOAD		0x00040000	/* reload filesystem data */
#define	MNT_FORCE		0x00080000	/* force unmount or readonly change */

/*
 * Internal filesystem control flags.
 *
 * MNT_UNMOUNT locks the mount entry so that name lookup cannot proceed
 * past the mount point.  This keeps the subtree stable during mounts
 * and unmounts.
 */
#define MNT_UNMOUNT		0x01000000	/* unmount in progress */
#define	MNT_MWAIT		0x02000000	/* waiting for unmount to finish */
#define MNT_WANTRDWR	0x04000000	/* upgrade to read/write requested */

/*
 * Sysctl CTL_VFS definitions.
 *
 * Second level identifier specifies which filesystem. Second level
 * identifier VFS_GENERIC returns information about all filesystems.
 */
#define	VFS_GENERIC		0	/* generic filesystem information */

/*
 * used to get configured filesystems information
 */
#define VFS_MAXNAMELEN 32

/*
 * Third level identifiers for VFS_GENERIC are given below; third
 * level identifiers for specific filesystems are given in their
 * mount specific header files.
 */
#define VFS_MAXTYPENUM	1	/* int: highest defined filesystem type */
#define VFS_CONF		2	/* struct: vfsconf for filesystem given
				   	   	   	   as next argument */

/*
 * Flags for various system call interfaces.
 *
 * These aren't used for anything in the system and are present only
 * for source code compatibility reasons.
*/
#define	MNT_WAIT		1
#define	MNT_NOWAIT		2

/*
 * Generic file handle
 */
struct fhandle {
	fsid_t				fh_fsid;	/* File system id of mount point */
	struct	fid 		fh_fid;		/* File sys specific id */
};
typedef struct fhandle	fhandle_t;

/*
 * Export arguments for local filesystem mount calls.
 */
struct export_args {
	int					ex_flags;		/* export related flags */
	uid_t				ex_root;		/* mapping for root uid */
	struct	ucred 		ex_anon;		/* mapping for anonymous user */
	struct	sockaddr 	*ex_addr;		/* net address to which exported */
	int					ex_addrlen;		/* and the net address length */
	struct	sockaddr 	*ex_mask;		/* mask of valid bits in saddr */
	int					ex_masklen;		/* and the smask length */
};

/*
 * Filesystem configuration information. One of these exists for each
 * type of filesystem supported by the kernel. These are searched at
 * mount time to identify the requested filesystem.
 */
struct vfsconf {
	void 			*vfc_vfsops;				/* filesystem operations vector */
	char 			vfc_name[VFS_MAXNAMELEN];	/* filesystem type name */
	int 			vfc_index;
	int				vfc_typenum;				/* historic filesystem type number */
	int 			vfc_refcount;				/* number mounted of this type */
	int 			vfc_flags;					/* permanent flags */
	int				(*vfc_mountroot)(void);		/* if != NULL, routine to mount root */
	struct	vfsconf *vfc_next;					/* next in list */
};

#ifdef KERNEL

extern int 				maxvfsconf;		/* highest defined filesystem type */
extern struct vfsconf 	*vfsconf;		/* head of list of filesystem types */

/*
 * Operations supported on mounted file system.
 */
#ifdef __STDC__
struct nameidata;
struct mbuf;
#endif

struct vfsops {
	int	(*vfs_mount) (struct mount *mp, char *path, caddr_t data, struct nameidata *ndp, struct proc *p);
	int	(*vfs_start) (struct mount *mp, int flags, struct proc *p);
	int	(*vfs_unmount) (struct mount *mp, int mntflags, struct proc *p);
	int	(*vfs_root) (struct mount *mp, struct vnode **vpp);
	int	(*vfs_quotactl) (struct mount *mp, int cmds, uid_t uid, caddr_t arg, struct proc *p);
	int	(*vfs_statfs) (struct mount *mp, struct statfs *sbp, struct proc *p);
	int	(*vfs_sync)	(struct mount *mp, int waitfor, struct ucred *cred, struct proc *p);
	int	(*vfs_vget) (struct mount *mp, ino_t ino, struct vnode **vpp);
	int	(*vfs_fhtovp) (struct mount *mp, struct fid *fhp, struct mbuf *nam, struct vnode **vpp, int *exflagsp, struct ucred **credanonp);
	int	(*vfs_vptofh) (struct vnode *vp, struct fid *fhp);
	int	(*vfs_init) (struct vfsconf *);
	int	(*vfs_sysctl) (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
};

#define VFS_MOUNT(MP, PATH, DATA, NDP, P) \
	(*(MP)->mnt_op->vfs_mount)(MP, PATH, DATA, NDP, P)
#define VFS_START(MP, FLAGS, P)	  (*(MP)->mnt_op->vfs_start)(MP, FLAGS, P)
#define VFS_UNMOUNT(MP, FORCE, P) (*(MP)->mnt_op->vfs_unmount)(MP, FORCE, P)
#define VFS_ROOT(MP, VPP)	  (*(MP)->mnt_op->vfs_root)(MP, VPP)
#define VFS_QUOTACTL(MP,C,U,A,P)  (*(MP)->mnt_op->vfs_quotactl)(MP, C, U, A, P)
#define VFS_STATFS(MP, SBP, P)	  (*(MP)->mnt_op->vfs_statfs)(MP, SBP, P)
#define VFS_SYNC(MP, WAIT, C, P)  (*(MP)->mnt_op->vfs_sync)(MP, WAIT, C, P)
#define VFS_VGET(MP, INO, VPP)	  (*(MP)->mnt_op->vfs_vget)(MP, INO, VPP)
#define VFS_FHTOVP(MP, FIDP, NAM, VPP, EXFLG, CRED) \
	(*(MP)->mnt_op->vfs_fhtovp)(MP, FIDP, NAM, VPP, EXFLG, CRED)
#define	VFS_VPTOFH(VP, FIDP)	  (*(VP)->v_mount->mnt_op->vfs_vptofh)(VP, FIDP)

/*
 * Network address lookup element
 */
struct netcred {
	struct	radix_node 	netc_rnodes[2];
	int					netc_exflags;
	struct	ucred 		netc_anon;
};

/*
 * Network export information
 */
struct netexport {
	//struct	netcred ne_defexported;		      		/* Default export */
	struct	radix_node_head *ne_rtable[AF_MAX+1]; 	/* Individual exports */
};

/*
 * exported vnode operations
 */
int		dounmount (struct mount *, int, struct proc *);
struct	mount *getvfs (fsid_t *);      													/* return vfs given fsid */
int		vflush (struct mount *mp, struct vnode *skipvp, int flags);
int		vfs_export (struct mount *, struct netexport *, struct export_args *); 			/* process mount export info */
struct	netcred *vfs_export_lookup (struct mount *, struct netexport *, struct mbuf *); /* lookup host in fs export list */
int		vfs_lock (struct mount *);         												/* lock a vfs */
int		vfs_mountedon (struct vnode *);    												/* is a vfs mounted on vp */
void	vfs_unlock (struct mount *);       												/* unlock a vfs */
int		vfs_busy (struct mount *);         												/* mark a vfs  busy */
void	vfs_unbusy (struct mount *);       												/* mark a vfs not busy */
int		vfs_rootmountalloc (char *, char *, struct mount **);
int		vfs_mountroot (void);
void	vfs_getnewfsid (struct mount *);
void	vfs_unmountall (void);
extern	CIRCLEQ_HEAD(mntlist, mount) mountlist;											/* mounted filesystem list */
extern	struct simplelock mountlist_slock;
extern	struct vfsops *vfssw[];															/* filesystem type table */

#else /* KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
int		fstatfs (int, struct statfs *);
int		getfh (const char *, fhandle_t *);
int		getfsstat (struct statfs *, long, int);
int		getmntinfo (struct statfs **, int);
int		mount (int, const char *, int, void *);
int		statfs (const char *, struct statfs *);
int		unmount (const char *, int);
__END_DECLS

#endif /* KERNEL */
