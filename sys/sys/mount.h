/*
 * Copyright (c) 1989, 1991, 1993
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
 *	@(#)mount.h	8.21 (Berkeley) 5/20/95
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mount.h	7.2.5 (2.11BSD GTE) 1997/6/29
 */

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#ifndef _KERNEL
#include <sys/ucred.h>
#endif
#include <sys/queue.h>
#include <sys/lock.h>
#include <net/radix.h>
#include <sys/socket.h>		/* XXX for AF_MAX */

typedef struct {
	long val[2];
} fsid_t;					/* file system id type */

/*
 * File identifier.
 * These are unique per filesystem on a single machine.
 */
#define	MAXFIDSZ			16

struct fid {
	u_short					fid_len;			/* length of data in bytes */
	u_short					fid_reserved;		/* force longword alignment */
	char					fid_data[MAXFIDSZ];	/* data (variable length) */
};

/*
 * file system statistics
 */
#define MFSNAMELEN	16	/* length of fs type name, including null */
#define MNAMELEN 	90	/* length of buffer for returned name */

struct statfs {
	short					f_type;						/* type of filesystem (see below) */
	short					f_flags;					/* copy of mount flags */
	long					f_bsize;					/* fundamental file system block size */
	long					f_iosize;					/* optimal transfer block size */
	long					f_blocks;					/* total data blocks in file system */
	long					f_bfree;					/* free blocks in fs */
	long					f_bavail;					/* free blocks avail to non-superuser */
	long					f_files;					/* total file nodes in file system */
	long					f_ffree;					/* free file nodes in fs */
	fsid_t					f_fsid;						/* file system id */
	uid_t					f_owner;					/* user that mounted the filesystem */
	long					f_spare[9];					/* spare for later */
	char					f_fstypename[MFSNAMELEN]; 	/* fs type name */
	char					f_mntonname[MNAMELEN];		/* directory on which mounted */
	char					f_mntfromname[MNAMELEN];	/* mounted filesystem */
};

/*
 * File system types.
 */
/*
 * Note: If you add more file system types, please
 * append the MOUNT_MAXTYPE to reflect the new number
 * of mountable file system types.
 */
#define	MOUNT_NONE	"none"
#define	MOUNT_FFS	"ffs"		/* UNIX "Fast" Filesystem */
#define	MOUNT_UFS	"ufs"		/* for compatibility */
#define	MOUNT_NFS	"nfs"		/* Network Filesystem */
#define	MOUNT_MFS	"mfs"		/* Memory Filesystem */
#define	MOUNT_MSDOS	"msdos"		/* MSDOS Filesystem */
#define	MOUNT_LFS	"lfs"		/* Log-based Filesystem */
#define	MOUNT_FDESC	"fdesc"		/* File Descriptor Filesystem */
#define	MOUNT_CD9660	"cd9660"	/* ISO9660 (aka CDROM) Filesystem */
#define	MOUNT_LOFS	"lofs"		/* Loopback Filesystem */
#define	MOUNT_UNION	"union"		/* Union (translucent) Filesystem */
#define	MOUNT_UFS211	"ufs211"	/* 2.11BSD UFS Filesystem */
#define	MOUNT_UFML	"ufml"		/* UFML Filesystem */

#define	INITMOUNTNAMES { 	\
	MOUNT_NONE, 	/* 0 */	\
	MOUNT_UFS, 	/* 1 */	\
	MOUNT_FFS, 	/* 2 */	\
	MOUNT_NFS,	/* 3 */	\
	MOUNT_MFS, 	/* 4 */	\
	MOUNT_LFS, 	/* 5 */	\
	MOUNT_MSDOS, 	/* 6 */ \
	MOUNT_FDESC, 	/* 7 */	\
	MOUNT_LOFS, 	/* 8 */	\
	MOUNT_CD9660, 	/* 9 */	\
	MOUNT_UNION, 	/* 10 */\
	MOUNT_UFS211, 	/* 11 */\
	MOUNT_UFML, 	/* 12 */\
	0,			\
}

