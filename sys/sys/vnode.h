/*
 * Copyright (c) 1989, 1993
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
 *	@(#)vnode.h	8.17 (Berkeley) 5/20/95
 */
#ifndef _SYS_VNODE_H_
#define	_SYS_VNODE_H_

#include <sys/lock.h>
#include <sys/queue.h>

/*
 * The vnode is the focus of all file activity in UNIX.  There is a
 * unique vnode allocated for each active file, each current directory,
 * each mounted-on file, text file, and the root.
 */

/*
 * Vnode types.  VNON means no type.
 */
enum vtype	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD };

/*
 * Vnode tag types.
 * These are for the benefit of external programs only (e.g., pstat)
 * and should NEVER be inspected by the kernel.
 */
enum vtagtype	{
	VT_NON, VT_UFS, VT_NFS, VT_MFS, VT_MSDOSFS, VT_LFS, VT_LOFS, VT_FDESC,
	VT_PORTAL, VT_NULL, VT_UMAP, VT_KERNFS, VT_PROCFS, VT_AFS, VT_ISOFS,
	VT_UNION, VT_UFML, VT_UFS211
};

/*
 * Each underlying filesystem allocates its own private area and hangs
 * it from v_data.  If non-null, this area is freed in getnewvnode().
 */
LIST_HEAD(buflists, buf);
struct vnode {
	u_long				v_flag;			/* vnode flags (see below) */
	short				v_usecount;		/* reference count of users */
	short				v_writecount;	/* reference count of writers */
	long				v_holdcnt;		/* page & buffer references */
	daddr_t				v_lastr;		/* last read (read-ahead) */
	u_long				v_id;			/* capability identifier */
	struct mount 		*v_mount;		/* ptr to vfs we are in */
	struct vnodeops		*v_op;			/* vnode operations vector */
	TAILQ_ENTRY(vnode) 	v_freelist;		/* vnode freelist */
	LIST_ENTRY(vnode) 	v_mntvnodes;	/* vnodes for mount point */
	struct buflists 	v_cleanblkhd;	/* clean blocklist head */
	struct buflists 	v_dirtyblkhd;	/* dirty blocklist head */
	long				v_numoutput;	/* num of writes in progress */
	enum vtype 			v_type;			/* vnode type */
	union {
		struct proc		*vu_proc;		/* ptr to proc */
		struct mount	*vu_mountedhere;/* ptr to mounted vfs (VDIR) */
		struct socket	*vu_socket;		/* unix ipc (VSOCK) */
		caddr_t			vu_vmdata;		/* private data for vm (VREG) */
		struct specinfo	*vu_specinfo;	/* device (VCHR, VBLK) */
		struct fifoinfo	*vu_fifoinfo;	/* fifo (VFIFO) */
	} v_un;
	struct nqlease 		*v_lease;		/* Soft reference to lease */
	daddr_t				v_lastw;		/* last write (write cluster) */
	daddr_t				v_cstart;		/* start block of cluster */
	daddr_t				v_lasta;		/* last allocation */
	int					v_clen;			/* length of current cluster */
	int					v_ralen;		/* Read-ahead length */
	daddr_t				v_maxra;		/* last readahead block */
	struct lock_object	v_interlock;	/* lock on usecount and flag */
	struct lock 		*v_vnlock;		/* used for non-locking fs's */
	long				v_spare[5];		/* round to 128 bytes */
	enum vtagtype 		v_tag;			/* type of underlying data */
	void 				*v_data;		/* private data for fs */
};

#define	v_mountedhere	v_un.vu_mountedhere
#define v_proc			v_un.vu_proc
#define	v_socket		v_un.vu_socket
#define	v_vmdata		v_un.vu_vmdata
#define	v_specinfo		v_un.vu_specinfo
#define	v_fifoinfo		v_un.vu_fifoinfo

/*
 * Vnode flags.
 */
