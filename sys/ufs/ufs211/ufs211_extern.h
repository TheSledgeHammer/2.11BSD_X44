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
#ifndef _UFS_UFS211_EXTERN_H_
#define	_UFS_UFS211_EXTERN_H_

//int				 		ufs211_updlock;			/* lock for sync */
extern daddr_t	 			ufs211_rablock;			/* block to be read ahead */

/* buffer map */
struct ufs211_bufmap {
	void 				*bm_data;			/* data */
	long				bm_size;			/* sizeof data */
};

//#define	DEV_BSHIFT		10					/* log2(DEV_BSIZE) */
#define	DEV_BMASK		(DEV_BSIZE - 1)

struct buf;
struct componentname;
struct direct;
struct disklabel;
struct ufid;
struct fid;
struct flock;
struct mbuf;
struct nameidata;
struct proc;
struct statfs;
struct ucred;
struct uio;
struct vattr;
struct vfsconf;
struct vnode;
struct ufs211_inode;
struct ufs211_mount;
struct ufs211_args;
struct ufs211_fs;

__BEGIN_DECLS
/* ufs211 bufmap */
void        ufs211_bufmap_init(void);
void 		ufs211_mapin(void *);			/* allocate to the buffer (* buffer only) */
void 		ufs211_mapout(void *); 			/* free from the buffer (* buffer only) */