#define	MOUNT_MAXTYPE	12

/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
LIST_HEAD(vnodelst, vnode);
struct	mount {
	CIRCLEQ_ENTRY(mount) 	mnt_list;					/* mount list */
	const struct vfsops		*mnt_op;					/* operations on fs */
	struct vfsconf			*mnt_vfc;					/* configuration info */
	struct vnode			*mnt_vnodecovered;			/* vnode we mounted on */
	struct vnodelst			mnt_vnodelist;				/* list of vnodes this mount */
	struct lock				mnt_lock;					/* mount structure lock */
	int						mnt_flag;					/* flags */
	int						mnt_maxsymlinklen;			/* max size of short symlink */
	struct statfs			mnt_stat;					/* cache of filesystem stats */
	void 					*mnt_data;					/* private data */

	dev_t					mnt_dev;					/* device mounted */
	memaddr_t				mnt_extern;					/* click address of mount table extension */
	struct xmount			*mnt_xmnt;					/* 2.11BSD mount extension */
#define	mnt_mntfrom			mnt_xmnt->xm_mntfrom
#define mnt_mnton			mnt_xmnt->xm_mnton
};

struct xmount {
	char					xm_mntfrom[MNAMELEN];		/* /dev/xxxx mounted from */
	char					xm_mnton[MNAMELEN];			/* directory mounted on - this is the full(er) version of fs_fsmnt. */
};

#define XMNT_RW				0x06						/* 2.11BSD RW flag from pdp11 seg.h */
#define	XMOUNTDESC			(((btoc(sizeof(struct xmount)) - 1) << 8) | XMNT_RW)

/*
 * Mount flags.
 */
#define	MNT_RDONLY			0x0001		/* read only filesystem */
#define	MNT_SYNCHRONOUS		0x0002		/* file system written synchronously */
#define	MNT_NOEXEC			0x0004		/* can't exec from filesystem */
#define	MNT_NOSUID			0x0008		/* don't honor setuid bits on fs */
#define	MNT_NODEV			0x0010		/* don't interpret special files */
#define	MNT_QUOTA			0x0020		/* quotas are enabled on filesystem */
#define	MNT_ASYNC			0x0040		/* file system written asynchronously */
#define	MNT_NOATIME			0x0080		/* don't update access times */
#define	MNT_UNION			0x0100		/* union with underlying filesystem */

/*
 * exported mount flags.
 */
#define	MNT_EXRDONLY		0x00000080	/* exported read only */
#define	MNT_EXPORTED		0x00000100	/* file system is exported */
#define	MNT_DEFEXPORTED		0x00000200	/* exported to the world */
#define	MNT_EXPORTANON		0x00000400	/* use anon uid mapping for everyone */
#define	MNT_EXKERB			0x00000800	/* exported with Kerberos uid mapping */

/*
 * Flags set by internal operations.
 */
#define	MNT_LOCAL			0x00001000	/* filesystem is stored locally */
/*							0x00002000 */
#define	MNT_ROOTFS			0x00004000	/* identifies the root filesystem */

/*
 * Mask of flags that are visible to statfs().
*/
#define	MNT_VISFLAGMASK		0x0fff

/*
 * filesystem control flags.  The high 4 bits are used for this.  Since NFS
 * support will never be a problem we can avoid making the flags into a 'long.
*/
#define	MNT_UPDATE			0x1000		/* not a real mount, just an update */
#define	MNT_DELEXPORT		0x00020000	/* delete export host lists */
#define	MNT_RELOAD			0x00040000	/* reload filesystem data */
#define	MNT_FORCE			0x00080000	/* force unmount or readonly change */

/*
 * Internal filesystem control flags.
 *
 * MNT_UNMOUNT locks the mount entry so that name lookup cannot proceed
 * past the mount point.  This keeps the subtree stable during mounts
 * and unmounts.
 */
#define MNT_UNMOUNT			0x01000000	/* unmount in progress */
#define	MNT_MWAIT			0x02000000	/* waiting for unmount to finish */
#define MNT_WANTRDWR		0x04000000	/* upgrade to read/write requested */