#define	VROOT		0x0001	/* root of its file system */
#define	VTEXT		0x0002	/* vnode is a pure text prototype */
#define	VSYSTEM		0x0004	/* vnode being used by kernel */
#define	VXLOCK		0x0100	/* vnode is locked to change underlying type */
#define	VXWANT		0x0200	/* process is waiting for vnode */
#define	VBWAIT		0x0400	/* waiting for output to complete */
#define	VALIASED	0x0800	/* vnode has an alias */
#define	VDIROP		0x1000	/* LFS: vnode is involved in a directory op */

/*
 * Vnode attributes.  A field value of VNOVAL represents a field whose value
 * is unavailable (getattr) or which is not to be changed (setattr).
 */
struct vattr {
	enum vtype		va_type;		/* vnode type (for create) */
	u_short			va_mode;		/* files access mode and type */
	short			va_nlink;		/* number of references to file */
	uid_t			va_uid;			/* owner user id */
	gid_t			va_gid;			/* owner group id */
	long			va_fsid;		/* file system id (dev for now) */
	long			va_fileid;		/* file id */
	u_quad_t		va_size;		/* file size in bytes */
	long			va_blocksize;	/* blocksize preferred for i/o */
	struct timespec	va_atime;		/* time of last access */
	struct timespec	va_mtime;		/* time of last modification */
	struct timespec	va_ctime;		/* time file changed */
	u_long			va_gen;			/* generation number of file */
	u_long			va_flags;		/* flags defined for file */
	dev_t			va_rdev;		/* device the special file represents */
	u_quad_t		va_bytes;		/* bytes of disk space held by file */
	u_quad_t		va_filerev;		/* file modification number */
	u_int			va_vaflags;		/* operations flags, see below */
	long			va_spare;		/* remain quad aligned */
};

/*
 * Flags for va_cflags.
 */
#define	VA_UTIMES_NULL	0x01		/* utimes argument was NULL */
#define VA_EXCLUSIVE	0x02		/* exclusive create request */

/*
 * Flags for ioflag.
 */
#define	IO_UNIT			0x01		/* do I/O as atomic unit */
#define	IO_APPEND		0x02		/* append write to end */
#define	IO_SYNC			0x04		/* do I/O synchronously */
#define	IO_NODELOCKED	0x08		/* underlying node already locked */
#define	IO_NDELAY		0x10		/* FNDELAY flag set in file table */

/*
 *  Modes.  Some values same as Ixxx entries from inode.h for now.
 */
#define	VSUID			04000		/* set user id on execution */
#define	VSGID			02000		/* set group id on execution */
#define	VSVTX			01000		/* save swapped text even after use */
#define	VREAD			00400		/* read, write, execute permissions */
#define	VWRITE			00200
#define	VEXEC			00100

/*
 * Token indicating no attribute value yet assigned.
 */
#define	VNOVAL	(-1)

#ifdef KERNEL
/*
 * Convert between vnode types and inode formats (since POSIX.1
 * defines mode word of stat structure in terms of inode formats).
 */
extern enum vtype				iftovt_tab[];
extern int						vttoif_tab[];
#define IFTOVT(mode)			(iftovt_tab[((mode) & S_IFMT) >> 12])
#define VTTOIF(indx)			(vttoif_tab[(int)(indx)])
#define MAKEIMODE(indx, mode)	(int)(VTTOIF(indx) | (mode))

/*
 * Flags to various vnode functions.
 */
#define	SKIPSYSTEM	0x0001		/* vflush: skip vnodes marked VSYSTEM */
#define	FORCECLOSE	0x0002		/* vflush: force file closeure */
#define	WRITECLOSE	0x0004		/* vflush: only close writeable files */
#define	DOCLOSE		0x0008		/* vclean: close active files */
#define	V_SAVE		0x0001		/* vinvalbuf: sync file first */
#define	V_SAVEMETA	0x0002		/* vinvalbuf: leave indirect blocks */
#define	REVOKEALL	0x0001		/* vop_revoke: revoke all aliases */

#ifdef DIAGNOSTIC
#define	HOLDRELE(vp)	holdrele(vp)
#define	VATTR_NULL(vap)	vattr_null(vap)
#define	VHOLD(vp)		vhold(vp)
#define	VREF(vp)		vref(vp)

