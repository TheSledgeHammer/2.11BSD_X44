/*
 * Copyright (c) 1986, 1989, 1991, 1993, 1995
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
 *	@(#)lfs_vnops.c	8.13 (Berkeley) 6/10/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <vm/include/vm.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>
#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>

/* Global vfs data structures for lfs. */
struct vnodeops lfs_vnodeops = {
		.vop_default = vop_default_error,/* default */
		.vop_lookup = ufs_lookup,		/* lookup */
		.vop_create = ufs_create,		/* create */
		.vop_mknod = ufs_mknod,			/* mknod */
		.vop_open = ufs_open,			/* open */
		.vop_close = lfs_close,			/* close */
		.vop_access = ufs_access,		/* access */
		.vop_getattr = lfs_getattr,		/* getattr */
		.vop_setattr = ufs_setattr,		/* setattr */
		.vop_read = lfs_read,			/* read */
		.vop_write = lfs_write,			/* write */
		.vop_lease = ufs_lease_check,	/* lease */
		.vop_ioctl = ufs_ioctl,			/* ioctl */
		.vop_select = ufs_select,		/* select */
		.vop_poll = ufs_poll,			/* poll */
		.vop_revoke = ufs_revoke,		/* revoke */
		.vop_mmap = ufs_mmap,			/* mmap */
		.vop_fsync = lfs_fsync,			/* fsync */
		.vop_seek = ufs_seek,			/* seek */
		.vop_remove = ufs_remove,		/* remove */
		.vop_link = ufs_link,			/* link */
		.vop_rename = ufs_rename,		/* rename */
		.vop_mkdir = ufs_mkdir,			/* mkdir */
		.vop_rmdir = ufs_rmdir,			/* rmdir */
		.vop_symlink = ufs_symlink,		/* symlink */
		.vop_readdir = ufs_readdir,		/* readdir */
		.vop_readlink = ufs_readlink,	/* readlink */
		.vop_abortop = ufs_abortop,		/* abortop */
		.vop_inactive = ufs_inactive,	/* inactive */
		.vop_reclaim = lfs_reclaim,		/* reclaim */
		.vop_lock = ufs_lock,			/* lock */
		.vop_unlock = ufs_unlock,		/* unlock */
		.vop_bmap = ufs_bmap,			/* bmap */
		.vop_strategy = ufs_strategy,	/* strategy */
		.vop_print = ufs_print,			/* print */
		.vop_islocked = ufs_islocked,	/* islocked */
		.vop_pathconf = ufs_pathconf,	/* pathconf */
		.vop_advlock = ufs_advlock,		/* advlock */
		.vop_blkatoff = lfs_blkatoff,	/* blkatoff */
		.vop_valloc = lfs_valloc,		/* valloc */
		.vop_vfree = lfs_vfree,			/* vfree */
		.vop_truncate = lfs_truncate,	/* truncate */
		.vop_update = lfs_update,		/* update */
		.vop_bwrite = lfs_bwrite,		/* bwrite */
};

