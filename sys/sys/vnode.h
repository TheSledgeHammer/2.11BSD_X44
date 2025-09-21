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
 *	@(#)vnode.h	7.39 (Berkeley) 6/27/91
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
enum vtype {
	VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD
};

/*
 * Vnode tag types.
 * These are for the benefit of external programs only (e.g., pstat)
 * and should NEVER be inspected by the kernel.
 */
enum vtagtype {
	VT_NON,	VT_UFS,	VT_MFS,	VT_LFS,	VT_UFS211, VT_NFS, VT_LOFS,
	VT_FDESC, VT_ISOFS, VT_MSDOSFS,	VT_UNION, VT_UFML
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
		struct vm_text	*vu_text;		/* text/mapped region (VREG) */
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
	struct selinfo		v_sel;			/* identity of poller(s) */
};

#define	v_mountedhere	v_un.vu_mountedhere
#define v_proc			v_un.vu_proc
#define	v_socket		v_un.vu_socket
#define v_text			v_un.vu_text
#define	v_vmdata		v_un.vu_vmdata
#define	v_specinfo		v_un.vu_specinfo
#define	v_fifoinfo		v_un.vu_fifoinfo

/*
 * Vnode flags.
 */
#define	VROOT		0x0001	/* root of its file system */
#define	VTEXT		0x0002	/* vnode is a pure text prototype */
#define	VSYSTEM		0x0004	/* vnode being used by kernel */
#define	VISTTY		0x0008	/* vnode represents a tty */
#define	VEXECMAP	0x0010	/* might have PROT_EXEC mappings */
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
#define	VWRITE			00200		/* write permission */
#define	VEXEC			00100		/* execute/search permission */
#define	VADMIN			10000 		/* being the file owner */
#define	VAPPEND			40000 		/* permission to write/append */

/*
 * Token indicating no attribute value yet assigned.
 */
#define	VNOVAL	(-1)

#ifdef _KERNEL
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
#define	SKIPSYSTEM		0x0001		/* vflush: skip vnodes marked VSYSTEM */
#define	FORCECLOSE		0x0002		/* vflush: force file closeure */
#define	WRITECLOSE		0x0004		/* vflush: only close writeable files */
#define	DOCLOSE			0x0008		/* vclean: close active files */
#define	V_SAVE			0x0001		/* vinvalbuf: sync file first */
#define	V_SAVEMETA		0x0002		/* vinvalbuf: leave indirect blocks */

#define	V_WAIT		        0x0001		/*  sleep for suspend */

#define	REVOKEALL		0x0001		/* vop_revoke: revoke all aliases */

#define	UPDATE_WAIT		0x0001		/* update: wait for completion */
#define	UPDATE_DIROP		0x0002		/* update: hint to fs to wait or not */
#define	UPDATE_CLOSE		0x0004		/* update: clean up on close */

#ifdef DIAGNOSTIC
#define	HOLDRELE(vp)		holdrele(vp)
#define	VATTR_NULL(vap)		vattr_null(vap)
#define	VHOLD(vp)		vhold(vp)
#define	VREF(vp)		vref(vp)

void	holdrele(struct vnode *);
void	vattr_null(struct vattr *);
void	vhold(struct vnode *);
void	vref(struct vnode *);
#else
#define	VATTR_NULL(vap)	(*(vap) = va_null)	/* initialize a vattr */

#define	HOLDRELE(vp)	holdrele(vp)		/* decrease buf or page ref */
static __inline
holdrele(vp)
	struct vnode *vp;
{
	simple_lock(&vp->v_interlock);
	vp->v_holdcnt--;
	simple_unlock(&vp->v_interlock);
}
#define	VHOLD(vp)		vhold(vp)				/* increase buf or page ref */
static __inline
vhold(vp)
	struct vnode *vp;
{
	simple_lock(&vp->v_interlock);
	vp->v_holdcnt++;
	simple_unlock(&vp->v_interlock);
}
#define	VREF(vp)		vref(vp)				/* increase reference */
static __inline
vref(vp)
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
extern struct vnode *rootvnode;		/* root (i.e. "/") vnode */
extern int desiredvnodes;			/* number of vnodes desired */
extern struct vattr va_null;		/* predefined null vattr structure */

/*
 * Macro/function to check for client cache inconsistency w.r.t. leasing.
 */
#define	LEASE_READ	0x1		/* Check lease for readers */
#define	LEASE_WRITE	0x2		/* Check lease for modifiers */
#endif /* KERNEL */

/*
 * Flags for vdesc_flags:
 */
#define	VDESC_MAX_VPS		16
/* Low order 16 flag bits are reserved for willrele flags for vp arguments. */
#define VDESC_VP0_WILLRELE	0x0001
#define VDESC_VP1_WILLRELE	0x0002
#define VDESC_VP2_WILLRELE	0x0004
#define VDESC_VP3_WILLRELE	0x0008
#define VDESC_NOMAP_VPP		0x0100
#define VDESC_VPP_WILLRELE	0x0200

/*
 * VDESC_NO_OFFSET is used to identify the end of the offset list
 * and in places where no such field exists.
 */
#define VDESC_NO_OFFSET 	-1

/*
 * This structure describes the vnode operation taking place.
 */
struct vnodeop_desc {
	int		vdesc_offset;				/* offset in vector--first for speed */
	char    *vdesc_name;				/* a readable name for debugging */
	int		vdesc_flags;				/* VDESC_* flags */

	/*
	 * These ops are used by bypass routines to map and locate arguments.
	 * Creds and procs are not needed in bypass routines, but sometimes
	 * they are useful to (for example) transport layers.
	 * Nameidata is useful because it has a cred in it.
	 */
	int		*vdesc_vp_offsets;			/* list ended by VDESC_NO_OFFSET */
	int		vdesc_vpp_offset;			/* return vpp location */
	int		vdesc_cred_offset;			/* cred location, if any */
	int		vdesc_proc_offset;			/* proc location, if any */
	int		vdesc_componentname_offset; /* if any */
	/* Support to manage this list is not yet part of BSD. */
	caddr_t	*vdesc_transports;			/* a list private data (about each operation) for each transport layer. */
};

#ifdef _KERNEL
/*
 * A list of all the operation descs.
 */
extern struct vnodeop_desc *vnodeop_descs[];

/*
 * Interlock for scanning list of vnodes attached to a mountpoint
 */
extern struct lock_object mntvnode_slock;

/*
 * This macro is very helpful in defining those offsets in the vdesc struct.
 *
 * This is stolen from X11R4.  I ingored all the fancy stuff for
 * Crays, so if you decide to port this to such a serious machine,
 * you might want to consult Intrisics.h's XtOffset{,Of,To}.
 */
#define VOPARG_OFFSET(p_type, field) 							\
	((int) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))