void	holdrele (struct vnode *);
void	vattr_null (struct vattr *);
void	vhold (struct vnode *);
void	vref (struct vnode *);
#else
#define	VATTR_NULL(vap)	(*(vap) = va_null)	/* initialize a vattr */
#define	HOLDRELE(vp)	holdrele(vp)		/* decrease buf or page ref */

static __inline holdrele(vp)
	struct vnode *vp;
{
	simple_lock(&vp->v_interlock);
	vp->v_holdcnt--;
	simple_unlock(&vp->v_interlock);
}
#define	VHOLD(vp)	vhold(vp)				/* increase buf or page ref */
static __inline vhold(vp)
	struct vnode *vp;
{
	simple_lock(&vp->v_interlock);
	vp->v_holdcnt++;
	simple_unlock(&vp->v_interlock);
}
#define	VREF(vp)	vref(vp)				/* increase reference */
static __inline vref(vp)
	struct vnode *vp;
{
	simple_lock(&vp->v_interlock);
	vp->v_usecount++;
	simple_unlock(&vp->v_interlock);
}
#endif /* DIAGNOSTIC */

#define	NULLVP	((struct vnode *)NULL)

/*
 * Global vnode data.
 */
extern	struct vnode *rootvnode;	/* root (i.e. "/") vnode */
extern	int desiredvnodes;			/* number of vnodes desired */
extern	struct vattr va_null;		/* predefined null vattr structure */

/*
 * Macro/function to check for client cache inconsistency w.r.t. leasing.
 */
#define	LEASE_READ	0x1		/* Check lease for readers */
#define	LEASE_WRITE	0x2		/* Check lease for modifiers */

#endif /* KERNEL */

struct vnodeops {
	int	(*vop_lookup)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp);
	int	(*vop_create)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
	int (*vop_whiteout)		(struct vnode *dvp, struct componentname *cnp, int flags);
	int	(*vop_mknod)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
	int	(*vop_open)	    	(struct vnode *vp, int mode, struct ucred *cred, struct proc *p);
	int	(*vop_close)		(struct vnode *vp, int fflag, struct ucred *cred, struct proc *p);
	int	(*vop_access)		(struct vnode *vp, int mode, struct ucred *cred, struct proc *p);
	int	(*vop_getattr)		(struct vnode *vp, struct vattr *vap, struct ucred *cred, struct proc *p);
	int	(*vop_setattr)		(struct vnode *vp, struct vattr *vap, struct ucred *cred, struct proc *p);
	int	(*vop_read)	    	(struct vnode *vp, struct uio *uio, int ioflag, struct ucred *cred);
	int	(*vop_write)		(struct vnode *vp, struct uio *uio, int ioflag, struct ucred *cred);
	int (*vop_lease)		(struct vnode *vp, struct proc *p, struct ucred *cred, int flag);
	int	(*vop_ioctl)		(struct vnode *vp, u_long command, caddr_t data, int fflag, struct ucred *cred, struct proc *p);
	int	(*vop_select)		(struct vnode *vp, int which, int fflags, struct ucred *cred, struct proc *p);
	int	(*vop_poll)			(struct vnode *vp, int fflags, int events, struct proc *p);
	int (*vop_revoke)		(struct vnode *vp, int flags);
	int	(*vop_mmap)	    	(struct vnode *vp, int fflags, struct ucred *cred, struct proc *p);
	int	(*vop_fsync)		(struct vnode *vp, int fflags, struct ucred *cred, int waitfor, int flag, struct proc *p);
	int	(*vop_seek)	    	(struct vnode *vp, off_t oldoff, off_t newoff, struct ucred *cred);
	int	(*vop_remove)		(struct vnode *dvp, struct vnode *vp, struct componentname *cnp);
	int	(*vop_link)	    	(struct vnode *vp, struct vnode *tdvp, struct componentname *cnp);
	int	(*vop_rename)		(struct vnode *fdvp, struct vnode *fvp, struct componentname *fcnp, struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp);
	int	(*vop_mkdir)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
	int	(*vop_rmdir)		(struct vnode *dvp, struct vnode *vp, struct componentname *cnp);
	int	(*vop_symlink)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap, char *target);
	int	(*vop_readdir)		(struct vnode *vp, struct uio *uio, struct ucred *cred, int *eofflag, int *ncookies, u_long **cookies);
	int	(*vop_readlink)		(struct vnode *vp, struct uio *uio, struct ucred *cred);
	int	(*vop_abortop)		(struct vnode *dvp, struct componentname *cnp);
	int	(*vop_inactive)		(struct vnode *vp, struct proc *p);
	int	(*vop_reclaim)		(struct vnode *vp, struct proc *p);
	int	(*vop_lock)	    	(struct vnode *vp, int flags, struct proc *p);
	int	(*vop_unlock)		(struct vnode *vp, int flags, struct proc *p);
	int	(*vop_bmap)	    	(struct vnode *vp, daddr_t bn, struct vnode **vpp, daddr_t *bnp, int *runp);
	int	(*vop_print)		(struct vnode *vp);
	int	(*vop_islocked)		(struct vnode *vp);
	int (*vop_pathconf)		(struct vnode *vp, int name, register_t *retval);
	int	(*vop_advlock)		(struct vnode *vp, caddr_t id, int op, struct flock *fl, int flags);
	int (*vop_blkatoff)		(struct vnode *vp, off_t offset, char **res, struct buf **bpp);
	int (*vop_valloc)		(struct vnode *pvp, int mode, struct ucred *cred, struct vnode **vpp);
	int (*vop_reallocblks)	(struct vnode *vp, struct cluster_save *buflist);
	int (*vop_vfree)		(struct vnode *pvp, ino_t ino, int mode);
	int (*vop_truncate)		(struct vnode *vp, off_t length, int flags, struct ucred *cred, struct proc *p);
	int (*vop_update)		(struct vnode *vp, struct timeval *access, struct timeval *modify, int waitfo);
	int	(*vop_strategy)		(struct buf *bp);
	int (*vop_bwrite)		(struct buf *bp);
};