struct vnodeops lfs_specops = {
		.vop_default = vop_default_error,/* default */
		.vop_lookup = spec_lookup,		/* lookup */
		.vop_create = spec_create,		/* create */
		.vop_mknod = spec_mknod,		/* mknod */
		.vop_open = spec_open,			/* open */
		.vop_close = ufsspec_close,		/* close */
		.vop_access = ufs_access,		/* access */
		.vop_getattr = lfs_getattr,		/* getattr */
		.vop_setattr = ufs_setattr,		/* setattr */
		.vop_read = ufsspec_read,		/* read */
		.vop_write = ufsspec_write,		/* write */
		.vop_lease = spec_lease_check,	/* lease */
		.vop_ioctl = spec_ioctl,		/* ioctl */
		.vop_select = spec_select,		/* select */
		.vop_poll = spec_poll,			/* poll */
		.vop_revoke = spec_revoke,		/* revoke */
		.vop_mmap = spec_mmap,			/* mmap */
		.vop_fsync = spec_fsync,		/* fsync */
		.vop_seek = spec_seek,			/* seek */
		.vop_remove = spec_remove,		/* remove */
		.vop_link = spec_link,			/* link */
		.vop_rename = spec_rename,		/* rename */
		.vop_mkdir = spec_mkdir,		/* mkdir */
		.vop_rmdir = spec_rmdir,		/* rmdir */
		.vop_symlink = spec_symlink,	/* symlink */
		.vop_readdir = spec_readdir,	/* readdir */
		.vop_readlink = spec_readlink,	/* readlink */
		.vop_abortop = spec_abortop,	/* abortop */
		.vop_inactive = ufs_inactive,	/* inactive */
		.vop_reclaim = lfs_reclaim,		/* reclaim */
		.vop_lock = ufs_lock,			/* lock */
		.vop_unlock = ufs_unlock,		/* unlock */
		.vop_bmap = spec_bmap,			/* bmap */
		.vop_strategy = spec_strategy,	/* strategy */
		.vop_print = ufs_print,			/* print */
		.vop_islocked = ufs_islocked,	/* islocked */
		.vop_pathconf = spec_pathconf,	/* pathconf */
		.vop_advlock = spec_advlock,	/* advlock */
		.vop_blkatoff = spec_blkatoff,	/* blkatoff */
		.vop_valloc = spec_valloc,		/* valloc */
		.vop_vfree = lfs_vfree,			/* vfree */
		.vop_truncate = spec_truncate,	/* truncate */
		.vop_update = lfs_update,		/* update */
		.vop_bwrite = lfs_bwrite,		/* bwrite */
};

#ifdef FIFO
struct vnodeops lfs_fifoops = {
		.vop_default = vop_default_error,/* default */
		.vop_lookup = fifo_lookup,		/* lookup */
		.vop_create = fifo_create,		/* create */
		.vop_mknod = fifo_mknod,		/* mknod */
		.vop_open = fifo_open,			/* open */
		.vop_close = ufsfifo_close,		/* close */
		.vop_access = ufs_access,		/* access */
		.vop_getattr = lfs_getattr,		/* getattr */
		.vop_setattr = ufs_setattr,		/* setattr */
		.vop_read = ufsfifo_read,		/* read */
		.vop_write = ufsfifo_write,		/* write */
		.vop_lease = fifo_lease_check,	/* lease */
		.vop_ioctl = fifo_ioctl,		/* ioctl */
		.vop_select = fifo_select,		/* select */
		.vop_poll = fifo_poll,			/* poll */
		.vop_revoke = fifo_revoke,		/* revoke */
		.vop_mmap = fifo_mmap,			/* mmap */
		.vop_fsync = fifo_fsync,		/* fsync */
		.vop_seek = fifo_seek,			/* seek */
		.vop_remove = fifo_remove,		/* remove */
		.vop_link = fifo_link,			/* link */
		.vop_rename = fifo_rename,		/* rename */
		.vop_mkdir = fifo_mkdir,		/* mkdir */
		.vop_rmdir = fifo_rmdir,		/* rmdir */
		.vop_symlink = fifo_symlink,	/* symlink */
		.vop_readdir = fifo_readdir,	/* readdir */
		.vop_readlink = fifo_readlink,	/* readlink */
		.vop_abortop = fifo_abortop,	/* abortop */
		.vop_inactive = ufs_inactive,	/* inactive */
		.vop_reclaim = lfs_reclaim,		/* reclaim */
		.vop_lock = ufs_lock,			/* lock */
		.vop_unlock = ufs_unlock,		/* unlock */
		.vop_bmap = fifo_bmap,			/* bmap */
		.vop_strategy = fifo_strategy,	/* strategy */
		.vop_print = ufs_print,			/* print */
		.vop_islocked = ufs_islocked,	/* islocked */
		.vop_pathconf = fifo_pathconf,	/* pathconf */
		.vop_advlock = fifo_advlock,	/* advlock */
		.vop_blkatoff = fifo_blkatoff,	/* blkatoff */
		.vop_valloc = fifo_valloc,		/* valloc */
		.vop_vfree = lfs_vfree,			/* vfree */
		.vop_truncate = fifo_truncate,	/* truncate */
		.vop_update = lfs_update,		/* update */
		.vop_bwrite = lfs_bwrite,		/* bwrite */
};
#endif /* FIFO */