#define VOPARG_OFFSETOF(s_type, field) 		VOPARG_OFFSET(s_type*, field)
#define VOPARG_OFFSETTO(S_TYPE, S_OFFSET, STRUCT_P) 			\
	((S_TYPE)(((char*)(STRUCT_P))+(S_OFFSET)))

struct vop_generic_args;
typedef int (*vocall_func_t)(struct vop_generic_args *);

#define VOCALL(VOPS, AP)	(*(vocall_func_t *)((char *)(VOPS)+((AP)->a_desc->vdesc_offset)))(AP)
#define VCALL(VP, AP) 		(VOCALL((VP)->v_op,(AP)))
#define VDESC(OP) 			(&(OP##_desc))
#define VOFFSET(OP) 		(VDESC(OP)->vdesc_offset)

/*
 * Finally, include the default set of vnode operations.
 */
#include <sys/vnode_if.h>

/*
 * Public vnode manipulation functions.
 */
struct file;
struct flock;
struct mount;
struct nameidata;
struct ostat;
struct proc;
struct stat;
struct ucred;
struct uio;
enum uio_rw;
enum uio_seg;
struct vattr;
struct vnode;
struct vop_bwrite_args;

int 	bdevvp(dev_t, struct vnode **);
int		cdevvp(dev_t, struct vnode **);
void	cvtstat(struct stat *, struct ostat *);
int		vfinddev(dev_t, enum vtype, struct vnode **);
int 	getnewvnode(enum vtagtype, struct mount *, struct vnodeops *, struct vnode **);
void	insmntque(struct vnode *, struct mount *);
void 	vattr_null(struct vattr *);
void	vdevgone(int, int, int, enum vtype);
int 	vcount(struct vnode *);
int		vflush(struct mount *, struct vnode *, int);
void	vntblinit(void);
int 	vget(struct vnode *, int, struct proc *);
int		vrecycle(struct vnode *, struct lock_object *, struct proc *);
void 	vgone(struct vnode *);
void	vgonel(struct vnode *, struct proc *);
int		vaccess(enum vtype, mode_t, uid_t, gid_t, int, struct ucred *);
int		vinvalbuf(struct vnode *, int, struct ucred *, struct proc *, int, int);
void	vprint(char *, struct vnode *);
int		vn_bwrite(struct vop_bwrite_args *);
int 	vn_close(struct vnode *, int, struct ucred *, struct proc *);
int		vn_rw(struct file *, struct uio *, struct ucred *);
int 	vn_closefile(struct file *, struct proc *);
int		vn_ioctl(struct file *, u_long, void *, struct proc *);
int		vn_lock(struct vnode *, int, struct proc *);
int		vn_marktext(struct vnode *);
int 	vn_open(struct nameidata *, int, int);
int 	vn_rdwr(enum uio_rw, struct vnode *, caddr_t, int, off_t, enum uio_seg, int, struct ucred *, int *, struct proc *);
int		vn_read(struct file *, struct uio *, struct ucred *);
int		vn_poll(struct file *, int, struct proc *);
int		vn_kqfilter(struct file *, struct knote *);
int		vn_stat(struct vnode *, struct stat *, struct proc *);
int		vn_fstat(struct file *, struct stat *, struct proc *);
int		vn_write(struct file *, struct uio *, struct ucred *);
int		vn_writechk(struct vnode *);
struct 	vnode *checkalias(struct vnode *, dev_t, struct mount *);
void 	vput(struct vnode *);
void 	vref(struct vnode *);
void 	vrele(struct vnode *);

void    vfs_bufstats(void);
#endif /* KERNEL */
#endif /* !_SYS_VNODE_H_ */