/* Macros to call vnodeops */
#define	VOP_LOOKUP(dvp, vpp, cnp)	    						(*((dvp)->v_op->vop_lookup))(dvp, vpp, cnp)
#define	VOP_CREATE(dvp, vpp, cnp, vap)	    					(*((dvp)->v_op->vop_create))(dvp, vpp, cnp, vap)
#define VOP_WHITEOUT(dvp, cnp, flags)							(*((dvp)->v_op->vop_whiteout))(dvp, cnp, flags)
#define	VOP_MKNOD(dvp, vpp, cnp, vap)	    					(*((dvp)->v_op->vop_mknod))(dvp, vpp, cnp, vap)
#define	VOP_OPEN(vp, mode, cred, p)	    						(*((vp)->v_op->vop_open))(vp, mode, cred, p)
#define	VOP_CLOSE(vp, fflag, cred, p)	    					(*((vp)->v_op->vop_close))(vp, fflag, cred, p)
#define	VOP_ACCESS(vp, mode, cred, p)	    					(*((vp)->v_op->vop_access))(vp, mode, cred, p)
#define	VOP_GETATTR(vp, vap, cred, p)							(*((vp)->v_op->vop_getattr))(vp, vap, cred, p)
#define	VOP_SETATTR(vp, vap, cred, p)							(*((vp)->v_op->vop_setattr))(vp, vap, cred, p)
#define	VOP_READ(vp, uio, ioflag, cred)	    					(*((vp)->v_op->vop_read))(vp, uio, ioflag, cred)
#define	VOP_WRITE(vp, uio, ioflag, cred)						(*((vp)->v_op->vop_write))(vp, uio, ioflag, cred)
#define VOP_LEASE(vp, p, cred, flag)							(*((vp)->v_op->vop_lease))(vp, p, cred, flag)
#define	VOP_IOCTL(vp, command, data, fflag, cred, p)			(*((vp)->v_op->vop_ioctl))(vp, command, data, fflag, cred, p)
#define	VOP_SELECT(vp, which, fflags, cred, p)					(*((vp)->v_op->vop_select))(vp, which, fflags, cred, p)
#define VOP_POLL(vp, fflag, events, p)							(*((vp)->v_op->vop_poll))(vp, fflag, events, p)
#define VOP_REVOKE(vp, flags)									(*((vp)->v_op->vop_revoke))(vp, flags)
#define	VOP_MMAP(vp, fflags, cred, p)		    				(*((vp)->v_op->vop_mmap))(vp, fflags, cred, p)
#define	VOP_FSYNC(vp, cred, waitfor, flag, p)					(*((vp)->v_op->vop_fsync))(vp, cred, waitfor, flag, p)
#define	VOP_SEEK(vp, oldoff, newoff, cred)	    				(*((vp)->v_op->vop_seek))(vp, oldoff, newoff, cred)
#define	VOP_REMOVE(dvp, vp, cnp)		    					(*((dvp)->v_op->vop_remove))(dvp, vp, cnp)
#define	VOP_LINK(vp, tdvp, cnp)		    						(*((vp)->v_op->vop_link))(vp, tdvp, cnp)
#define	VOP_RENAME(fdvp, fvp, fcnp, tdvp, tvp, tcnp)			(*((fdvp)->v_op->vop_rename))(fdvp, fvp, fcnp, tdvp, tvp, tcnp)
#define	VOP_MKDIR(dvp, vpp, cnp, vap)	   						(*((dvp)->v_op->vop_mkdir))(dvp, vpp, cnp, vap)
#define	VOP_RMDIR(dvp, vp, cnp)		    						(*((dvp)->v_op->vop_rmdir))(dvp, vp, cnp)
#define	VOP_SYMLINK(dvp, vpp, cnp, vap, target)					(*((dvp)->v_op->vop_symlink))(dvp, vpp, cnp, vap, target)
#define	VOP_READDIR(vp, uio, cred, eofflag, ncookies, cookies)	(*((vp)->v_op->vop_readdir))(vp, uio, cred, eofflag, ncookies, cookies)
#define	VOP_READLINK(vp, uio, cred)	    						(*((vp)->v_op->vop_readlink))(vp, uio, cred)
#define	VOP_ABORTOP(dvp, cnp)		    						(*((dvp)->v_op->vop_abortop))(dvp, cnp)
#define	VOP_INACTIVE(vp, p)	    								(*((vp)->v_op->vop_inactive))(vp, p)
#define	VOP_RECLAIM(vp, p)		    							(*((vp)->v_op->vop_reclaim))(vp, p)
#define	VOP_LOCK(vp, flags, p)		        					(*((vp)->v_op->vop_lock))(vp, flags, p)
#define	VOP_UNLOCK(vp, flags, p)		    					(*((vp)->v_op->vop_unlock))(vp, flags, p)
#define	VOP_BMAP(vp, bn, vpp, bnp, runp)	    				(*((vp)->v_op->vop_bmap))(vp, bn, vpp, bnp, runp)
#define	VOP_PRINT(vp)		    								(*((vp)->v_op->vop_print))(vp)
#define	VOP_ISLOCKED(vp)		    							(*((vp)->v_op->vop_islocked))(vp)
#define VOP_PATHCONF(vp, name, retval)							(*((vp)->v_op->vop_pathconf))(vp, name, retval)
#define	VOP_ADVLOCK(vp, id, op, fl, flags)						(*((vp)->v_op->vop_advlock))(vp, id, op, fl, flags)
#define VOP_BLKATOFF(vp, offset, res, bpp)						(*((vp)->v_op->vop_blkatoff))(vp, offset, res, bpp)
#define VOP_VALLOC(pvp, mode, cred, vpp)						(*((pvp)->v_op->vop_valloc))(pvp, mode, cred, vpp)
#define VOP_REALLOCBLKS(vp, buflist)							(*((vp)->v_op->vop_reallocblks))(vp, buflist)
#define VOP_VFREE(pvp, ino, mode)								(*((pvp)->v_op->vop_vfree))(pvp, ino, mode)
#define VOP_TRUNCATE(vp, length, flags, cred, p)				(*((vp)->v_op->vop_truncate))(vp, length, flags, cred, p)
#define VOP_UPDATE(vp, access, modify, waitfor)					(*((vp)->v_op->vop_update))(vp, access, modify, waitfor)
#define	VOP_STRATEGY(bp)										(*((bp)->b_vp->v_op->vop_strategy))(bp)
#define VOP_BWRITE(bp)											(*((bp)->b_vp->v_op->vop_bwrite))(bp)