#define	LFS_READWRITE
#include <ufs/ufs/ufs_readwrite.c>
#undef	LFS_READWRITE

/*
 * Synch an open file.
 */
/* ARGSUSED */
int
lfs_fsync(ap)
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_waitfor;
		struct proc *a_p;
	} */ *ap;
{
	struct timeval tv;

	tv = time;
	return (VOP_UPDATE(ap->a_vp, &tv, &tv, ap->a_waitfor == MNT_WAIT ? LFS_SYNC : 0));
}

/*
 * These macros are used to bracket UFS directory ops, so that we can
 * identify all the pages touched during directory ops which need to
 * be ordered and flushed atomically, so that they may be recovered.
 */
#define	SET_DIROP(fs) {							\
	if ((fs)->lfs_writer)						\
		tsleep(&(fs)->lfs_dirops, PRIBIO + 1, "lfs_dirop", 0);	\
	++(fs)->lfs_dirops;							\
	(fs)->lfs_doifile = 1;						\
}

#define	SET_ENDOP(fs) {							\
	--(fs)->lfs_dirops;							\
	if (!(fs)->lfs_dirops)						\
		wakeup(&(fs)->lfs_writer);				\
}

#define	MARK_VNODE(dvp)	(dvp)->v_flag |= VDIROP

int
lfs_symlink(ap)
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap;
{
	struct inode *ip;
	int ret;

	ip = VTOI(ap->a_dvp);
	SET_DIROP(ip->i_lfs);
	MARK_VNODE(ap->a_dvp);
	ret = ufs_symlink(ap);
	SET_ENDOP(ip->i_lfs);
	return (ret);
}

int
lfs_mknod(ap)
	struct vop_mknod_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	struct inode *ip;
	int ret;

	ip = VTOI(ap->a_dvp);
	SET_DIROP(ip->i_lfs);
	MARK_VNODE(ap->a_dvp);
	ret = ufs_mknod(ap);
	SET_ENDOP(ip->i_lfs);
	return (ret);
}

int
lfs_create(ap)
	struct vop_create_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	struct inode *ip;
	int ret;

	ip = VTOI(ap->a_dvp);
	SET_DIROP(ip->i_lfs);
	MARK_VNODE(ap->a_dvp);
	ret = ufs_create(ap);
	SET_ENDOP(ip->i_lfs);
	return (ret);
}

int
lfs_mkdir(ap)
	struct vop_mkdir_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	struct inode *ip;
	int ret;

	ip = VTOI(ap->a_dvp);
	SET_DIROP(ip->i_lfs);
	MARK_VNODE(ap->a_dvp);
	ret = ufs_mkdir(ap);
	SET_ENDOP(ip->i_lfs);
	return (ret);
}

int
lfs_remove(ap)
	struct vop_remove_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap;
{
	struct inode *ip;
	int ret;

	ip = VTOI(ap->a_dvp);
	SET_DIROP(ip->i_lfs);
	MARK_VNODE(ap->a_dvp);
	MARK_VNODE(ap->a_vp);
	ret = ufs_remove(ap);
	SET_ENDOP(ip->i_lfs);
	return (ret);
}