/*
 * Sysctl CTL_VFS definitions.
 *
 * Second level identifier specifies which filesystem. Second level
 * identifier VFS_GENERIC returns information about all filesystems.
 */
#define	VFS_GENERIC			0	/* generic filesystem information */

/*
 * used to get configured filesystems information
 */
#define VFS_MAXNAMELEN 		32

/*
 * Third level identifiers for VFS_GENERIC are given below; third
 * level identifiers for specific filesystems are given in their
 * mount specific header files.
 */
#define VFS_MAXTYPENUM		1	/* int: highest defined filesystem type */
#define VFS_CONF			2	/* struct: vfsconf for filesystem given
				   	   	   	   as next argument */

/*
 * Flags for various system call interfaces.
 *
 * These aren't used for anything in the system and are present only
 * for source code compatibility reasons.
*/
#define	MNT_WAIT			1
#define	MNT_NOWAIT			2
#define MNT_LAZY			3

/*
 * Generic file handle
 */
struct fhandle {
	fsid_t					fh_fsid;	/* File system id of mount point */
	struct	fid 			fh_fid;		/* File sys specific id */
};
typedef struct fhandle		fhandle_t;

/*
 * Export arguments for local filesystem mount calls.
 */
struct export_args {
	int						ex_flags;		/* export related flags */
	uid_t					ex_root;		/* mapping for root uid */
	struct ucred 			ex_anon;		/* mapping for anonymous user */
	struct sockaddr 		*ex_addr;		/* net address to which exported */
	int						ex_addrlen;		/* and the net address length */
	struct sockaddr 		*ex_mask;		/* mask of valid bits in saddr */
	int						ex_masklen;		/* and the smask length */
};

/*
 * Filesystem configuration information. One of these exists for each
 * type of filesystem supported by the kernel. These are searched at
 * mount time to identify the requested filesystem.
 */

struct vfs_list;
LIST_HEAD(vfs_list, vfsconf);
struct vfsconf {
	const struct vfsops 	*vfc_vfsops; 				/* filesystem operations vector */
	const char 				*vfc_name;	                /* filesystem type name */
	int 					vfc_index;
	int						vfc_typenum;				/* historic filesystem type number */
	int 					vfc_refcount;				/* number mounted of this type */
	int 					vfc_flags;					/* permanent flags */
	int						(*vfc_mountroot)(void);		/* if != NULL, routine to mount root */
	LIST_ENTRY(vfsconf)		vfc_next;					/* next in list */
};

#ifdef _KERNEL

extern int 					maxvfsconf;		/* highest defined filesystem type */
extern struct vfs_list		vfsconflist;	/* head of list of filesystem types */

/*
 * Operations supported on mounted file system.
 */
struct mbuf;
struct mount;
struct nameidata;

struct vfsops {
	int	(*vfs_mount)(struct mount *, char *, caddr_t, struct nameidata *, struct proc *);
	int	(*vfs_start)(struct mount *, int, struct proc *);
	int	(*vfs_unmount)(struct mount *, int, struct proc *);
	int	(*vfs_root)(struct mount *, struct vnode **);
	int	(*vfs_quotactl)(struct mount *, int, uid_t, caddr_t, struct proc *);
	int	(*vfs_statfs)(struct mount *, struct statfs *, struct proc *);
	int	(*vfs_sync)(struct mount *, int, struct ucred *, struct proc *);
	int	(*vfs_vget)(struct mount *, ino_t, struct vnode **);
	int	(*vfs_fhtovp)(struct mount *, struct fid *, struct mbuf *, struct vnode **, int *, struct ucred **);
	int	(*vfs_vptofh)(struct vnode *, struct fid *);
	int	(*vfs_init)(struct vfsconf *);
	int	(*vfs_sysctl)(int *, u_int, void *, size_t *, void *, size_t, struct proc *);
};