/*
 * vnodeops args:
 * A generic structure.
 * This can be used by bypass routines to identify generic arguments.
 */
struct vop_generic_args {
	struct vnodeops 		*a_ops;
};

struct vop_lookup_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
};

struct vop_create_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};

struct vop_whiteout_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct componentname 	*a_cnp;
	int 					a_flags;
};

struct vop_mknod_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};

struct vop_open_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_close_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_fflag;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_access_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_getattr_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct vattr 			*a_vap;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_setattr_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct vattr 			*a_vap;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_read_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	int 					a_ioflag;
	struct ucred 			*a_cred;
};

struct vop_write_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	int 					a_ioflag;
	struct ucred 			*a_cred;
};

struct vop_lease_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
	struct ucred 			*a_cred;
	int 					a_flag;
};

struct vop_ioctl_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	u_long 					a_command;
	caddr_t 				a_data;
	int 					a_fflag;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_select_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_which;
	int 					a_fflags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_poll_args {
	struct vop_generic_args a_head;
	struct vnode 			*a_vp;
	int 					a_fflags;
	int 					a_events;
	struct proc 			*a_p;
};

struct vop_revoke_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_flags;
};

struct vop_mmap_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_fflags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_fsync_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct ucred 			*a_cred;
	int 					a_waitfor;
	int						a_flags;
	struct proc 			*a_p;
};