int
lfs_rmdir(ap)
	struct vop_rmdir_args /* {
		struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap;
{
	struct inode *ip;
	int ret;

	ip = VTOI(ap->a_dvp);
	SET_DIROP(ip->i_lfs);
	MARK_VNODE(ap->a_dvp);
	MARK_VNODE(ap->a_vp);
	ret = ufs_rmdir(ap);
	SET_ENDOP(ip->i_lfs);
	return (ret);
}

int
lfs_link(ap)
	struct vop_link_args /* {
		struct vnode *a_vp;
		struct vnode *a_tdvp;
		struct componentname *a_cnp;
	} */ *ap;
{
	struct inode *ip;
	int ret;

	ip = VTOI(ap->a_tdvp);
	SET_DIROP(ip->i_lfs);
	MARK_VNODE(ap->a_tdvp);
	ret = ufs_link(ap);
	SET_ENDOP(ip->i_lfs);
	return (ret);
}

int
lfs_rename(ap)
	struct vop_rename_args  /* {
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap;
{
	struct inode *ip;
	int ret;

	ip = VTOI(ap->a_fdvp);
	SET_DIROP(ip->i_lfs);
	MARK_VNODE(ap->a_fdvp);
	MARK_VNODE(ap->a_tdvp);
	ret = ufs_rename(ap);
	SET_ENDOP(ip->i_lfs);
	return (ret);
}
/* XXX hack to avoid calling ITIMES in getattr */
int
lfs_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	register struct vattr *vap = ap->a_vap;
	/*
	 * Copy from inode table
	 */
	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;
	vap->va_mode = ip->i_mode & ~IFMT;
	vap->va_nlink = ip->i_nlink;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	if (I_IS_UFS1(ip) || I_IS_UFS1_MOUNTED(ip)) {
		vap->va_rdev = ip->i_ffs1_rdev;
		vap->va_size = ip->i_ffs1_size;
		vap->va_atime.tv_sec = ip->i_ffs1_atime;
		vap->va_atime.tv_nsec = ip->i_ffs1_atimensec;
		vap->va_mtime.tv_sec = ip->i_ffs1_mtime;
		vap->va_mtime.tv_nsec = ip->i_ffs1_mtimensec;
		vap->va_ctime.tv_sec = ip->i_ffs1_ctime;
		vap->va_ctime.tv_nsec = ip->i_ffs1_ctimensec;
		vap->va_bytes = dbtob((u_quad_t)ip->i_ffs1_blocks);
	} else {
		vap->va_rdev = ip->i_ffs2_rdev;
		vap->va_size = ip->i_ffs2_size;
		vap->va_atime.tv_sec = ip->i_ffs2_atime;
		vap->va_atime.tv_nsec = ip->i_ffs2_atimensec;
		vap->va_mtime.tv_sec = ip->i_ffs2_mtime;
		vap->va_mtime.tv_nsec = ip->i_ffs2_mtimensec;
		vap->va_ctime.tv_sec = ip->i_ffs2_ctime;
		vap->va_ctime.tv_nsec = ip->i_ffs2_ctimensec;
		vap->va_bytes = dbtob((u_quad_t)ip->i_ffs2_blocks);
	}
	vap->va_flags = ip->i_flags;
	vap->va_gen = ip->i_gen;
	vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;
	vap->va_type = IFTOVT(ip->i_mode);
	vap->va_filerev = ip->i_modrev;
	return (0);
}
/*
 * Close called
 *
 * XXX -- we were using ufs_close, but since it updates the
 * times on the inode, we might need to bump the uinodes
 * count.
 */
/* ARGSUSED */
int
lfs_close(ap)
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct inode *ip = VTOI(vp);
	int mod;

	simple_lock(&vp->v_interlock);
	if (vp->v_usecount > 1) {
		mod = ip->i_flag & IN_MODIFIED;
		ITIMES(ip, &time, &time);
		if (!mod && (ip->i_flag & IN_MODIFIED))
			ip->i_lfs->lfs_uinodes++;
	}
	simple_unlock(&vp->v_interlock);
	return (0);
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int
lfs_reclaim(ap)
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	int error;

	if ((error = ufs_reclaim(vp, ap->a_p)))
		return (error);
	FREE(vp->v_data, M_LFSNODE);
	vp->v_data = NULL;
	return (0);
}