/* ufs211 */
struct buf 	*ufs211_balloc(struct ufs211_inode *, int);
void		ufs211_bfree(struct ufs211_inode *, daddr_t);
void		ufs211_fserr(struct ufs211_fs *, char *);
daddr_t		ufs211_bmap1(struct ufs211_inode *, daddr_t, int, int);
int			ufs211_makeinode(int, struct vnode *, struct vnode **, struct componentname *);
void		ufs211_trunc(struct ufs211_inode *, u_long, int);
void 		ufs211_trsingle(struct ufs211_inode *, caddr_t, daddr_t, int);
void		ufs211_updat(struct ufs211_inode *, volatile struct timeval *, volatile struct timeval *, int);
void 		ufs211_dirbad(struct ufs211_inode *, off_t, char *);
int 		ufs211_dirbadentry(struct vnode *, struct direct *, int);
int 		ufs211_direnter(struct ufs211_inode *, struct vnode *, struct componentname *);
int			ufs211_direnter2(struct ufs211_inode *, struct direct *, struct vnode *, struct componentname *);
int 		ufs211_dirremove(struct vnode *, struct componentname *);
int 		ufs211_dirrewrite(struct ufs211_inode *, struct ufs211_inode *, struct componentname *);
int 		ufs211_dirempty(struct ufs211_inode *, ino_t);
int 		ufs211_checkpath(struct ufs211_inode *, struct ufs211_inode *);
void	 	ufs211_syncip(struct vnode *);
int			ufs211_badblock(struct ufs211_fs *, daddr_t);
int 		ufs211_init(struct vfsconf *);
struct vnode *ufs211_ihashget(dev_t, ino_t);
void		ufs211_ihinit(void);
void		ufs211_ihashins(struct ufs211_inode *);
void		ufs211_ihashrem(struct ufs211_inode *);
struct vnode *ufs211_ihashfind(dev_t, ino_t);
int	 	ufs211_init(struct vfsconf *);
int	 	ufs211_root(struct mount *, struct vnode **);
int	 	ufs211_start(struct mount *, int, struct proc *);
int	 	ufs211_vinit(struct mount *, struct vnodeops *, struct vnodeops *, struct vnode **);
int		ufs211_mount(struct mount *, char *, caddr_t, struct nameidata *, struct proc *);
int		ufs211_mountfs(struct vnode *, struct mount *, struct proc *);
int		ufs211_mountroot(void);
int		ufs211_statfs(struct mount *, struct statfs *, struct proc *);
int		ufs211_sync(struct mount *, int, struct ucred *, struct proc *);
int		ufs211_sysctl(int *, u_int, void *, size_t *, void *, size_t, struct proc *);
int		ufs211_unmount(struct mount *, int, struct proc *);
int		ufs211_vget(struct mount *, ino_t, struct vnode **);
int		ufs211_vptofh(struct vnode *, struct fid *);
int		ufs211_fhtovp(struct mount *, struct fid *, struct mbuf *, struct vnode **, int *, struct ucred **);
int		ufs211_flushfiles(struct mount *, int, struct proc *);
void	ufs211_quotainit(void);
int     ufs211_chmod1(struct vnode *, int);
int			ufs211_chown1(struct ufs211_inode *, uid_t, gid_t);
#ifdef NFS
int	 	lease_check (struct vop_lease_args *);
#define	 	ufs211_lease_check lease_check
#else
#define	 	ufs211_lease_check ((int (*) (struct vop_lease_args *))nullop)
#endif
#define		ufs211_revoke ((int (*) (struct vop_revoke_args *))vop_norevoke)
#define     ufs211_reallocblks ((int (*) (struct  vop_reallocblks_args *))eopnotsupp)
int 	ufs211_abortop(struct vop_abortop_args *);
int		ufs211_access(struct vop_access_args *);
int		ufs211_advlock(struct vop_advlock_args *);
int		ufs211_bmap(struct vop_bmap_args *);
int		ufs211_close(struct vop_close_args *);
int		ufs211_create(struct vop_create_args *);
int		ufs211_getattr(struct vop_getattr_args *);
int	 	ufs211_inactive(struct vop_inactive_args *);
int	 	ufs211_ioctl(struct vop_ioctl_args *);
int	 	ufs211_islocked(struct vop_islocked_args *);
int	 	ufs211_link(struct vop_link_args *);
int	 	ufs211_lock(struct vop_lock_args *);
int	 	ufs211_lookup(struct vop_lookup_args *);
int	 	ufs211_mkdir(struct vop_mkdir_args *);
int	 	ufs211_mknod(struct vop_mknod_args *);
int	 	ufs211_mmap(struct vop_mmap_args *);
int	 	ufs211_open(struct vop_open_args *);
int	 	ufs211_pathconf(struct vop_pathconf_args *);
int	 	ufs211_print(struct vop_print_args *);
int	 	ufs211_readdir(struct vop_readdir_args *);
int	 	ufs211_readlink(struct vop_readlink_args *);
int	 	ufs211_remove(struct vop_remove_args *);
int	 	ufs211_rename(struct vop_rename_args *);
int	 	ufs211_rmdir(struct vop_rmdir_args *);
int	 	ufs211_seek(struct vop_seek_args *);
int	 	ufs211_select(struct vop_select_args *);
int	 	ufs211_poll(struct vop_poll_args *);
int	 	ufs211_setattr(struct vop_setattr_args *);
int	 	ufs211_strategy(struct vop_strategy_args *);
int	 	ufs211_symlink(struct vop_symlink_args *);
int	 	ufs211_unlock(struct vop_unlock_args *);
int	 	ufs211_whiteout(struct vop_whiteout_args *);
int		ufs211_blkatoff(struct vop_blkatoff_args *);
int		ufs211_fsync(struct vop_fsync_args *);
int		ufs211_read(struct vop_read_args *);
int		ufs211_reclaim(struct vop_reclaim_args *);
int		ufs211_truncate(struct vop_truncate_args *);
int		ufs211_update(struct vop_update_args *);
int		ufs211_valloc(struct vop_valloc_args *);
int		ufs211_vfree(struct vop_vfree_args *);
int		ufs211_write(struct vop_write_args *);
int	    ufs211spec_close(struct vop_close_args *);
int 	ufs211spec_read(struct vop_read_args *);
int     ufs211spec_write(struct vop_write_args *);
#ifdef FIFO
int	    ufs211fifo_read(struct vop_read_args *);
int	    ufs211fifo_write(struct vop_write_args *);
int	    ufs211fifo_close(struct vop_close_args *);
#endif
__END_DECLS

extern struct vnodeops ufs211_vnodeops;
extern struct vnodeops ufs211_specops;
#ifdef FIFO
extern struct vnodeops ufs211_fifoops;
#endif

#endif /* _UFS_UFS211_EXTERN_H_ */