struct vop_seek_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	off_t 					a_oldoff;
	off_t 					a_newoff;
	struct ucred 			*a_cred;
};

struct vop_remove_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			*a_vp;
	struct componentname 	*a_cnp;
};

struct vop_link_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct vnode 			*a_tdvp;
	struct componentname 	*a_cnp;
};

struct vop_rename_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_fdvp;
	struct vnode 			*a_fvp;
	struct componentname 	*a_fcnp;
	struct vnode 			*a_tdvp;
	struct vnode 			*a_tvp;
	struct componentname 	*a_tcnp;
};

struct vop_mkdir_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};

struct vop_rmdir_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			*a_vp;
	struct componentname 	*a_cnp;
};

struct vop_symlink_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
	char 					*a_target;
};

struct vop_readdir_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	struct ucred 			*a_cred;
	int 					*a_eofflag;
	int 					*a_ncookies;
	u_long 					**a_cookies;
};

struct vop_readlink_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	struct ucred 			*a_cred;
};

struct vop_abortop_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct componentname	*a_cnp;
};

struct vop_inactive_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
};

struct vop_reclaim_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
};

struct vop_lock_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_flags;
	struct proc 			*a_p;
};

struct vop_unlock_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_flags;
	struct proc 			*a_p;
};

struct vop_bmap_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	daddr_t 				a_bn;
	struct vnode 			**a_vpp;
	daddr_t 				*a_bnp;
	int 					*a_runp;
};

struct vop_print_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
};

struct vop_islocked_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
};

struct vop_pathconf_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_name;
	register_t 				*a_retval;
};

struct vop_advlock_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	caddr_t 				a_id;
	int 					a_op;
	struct flock 			*a_fl;
	int 					a_flags;
};

struct vop_blkatoff_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	off_t 					a_offset;
	char 					**a_res;
	struct buf 				**a_bpp;
};

struct vop_valloc_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_pvp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct vnode 			**a_vpp;
};

struct vop_reallocblks_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct cluster_save 	*a_buflist;
};

struct vop_vfree_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_pvp;
	ino_t 					a_ino;
	int 					a_mode;
};

struct vop_truncate_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	off_t 					a_length;
	int 					a_flags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_update_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct timeval 			*a_access;
	struct timeval 			*a_modify;
	int 					a_waitfor;
};

struct vop_pageread {
	struct vop_generic_args	a_head;

	vm_page_t				a_page;
};

