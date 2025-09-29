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
 *	@(#)lfs_extern.h	8.6 (Berkeley) 5/8/95
 */

#ifndef _UFS_LFS_EXTERN_H
#define	_UFS_LFS_EXTERN_H

#ifdef _KERNEL

struct fid;
struct mount;
struct nameidata;
struct proc;
struct statfs;
struct timeval;
struct inode;
struct uio;
struct mbuf;

__BEGIN_DECLS
int	lfs_balloc(struct vnode *, int, u_long, daddr_t, struct buf **);
int	lfs_blkatoff(struct vop_blkatoff_args *);
int	lfs_bwrite(struct vop_bwrite_args *);
int	lfs_check(struct vnode *, ufs2_daddr_t);
int	lfs_close(struct vop_close_args *);
int	lfs_create(struct vop_create_args *);
int	lfs_fhtovp(struct mount *, struct fid *, struct mbuf *, struct vnode **, int *, struct ucred **);
int	lfs_fsync(struct vop_fsync_args *);
int	lfs_getattr(struct vop_getattr_args *);
//void *lfs_ifind(struct lfs *, ino_t, struct buf *);
struct ufs1_dinode *lfs1_ifind(struct lfs *, ino_t, struct buf *);
struct ufs2_dinode *lfs2_ifind(struct lfs *, ino_t, struct buf *);
int	lfs_inactive(struct vop_inactive_args *);
int	lfs_init(struct vfsconf *);
int	lfs_initseg(struct lfs *);
int	lfs_link(struct vop_link_args *);
int	lfs_makeinode(int, struct nameidata *, struct inode **);
int	lfs_mkdir(struct vop_mkdir_args *);
int	lfs_mknod(struct vop_mknod_args *);
int	lfs_mount(struct mount *, char *, caddr_t, struct nameidata *, struct proc *);
int	lfs_mountroot(void);
struct buf *
	 lfs_newbuf(struct vnode *, ufs1_daddr_t, size_t);
int	 lfs_read(struct vop_read_args *);
int	 lfs_reclaim(struct vop_reclaim_args *);
int	 lfs_remove(struct vop_remove_args *);
int	 lfs_rmdir(struct vop_rmdir_args *);
int  lfs_rename(struct vop_rename_args *);
void lfs_seglock(struct lfs *, unsigned long flags);
void lfs_segunlock(struct lfs *);
int	 lfs_segwrite(struct mount *, int);
int	 lfs_statfs(struct mount *, struct statfs *, struct proc *);
int	 lfs_symlink(struct vop_symlink_args *);
int	 lfs_sync(struct mount *, int, struct ucred *, struct proc *);
int	 lfs_truncate(struct vop_truncate_args *);
int	 lfs_unmount(struct mount *, int, struct proc *);
int	 lfs_update(struct vop_update_args *);
int	 lfs_valloc(struct vop_valloc_args *);
int	 lfs_vcreate(struct mount *, ino_t, struct vnode **);
int	 lfs_vfree(struct vop_vfree_args *);
int	 lfs_vflush(struct vnode *);
int	 lfs_vget(struct mount *, ino_t, struct vnode **);
int  lfs_vptofh(struct vnode *, struct fid *);
int	 lfs_vref(struct vnode *);
void lfs_vunref(struct vnode *);
int	 lfs_write(struct vop_write_args *);
void lfs_writesuper(struct lfs *);
void lfs_updatemeta(struct segment *);
int	 lfs_writeinode(struct lfs *, struct segment *, struct inode *);
int	 lfs_writeseg(struct lfs *, struct segment *);
int	 lfs_gatherblock(struct segment *, struct buf *, int *);

#ifdef DEBUG
void lfs_dump_dinode(void *);
void lfs_dump_super(struct lfs *);
//void lfs_dump_dinode(struct ufs1_dinode *);
#endif
__END_DECLS

extern int lfs_allclean_wakeup;
extern int lfs_mount_type;
extern struct vnodeops lfs_vnodeops;
extern struct vnodeops lfs_specops;
#ifdef FIFO
extern struct vnodeops lfs_fifoops;
#endif
#endif /* _KERNEL */

__BEGIN_DECLS
u_long	cksum(void *, size_t);				/* XXX */
__END_DECLS
#endif /* !_UFS_LFS_EXTERN_H */
