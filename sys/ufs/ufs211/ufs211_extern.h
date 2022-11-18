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

int				 		updlock;			/* lock for sync */
daddr_t	 				rablock;			/* block to be read ahead */

struct ufs211_args {
	struct mount 	    *ufs211_vfs;
	struct vnode 	    *ufs211_rootvp;	    /* block device mounted vnode */
};

/* see ufs211_bufmap.c */
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
struct flock;
struct mbuf;
struct nameidata;
struct proc;
struct ucred;
struct uio;
struct vattr;
struct vfsconf;
struct vnode;
struct ufs211_inode;
struct ufs211_mount;
struct ufs211_xmount;
struct ufs211_args;
struct ufs211_fs;
__BEGIN_DECLS
/* ufs211 bufmap */
void 		ufs211_buffmap_init(void);
void 		ufs211_mapin(struct buf *);		/* allocate to the buffer (* buffer only) */
void 		ufs211_mapout(struct buf *); 	/* free from the buffer (* buffer only) */
static void *bufmap_alloc(void *, long);	/* allocate to dedicated ufs211 space (generic) */
static void *bufmap_free(void *);			/* free from the dedicated ufs211 space (generic) */

/* ufs211 */
struct buf 	*ufs211_balloc(struct ufs211_inode *, int);
void		ufs211_bfree(struct ufs211_inode *, daddr_t);
void		ufs211_fserr(struct ufs211_fs *, char *);
daddr_t		ufs211_bmap1(struct ufs211_inode *, daddr_t, int, int);
int		ufs211_makeinode(int, struct vnode *, struct vnode **, struct componentname *);
void 		ufs211_trsingle(struct ufs211_inode *, caddr_t, daddr_t, int);
void 		ufs211_dirbad(struct ufs211_inode *, off_t, char *);
int 		ufs211_dirbadentry(struct direct *, int);
int 		ufs211_direnter(struct ufs211_inode *, struct direct *, struct vnode *, struct componentname *);
int 		ufs211_dirremove(struct vnode *, struct componentname *);
int 		ufs211_dirrewrite(struct ufs211_inode *, struct ufs211_inode *, struct componentname *);
int 		ufs211_dirempty(struct ufs211_inode *, ino_t);
int 		ufs211_checkpath(struct ufs211_inode *, struct ufs211_inode *);
void	 	ufs211_syncip(struct vnode *);
int			ufs211_badblock(struct ufs211_fs *, daddr_t);
int 		ufs211_init(struct vfsconf *);
void		ufs211_ihinit();
void		ufs211_ihashins(struct ufs211_inode *);
void		ufs211_ihashrem(struct ufs211_inode *);

int ufs211_lookup(struct vop_lookup_args *);
int ufs211_create(struct vop_create_args *);
int ufs211_whiteout(struct vop_whiteout_args *);
int ufs211_open(struct vop_open_args *);
int ufs211_close(struct vop_close_args *);
int ufs211_access(struct vop_access_args *);
int ufs211_getattr(struct vop_getattr_args *);
int ufs211_setattr(struct vop_setattr_args *);
int ufs211_read(struct vop_read_args *);
int ufs211_write(struct vop_write_args *);
int ufs211_fsync(struct vop_fsync_args *);
int ufs211_remove(struct vop_remove_args *);
int ufs211_rename(struct vop_rename_args *);
int ufs211_readdir(struct vop_readdir_args *);
int ufs211_inactive(struct vop_inactive_args *);
int ufs211_reclaim(struct vop_reclaim_args *);
int ufs211_bmap(struct vop_bmap_args *);
int ufs211_strategy(struct vop_strategy_args *);
int ufs211_print(struct vop_print_args *);
int ufs211_advlock(struct vop_advlock_args *);
int ufs211_pathconf(struct vop_pathconf_args *);
int ufs211_link(struct vop_link_args *);
int ufs211_symlink(struct vop_symlink_args *);
int ufs211_readlink(struct vop_readlink_args *);
int ufs211_mkdir(struct vop_mkdir_args *);
int ufs211_rmdir(struct vop_rmdir_args *);
int ufs211_mknod(struct vop_mknod_args *);
int	ufs211spec_close(struct vop_close_args *);
int	ufs211spec_read(struct vop_read_args *);
int ufs211spec_write(struct vop_write_args *);
#ifdef FIFO
int	ufs211fifo_read(struct vop_read_args *);
int	ufs211fifo_write(struct vop_write_args *);
int	ufs211fifo_close(struct vop_close_args *);
#endif
__END_DECLS

extern struct vnodeops ufs211_vnodeops;
extern struct vnodeops ufs211_specops;
#ifdef FIFO
extern struct vnodeops ufs211_fifoops;
#endif

#endif /* _UFS_UFS_EXTERN_H_ */