struct vop_pagewrite {
	struct vop_generic_args	a_head;

	vm_page_t				a_page;
};

/* Special cases: */
struct vop_strategy_args {
	struct vop_generic_args	a_head;
	struct buf 				*a_bp;
};

struct vop_bwrite_args {
	struct vop_generic_args	a_head;
	struct buf 				*a_bp;
};
/* End of special cases. */

#ifdef _KERNEL

#define VARGVOPS(ap) 		((ap)->a_head.a_ops)
#define VOPARGS(ap, vop_field) 									\
	VARGVOPS(ap)->vop_field

#define VOPTEST(vops, ap, vop_field) { 							\
	if(VOPARGS(ap, vop_field) == (vops).vop_field) {			\
		VOPARGS(ap, vop_field);									\
	} else {													\
		(vops).vop_field;										\
	}															\
}																\

#define VOCALL(vops, ap, vop_field)								\
	VOPTEST(vops, ap, vop_field)

/*
 * Interlock for scanning list of vnodes attached to a mountpoint
 */
struct lock_object mntvnode_slock;

/*
 * Finally, include the default set of vnode operations.
 */

extern struct vnodeops vops;

/*
 * Public vnode manipulation functions.
 */
struct file;
struct mount;
struct nameidata;
struct ostat;
struct proc;
struct stat;
struct ucred;
struct uio;
struct vattr;
struct vnode;
struct vop_bwrite_args;

int 	bdevvp (dev_t dev, struct vnode **vpp);
void	cvtstat (struct stat *st, struct ostat *ost);
int 	getnewvnode (enum vtagtype tag, struct mount *mp, int (**vops)(), struct vnode **vpp);
void	insmntque (struct vnode *vp, struct mount *mp);
void 	vattr_null (struct vattr *vap);
int 	vcount (struct vnode *vp);
int		vflush (struct mount *mp, struct vnode *skipvp, int flags);
int 	vget (struct vnode *vp, int lockflag, struct proc *p);
void 	vgone (struct vnode *vp);
int		vinvalbuf (struct vnode *vp, int save, struct ucred *cred, struct proc *p, int slpflag, int slptimeo);
void	vprint (char *label, struct vnode *vp);
int		vrecycle (struct vnode *vp, struct simplelock *inter_lkp, struct proc *p);
int		vn_bwrite (struct vop_bwrite_args *ap);
int 	vn_close (struct vnode *vp, int flags, struct ucred *cred, struct proc *p);
int 	vn_closefile (struct file *fp, struct proc *p);
int		vn_ioctl (struct file *fp, u_long com, caddr_t data, struct proc *p);
int		vn_lock (struct vnode *vp, int flags, struct proc *p);
int 	vn_open (struct nameidata *ndp, int fmode, int cmode);
int 	vn_rdwr (enum uio_rw rw, struct vnode *vp, caddr_t base, int len, off_t offset, enum uio_seg segflg, int ioflg, struct ucred *cred, int *aresid, struct proc *p);
int		vn_read (struct file *fp, struct uio *uio, struct ucred *cred);
int		vn_select (struct file *fp, int which, struct proc *p);
int		vn_stat (struct vnode *vp, struct stat *sb, struct proc *p);
int		vn_write (struct file *fp, struct uio *uio, struct ucred *cred);
int		vop_noislocked (struct vop_islocked_args *);
int		vop_nolock (struct vop_lock_args *);
int		vop_nounlock (struct vop_unlock_args *);
int		vop_revoke (struct vop_revoke_args *);
struct 	vnode *checkalias (struct vnode *vp, dev_t nvp_rdev, struct mount *mp);
void 	vput (struct vnode *vp);
void 	vref (struct vnode *vp);
void 	vrele (struct vnode *vp);

/* vfs_vops.c */
void 	vop_init(); 					/* initialize vnode vector operations (vops) */
void 	vop_alloc(struct vnodeops *); 	/* allocate vnode vector operations (vops) */
void	vop_free(struct vnodeops *);	/* free vnode vector operations (vops) */
#endif /* KERNEL */
#endif /* !_SYS_VNODE_H_ */
