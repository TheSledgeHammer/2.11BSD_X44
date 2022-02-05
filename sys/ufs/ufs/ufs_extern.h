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
 *	@(#)ufs_extern.h	8.10 (Berkeley) 5/14/95
 */

#ifndef _UFS_UFS_EXTERN_H_
#define	_UFS_UFS_EXTERN_H_

struct buf;
struct direct;
struct indir;
struct disklabel;
struct fid;
struct flock;
struct inode;
struct mbuf;
struct mount;
struct nameidata;
struct proc;
struct ucred;
struct ufid;
struct ufs_args;
struct uio;
struct vattr;
struct vfsconf;
struct vnode;

__BEGIN_DECLS
void 	diskerr(struct buf *, char *, char *, int, int, struct disklabel *);
void 	disksort(struct buf *, struct buf *);
u_int 	dkcksum(struct disklabel *);
char 	*readdisklabel(dev_t, int (*)(), struct disklabel *);
int 	setdisklabel(struct disklabel *, struct disklabel *, u_long);
int 	writedisklabel(dev_t, int (*)(), struct disklabel *);
int		ufs_check_export(struct mount *, struct ufid *, struct mbuf *, struct vnode **, int *, struct ucred **);
int		ufs_checkpath(struct inode *, struct inode *, struct ucred *);
void	ufs_dirbad(struct inode *, doff_t, char *);
int		ufs_dirbadentry(struct vnode *, struct direct *, int);
int		ufs_dirempty(struct inode *, ino_t, struct ucred *);
int		ufs_direnter(struct inode *, struct vnode *,struct componentname *);
int		ufs_dirremove(struct vnode *, struct componentname*);
int		ufs_dirrewrite(struct inode *, struct inode *, struct componentname *);
int	 	ufs_getlbns(struct vnode *, ufs2_daddr_t, struct indir *, int *);
struct vnode *ufs_ihashget (dev_t, ino_t);
void	ufs_ihashinit (void);
void	ufs_ihashins(struct inode *);
struct vnode *ufs_ihashlookup(dev_t, ino_t);
void	ufs_ihashrem(struct inode *);
int	 	ufs_init(struct vfsconf *);
int	 	ufs_makeinode(int mode, struct vnode *, struct vnode **, struct componentname *);
int	 	ufs_reclaim(struct vnode *, struct proc *);
int	 	ufs_root(struct mount *, struct vnode **);
int	 	ufs_start(struct mount *, int, struct proc *);
int	 	ufs_vinit(struct mount *, struct vnodeops *, struct vnodeops *, struct vnode **);
int 	ufs_abortop(struct vop_abortop_args *);
int		ufs_access(struct vop_access_args *);
int		ufs_advlock(struct vop_advlock_args *);
int		ufs_bmap(struct vop_bmap_args *);
int		ufs_close(struct vop_close_args *);
int		ufs_create(struct vop_create_args *);
int		ufs_getattr(struct vop_getattr_args *);
int	 	ufs_inactive(struct vop_inactive_args *);
int	 	ufs_ioctl(struct vop_ioctl_args *);
int	 	ufs_islocked(struct vop_islocked_args *);
int	 	ufs_link(struct vop_link_args *);
int	 	ufs_lock(struct vop_lock_args *);
int	 	ufs_lookup(struct vop_lookup_args *);
int	 	ufs_mkdir(struct vop_mkdir_args *);
int	 	ufs_mknod(struct vop_mknod_args *);
int	 	ufs_mmap(struct vop_mmap_args *);
int	 	ufs_open(struct vop_open_args *);
int	 	ufs_pathconf(struct vop_pathconf_args *);
int	 	ufs_print(struct vop_print_args *);
int	 	ufs_readdir(struct vop_readdir_args *);
int	 	ufs_readlink(struct vop_readlink_args *);
int	 	ufs_remove(struct vop_remove_args *);
int	 	ufs_rename(struct vop_rename_args *);
int	 	ufs_rmdir(struct vop_rmdir_args *);
int	 	ufs_seek(struct vop_seek_args *);
int	 	ufs_select(struct vop_select_args *);
int	 	ufs_setattr(struct vop_setattr_args *);
int	 	ufs_strategy(struct vop_strategy_args *);
int	 	ufs_symlink(struct vop_symlink_args *);
int	 	ufs_unlock(struct vop_unlock_args *);
int	 	ufs_whiteout(struct vop_whiteout_args *);
int	 	ufsspec_close(struct vop_close_args *);
int	 	ufsspec_read(struct vop_read_args *);
int	 	ufsspec_write(struct vop_write_args *);
#ifdef FIFO
int		ufsfifo_read(struct vop_read_args *);
int		ufsfifo_write(struct vop_write_args *);
int		ufsfifo_close(struct vop_close_args *);
#endif
__END_DECLS
#endif /* _UFS_UFS_EXTERN_H_ */
