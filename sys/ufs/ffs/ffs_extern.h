/*-
 * Copyright (c) 1991, 1993, 1994
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
 *	@(#)ffs_extern.h	8.6 (Berkeley) 3/30/95
 */

/*
 * Sysctl values for the fast filesystem.
 */
#define FFS_CLUSTERREAD		1	/* cluster reading enabled */
#define FFS_CLUSTERWRITE	2	/* cluster writing enabled */
#define FFS_REALLOCBLKS		3	/* block reallocation enabled */
#define FFS_ASYNCFREE		4	/* asynchronous block freeing enabled */
#define	FFS_MAXID		5	/* number of valid ffs ids */

#define FFS_NAMES { \
	{ 0, 0 }, \
	{ "doclusterread", CTLTYPE_INT }, \
	{ "doclusterwrite", CTLTYPE_INT }, \
	{ "doreallocblks", CTLTYPE_INT }, \
	{ "doasyncfree", CTLTYPE_INT }, \
}

struct buf;
struct fid;
struct fs;
struct inode;
struct mount;
struct nameidata;
struct proc;
struct statfs;
struct timeval;
struct ucred;
struct uio;
struct vnode;
struct mbuf;
struct vfsconf;

__BEGIN_DECLS
int	ffs_alloc (struct inode *, ufs_daddr_t, ufs_daddr_t, int, struct ucred *, ufs_daddr_t *);
int	ffs_balloc (struct inode *,
	    ufs_daddr_t, int, struct ucred *, struct buf **, int);
int	ffs_blkatoff (struct vop_blkatoff_args *);
int	ffs_blkfree (struct inode *, ufs_daddr_t, long);
ufs_daddr_t ffs_blkpref (struct inode *, ufs_daddr_t, int, ufs_daddr_t *);
int	ffs_bmap (struct vop_bmap_args *);
void	ffs_clrblock (struct fs *, u_char *, ufs_daddr_t);
int	ffs_fhtovp (struct mount *, struct fid *, struct mbuf *,
	    struct vnode **, int *, struct ucred **);
void	ffs_fragacct (struct fs *, int, int32_t [], int);
int	ffs_fsync (struct vop_fsync_args *);
int	ffs_init (struct vfsconf *);
int	ffs_isblock (struct fs *, u_char *, ufs_daddr_t);
int	ffs_mount (struct mount *,
	    char *, caddr_t, struct nameidata *, struct proc *);
int	ffs_mountfs (struct vnode *, struct mount *, struct proc *);
int	ffs_mountroot (void);
int	ffs_read (struct vop_read_args *);
int	ffs_reallocblks (struct vop_reallocblks_args *);
int	ffs_realloccg (struct inode *,
	    ufs_daddr_t, ufs_daddr_t, int, int, struct ucred *, struct buf **);
int	ffs_reclaim (struct vop_reclaim_args *);
void	ffs_setblock (struct fs *, u_char *, ufs_daddr_t);
int	ffs_statfs (struct mount *, struct statfs *, struct proc *);
int	ffs_sync (struct mount *, int, struct ucred *, struct proc *);
int	ffs_sysctl (int *, u_int, void *, size_t *, void *, size_t,
	    struct proc *);
int	ffs_truncate (struct vop_truncate_args *);
int	ffs_unmount (struct mount *, int, struct proc *);
int	ffs_update (struct vop_update_args *);
int	ffs_valloc (struct vop_valloc_args *);
int	ffs_vfree (struct vop_vfree_args *);
int	ffs_vget (struct mount *, ino_t, struct vnode **);
int	ffs_vptofh (struct vnode *, struct fid *);
int	ffs_write (struct vop_write_args *);

int	bwrite ();		/* FFS needs a bwrite routine.  XXX */

#ifdef DIAGNOSTIC
void	ffs_checkoverlap (struct buf *, struct inode *);
#endif
__END_DECLS

extern struct ffs_vnodeops;
extern struct ffs_specops;
#ifdef FIFO
extern struct ffs_fifoops;
#define FFS_FIFOOPS ffs_fifoop_p
#else
#define FFS_FIFOOPS NULL
#endif