#define VFS_MOUNT(MP, PATH, DATA, NDP, P) 			\
	(*(MP)->mnt_op->vfs_mount)(MP, PATH, DATA, NDP, P)
#define VFS_START(MP, FLAGS, P)	  					\
	(*(MP)->mnt_op->vfs_start)(MP, FLAGS, P)
#define VFS_UNMOUNT(MP, FORCE, P) 					\
	(*(MP)->mnt_op->vfs_unmount)(MP, FORCE, P)
#define VFS_ROOT(MP, VPP)	  						\
	(*(MP)->mnt_op->vfs_root)(MP, VPP)
#define VFS_QUOTACTL(MP,C,U,A,P)  					\
	(*(MP)->mnt_op->vfs_quotactl)(MP, C, U, A, P)
#define VFS_STATFS(MP, SBP, P)	  					\
	(*(MP)->mnt_op->vfs_statfs)(MP, SBP, P)
#define VFS_SYNC(MP, WAIT, C, P)  					\
	(*(MP)->mnt_op->vfs_sync)(MP, WAIT, C, P)
#define VFS_VGET(MP, INO, VPP)	  					\
	(*(MP)->mnt_op->vfs_vget)(MP, INO, VPP)
#define VFS_FHTOVP(MP, FIDP, NAM, VPP, EXFLG, CRED) \
	(*(MP)->mnt_op->vfs_fhtovp)(MP, FIDP, NAM, VPP, EXFLG, CRED)
#define	VFS_VPTOFH(VP, FIDP)	  					\
	(*(VP)->v_mount->mnt_op->vfs_vptofh)(VP, FIDP)

/*
 * Network address lookup element
 */
struct netcred {
	struct	radix_node 		netc_rnodes[2];
	int						netc_refcnt;
	int						netc_exflags;
	struct	ucred 			netc_anon;
};

/*
 * Network export information
 */
struct netexport {
	struct	netcred 		ne_defexported;		    /* Default export */
	struct	radix_node_head *ne_rtable[AF_MAX+1]; 	/* Individual exports */
};

/*
 * exported vnode operations
 */
int				dounmount(struct mount *, int, struct proc *);
struct mount 	*vfs_getvfs(fsid_t *);														/* return vfs given fsid */
int				vflush(struct mount *, struct vnode *, int);
int				vfs_export(struct mount *, struct netexport *, struct export_args *); 		/* process mount export info */
struct netcred 	*vfs_export_lookup(struct mount *, struct netexport *, struct mbuf *); 		/* lookup host in fs export list */
int				vfs_mountedon(struct vnode *);    											/* is a vfs mounted on vp */
int				vfs_busy(struct mount *, int, struct lock_object *, struct proc *);  		/* mark a vfs  busy */
void			vfs_unbusy(struct mount *, struct proc *);       							/* mark a vfs not busy */
int				vfs_rootmountalloc(char *, char *, struct mount **);
int				vfs_mountroot(void);
void			vfs_getnewfsid(struct mount *);
void			vfs_timestamp(struct timespec *);
void			vfs_unmountall(void);

/* vfsconf */
void			vfsconf_fs_init(void);
void			vfsconf_fs_create(struct vfsops *, const char *, int, int, int, int (*)(void));
struct vfsconf 	*vfsconf_find_by_name(const char *);
struct vfsconf 	*vfsconf_find_by_typenum(int);
void			vfsconf_attach(struct vfsconf *);
void			vfsconf_detach(struct vfsconf *);

extern CIRCLEQ_HEAD(mntlist, mount) mountlist;												/* mounted filesystem list */
extern struct lock_object mountlist_slock;
extern struct lock_object spechash_slock;

#else /* KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
int		fstatfs(int, struct statfs *);
int		getfh(const char *, fhandle_t *);
int		getfsstat(struct statfs *, long, int);
int		getmntinfo(struct statfs **, int);
int		mount(const char *, const char *, int, void *);
int		statfs(const char *, struct statfs *);
int		unmount(const char *, int);
__END_DECLS

#endif /* KERNEL */
#endif
