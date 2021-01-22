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
 * @(#)ufs211_vnops.c	1.00
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/user.h>
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
#include <sys/dirent.h>
#include <sys/unistd.h>

#include <vm/include/vm.h>
#include <miscfs/specfs/specdev.h>

#include "ufs211/ufs211_dir.h"
#include "ufs211/ufs211_extern.h"
#include "ufs211/ufs211_fs.h"
#include "ufs211/ufs211_inode.h"
#include "ufs211/ufs211_mount.h"
#include "ufs211/ufs211_quota.h"

struct vnodeops ufs211_vnodeops = {
		.vop_lookup = ufs211_lookup,		/* lookup */
		.vop_create = ufs211_create,		/* create */
		.vop_mknod = ufs211_mknod,			/* mknod */
		.vop_open = ufs211_open,			/* open */
		.vop_close = ufs211_close,			/* close */
		.vop_access = ufs211_access,		/* access */
		.vop_getattr = ufs211_getattr,		/* getattr */
		.vop_setattr = ufs211_setattr,		/* setattr */
		.vop_read = ufs211_read,			/* read */
		.vop_write = ufs211_write,			/* write */
		.vop_lease = ufs211_lease_check,	/* lease */
		.vop_ioctl = ufs211_ioctl,			/* ioctl */
		.vop_select = ufs211_select,		/* select */
		.vop_revoke = ufs211_revoke,		/* revoke */
		.vop_mmap = ufs211_mmap,			/* mmap */
		.vop_fsync = ufs211_fsync,			/* fsync */
		.vop_seek = ufs211_seek,			/* seek */
		.vop_remove = ufs211_remove,		/* remove */
		.vop_link = ufs211_link,			/* link */
		.vop_rename = ufs211_rename,		/* rename */
		.vop_mkdir = ufs211_mkdir,			/* mkdir */
		.vop_rmdir = ufs211_rmdir,			/* rmdir */
		.vop_symlink = ufs211_symlink,		/* symlink */
		.vop_readdir = ufs211_readdir,		/* readdir */
		.vop_readlink = ufs211_readlink,	/* readlink */
		.vop_abortop = ufs211_abortop,		/* abortop */
		.vop_inactive = ufs211_inactive,	/* inactive */
		.vop_reclaim = ufs211_reclaim,		/* reclaim */
		.vop_lock = ufs211_lock,			/* lock */
		.vop_unlock = ufs211_unlock,		/* unlock */
		.vop_bmap = ufs211_bmap,			/* bmap */
		.vop_strategy = ufs211_strategy,	/* strategy */
		.vop_print = ufs211_print,			/* print */
		.vop_islocked = ufs211_islocked,	/* islocked */
		.vop_pathconf = ufs211_pathconf,	/* pathconf */
		.vop_advlock = ufs211_advlock,		/* advlock */
		.vop_blkatoff = ufs211_blkatoff,	/* blkatoff */
		.vop_valloc = ufs211_valloc,		/* valloc */
		.vop_vfree = ufs211_vfree,			/* vfree */
		.vop_truncate = ufs211_truncate,	/* truncate */
		.vop_update = ufs211_update,		/* update */
		.vop_bwrite = ufs211_bwrite,		/* bwrite */
		(struct vnodeops *)NULL = (int(*)())NULL
};

struct vnodeops ufs211_specops = {
		.vop_lookup = spec_lookup,			/* lookup */
		.vop_create = spec_create,			/* create */
		.vop_mknod = spec_mknod,			/* mknod */
		.vop_open = spec_open,				/* open */
		.vop_close = ufs211_close,			/* close */
		.vop_access = ufs211_access,		/* access */
		.vop_getattr = ufs211_getattr,		/* getattr */
		.vop_setattr = ufs211_setattr,		/* setattr */
		.vop_read = ufs211_read,			/* read */
		.vop_write = ufs211_write,			/* write */
		.vop_lease = spec_lease_check,		/* lease */
		.vop_ioctl = spec_ioctl,			/* ioctl */
		.vop_select = spec_select,			/* select */
		.vop_revoke = spec_revoke,			/* revoke */
		.vop_mmap = spec_mmap,				/* mmap */
		.vop_fsync = spec_fsync,			/* fsync */
		.vop_seek = spec_seek,				/* seek */
		.vop_remove = spec_remove,			/* remove */
		.vop_link = spec_link,				/* link */
		.vop_rename = spec_rename,			/* rename */
		.vop_mkdir = spec_mkdir,			/* mkdir */
		.vop_rmdir = spec_rmdir,			/* rmdir */
		.vop_symlink = spec_symlink,		/* symlink */
		.vop_readdir = spec_readdir,		/* readdir */
		.vop_readlink = spec_readlink,		/* readlink */
		.vop_abortop = spec_abortop,		/* abortop */
		.vop_inactive = ufs211_inactive,	/* inactive */
		.vop_reclaim = ufs211_reclaim,		/* reclaim */
		.vop_lock = ufs211_lock,			/* lock */
		.vop_unlock = ufs211_unlock,		/* unlock */
		.vop_bmap = spec_bmap,				/* bmap */
		.vop_strategy = spec_strategy,		/* strategy */
		.vop_print = ufs211_print,			/* print */
		.vop_islocked = ufs211_islocked,	/* islocked */
		.vop_pathconf = spec_pathconf,		/* pathconf */
		.vop_advlock = spec_advlock,		/* advlock */
		.vop_blkatoff = spec_blkatoff,		/* blkatoff */
		.vop_valloc = spec_valloc,			/* valloc */
		.vop_vfree = ufs211_vfree,			/* vfree */
		.vop_truncate = spec_truncate,		/* truncate */
		.vop_update = ufs211_update,		/* update */
		.vop_bwrite = ufs211_bwrite,		/* bwrite */
		(struct vnodeops *)NULL = (int(*)())NULL
};

#ifdef FIFO
struct vnodeops ufs211_fifoops = {
		.vop_lookup = fifo_lookup,			/* lookup */
		.vop_create = fifo_create,			/* create */
		.vop_mknod = fifo_mknod,			/* mknod */
		.vop_open = fifo_open,				/* open */
		.vop_close = ufs211_close,			/* close */
		.vop_access = ufs211_access,		/* access */
		.vop_getattr = ufs211_getattr,		/* getattr */
		.vop_setattr = ufs_setattr,			/* setattr */
		.vop_read = ufs211_read,			/* read */
		.vop_write = ufs211_write,			/* write */
		.vop_lease = fifo_lease_check,		/* lease */
		.vop_ioctl = fifo_ioctl,			/* ioctl */
		.vop_select = fifo_select,			/* select */
		.vop_revoke = fifo_revoke,			/* revoke */
		.vop_mmap = fifo_mmap,				/* mmap */
		.vop_fsync = fifo_fsync,			/* fsync */
		.vop_seek = fifo_seek,				/* seek */
		.vop_remove = fifo_remove,			/* remove */
		.vop_link = fifo_link,				/* link */
		.vop_rename = fifo_rename,			/* rename */
		.vop_mkdir = fifo_mkdir,			/* mkdir */
		.vop_rmdir = fifo_rmdir,			/* rmdir */
		.vop_symlink = fifo_symlink,		/* symlink */
		.vop_readdir = fifo_readdir,		/* readdir */
		.vop_readlink = fifo_readlink,		/* readlink */
		.vop_abortop = fifo_abortop,		/* abortop */
		.vop_inactive = ufs211_inactive,	/* inactive */
		.vop_reclaim = ufs211_reclaim,		/* reclaim */
		.vop_lock = ufs211_lock,			/* lock */
		.vop_unlock = ufs211_unlock,		/* unlock */
		.vop_bmap = fifo_bmap,				/* bmap */
		.vop_strategy = fifo_strategy,		/* strategy */
		.vop_print = ufs211_print,			/* print */
		.vop_islocked = ufs211_islocked,	/* islocked */
		.vop_pathconf = fifo_pathconf,		/* pathconf */
		.vop_advlock = fifo_advlock,		/* advlock */
		.vop_blkatoff = fifo_blkatoff,		/* blkatoff */
		.vop_valloc = fifo_valloc,			/* valloc */
		.vop_vfree = ufs211_vfree,			/* vfree */
		.vop_truncate = fifo_truncate,		/* truncate */
		.vop_update = ufs211_update,		/* update */
		.vop_bwrite = ufs211_bwrite,		/* bwrite */
		(struct vnodeops *)NULL = (int(*)())NULL
};
#endif /* FIFO */

int
ufs211_create(ap)
	struct vop_create_args ap;
{
	if(u->u_error == ufs211_makeinode(MAKEIMODE(ap->a_vap->va_type, ap->a_vap->va_mode), ap->a_dvp, ap->a_vpp, ap->a_cnp))
		return (u->u_error);
	return (0);
}

int
ufs211_open(ap)
	struct vop_open_args *ap;
{
	if ((UFS211_VTOI(ap->a_vp)->di_flags & APPEND) && (ap->a_mode & (FWRITE | O_APPEND)) == FWRITE)
		return (EPERM);
	return (0);
}

int
ufs211_close(ap)
	struct vop_close_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct ufs211_inode *ip = UFS211_VTOI(vp);

	simple_lock(&vp->v_interlock);
	if (vp->v_usecount > 1)
		ITIMES(ip, &time, &time);
	simple_unlock(&vp->v_interlock);
	return (0);
}

/*
 * Check mode permission on inode pointer.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the
 * read-only status of the file
 * system is checked.
 * Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select
 * the owner/group/other fields.
 * The super user is granted all
 * permissions.
 */
int
ufs211_access(ap)
	struct vop_access_args *ap;
{
	struct vnode *vp = ap->a_vp;
	register struct ufs211_inode *ip = UFS211_VTOI(vp);
	int mode;
	register m;
	register gid_t *gp;

	m = mode;
	if (m == VWRITE) {
		if (ip->i_flags & IMMUTABLE) {
			u->u_error = EPERM;
			return(1);
		}
		/*
		 * Disallow write attempts on read-only
		 * file systems; unless the file is a block
		 * or character device resident on the
		 * file system.
		 */
		if (ip->i_fs->fs_ronly != 0) {
			if ((ip->i_mode & UFS211_IFMT) != UFS211_IFCHR &&
			    (ip->i_mode & UFS211_IFMT) != UFS211_IFBLK) {
				u->u_error = EROFS;
				return (1);
			}
		}
		/*
		 * If there's shared text associated with
		 * the inode, try to free it up once.  If
		 * we fail, we can't allow writing.
		 */
		/*
		if (ip->i_flag& ITEXT)
			xuntext(ip->i_text);
		if (ip->i_flag & ITEXT) {
			u->u_error = ETXTBSY;
			return (1);
		}
		*/
	}
	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (u->u_uid == 0)
		return (0);
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (u->u_uid != ip->i_uid) {
		m >>= 3;
		gp = u->u_groups;
		for (; gp < &u->u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (ip->i_gid == *gp)
				goto found;
		m >>= 3;
found:
		;
	}
	if ((ip->i_mode&m) != 0)
		return (0);
	u->u_error = EACCES;
	return (1);
}

int
ufs211_getattr(ap)
	struct vop_getattr_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct ufs211_inode *ip = UFS211_VTOI(vp);
	register struct vattr *vap = ap->a_vap;

	ITIMES(ip, &time, &time);
	/*
	 * Copy from inode table
	 */
	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;
	vap->va_mode = ip->i_mode & ~UFS211_IFMT;
	vap->va_nlink = ip->i_nlink;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_rdev = (dev_t)ip->i_rdev;
	vap->va_size = ip->i_din.di_size;
	vap->va_atime.tv_sec = ip->i_atime;
	vap->va_atime.tv_nsec = ip->i_atimensec;
	vap->va_mtime.tv_sec = ip->i_mtime;
	vap->va_mtime.tv_nsec = ip->i_mtimensec;
	vap->va_ctime.tv_sec = ip->i_ctime;
	vap->va_ctime.tv_nsec = ip->i_ctimensec;
	vap->va_flags = ip->di_flags;
	vap->va_gen = ip->di_gen;
	/* this doesn't belong here */
	if (vp->v_type == VBLK)
		vap->va_blocksize = BLKDEV_IOSIZE;
	else if (vp->v_type == VCHR)
		vap->va_blocksize = MAXBSIZE;
	else
		vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;
	vap->va_bytes = dbtob((u_quad_t)ip->di_blocks);
	vap->va_type = vp->v_type;
	//vap->va_filerev = ip->i_modrev; /* nfs */
	return (0);
}

/*
 * Set the attributes on a file.  This was placed here because ufs_syscalls
 * is too large already (it will probably be split into two files eventually).
*/
int
ufs211_setattr(ap)
	struct vop_setattr_args *ap;
{
	struct vattr *vap = ap->a_vap;
	struct vnode *vp = ap->a_vp;
	struct ufs211_inode *ip = UFS211_VTOI(vp);
	struct timeval atimeval, mtimeval;
	int error;

	if (ip->i_fs->fs_ronly) /* can't change anything on a RO fs */
		return (EROFS);
	if (vap->va_flags != VNOVAL) {
		if (u->u_uid != ip->i_uid && !ufs211_suser())
			return (u->u_error);
		if (u->u_uid == 0) {
			if ((ip->i_flags & (SF_IMMUTABLE | SF_APPEND)) && securelevel > 0)
				return (EPERM);
			ip->i_flags = vap->va_flags;
		} else {
			if (ip->i_flags & (SF_IMMUTABLE | SF_APPEND))
				return (EPERM);
			ip->i_flags &= SF_SETTABLE;
			ip->i_flags |= (vap->va_flags & UF_SETTABLE);
		}
		ip->i_flag |= UFS211_ICHG;
		if (vap->va_flags & (IMMUTABLE | APPEND))
			return (0);
	}
	if (ip->i_flags & (IMMUTABLE | APPEND))
		return (EPERM);
	/*
	 * Go thru the fields (other than 'flags') and update iff not VNOVAL.
	 */
	if (vap->va_uid != (uid_t) VNOVAL || vap->va_gid != (gid_t) VNOVAL)
		if (error == ufs211_chown1(ip, vap->va_uid, vap->va_gid))
			return (error);
	if (vap->va_size != (off_t) VNOVAL) {
		if ((ip->i_mode & UFS211_IFMT) == UFS211_IFDIR)
			return (EISDIR);
		ufs211_trunc(ip, vap->va_size, 0);
		if (u->u_error)
			return (u->u_error);
	}
	if (vap->va_atime != (time_t) VNOVAL || vap->va_mtime != (time_t) VNOVAL) {
		if (u->u_uid != ip->i_uid && !suser()
				&& ((vap->va_vaflags & VA_UTIMES_NULL) == 0
						|| access(ip, UFS211_IWRITE)))
			return (u->u_error);
		if (vap->va_atime != (time_t) VNOVAL
				&& !(ip->i_fs->fs_flags & MNT_NOATIME))
			ip->i_flag |= UFS211_IACC;
		if (vap->va_mtime != (time_t) VNOVAL)
			ip->i_flag |= (UFS211_IUPD | UFS211_ICHG);
		atimeval.tv_sec = vap->va_atime;
		mtimeval.tv_sec = vap->va_mtime;
		ufs211_updat(ip, &atimeval, &mtimeval, 1);
	}
	if (vap->va_mode != (mode_t) VNOVAL)
		return (ufs211_chmod1(ip, vap->va_mode));
	return(0);
}

/* copied, for supervisory networking, to sys_net.c */
/*
 * Test if the current user is the
 * super user.
 */
int
ufs211_suser()
{
	if (u->u_uid == 0) {
		u->u_acflag |= ASU;
		crset(u->u_ucred);
		return (1);
	}
	u->u_error = EPERM;
	crset(u->u_ucred);
	return (0);
}

int
ufs211_read(ap)
	struct vop_read_args *ap;
{
	return (0);
}

int
ufs211_write(ap)
	struct vop_write_args *ap;
{
	return (0);
}

int
ufs211_lease_check(ap)
	struct vop_lease_check_args ap;
{
	return (0);
}

int
ufs211_ioctl(ap)
	struct vop_ioctl_args ap;
{
	return (0);
}

int
ufs211_select(ap)
	struct vop_select_args ap;
{
	return (0);
}

int
ufs211_revoke(ap)
	struct vop_revoke_args ap;
{
	return (0);
}

int
ufs211_mmap(ap)
	struct vop_mmap_args ap;
{
	return (0);
}

int
ufs211_fsync(ap)
	struct vop_fsync_args *ap;
{
	struct vnode *vp = ap->a_vp;
	//struct filedesc *fdp = ap->a_p->p_fd;
	//struct file *fpp = fdp->fd_ofiles;

	syncip(UFS211_VTOI(vp));

	return (VOP_UPDATE(ap->a_vp, &tv, &tv, ap->a_waitfor == MNT_WAIT));
}

int
ufs211_seek(ap)
	struct vop_seek_args *ap;
{
	return (0);
}

int
ufs211_remove(ap)
	struct vop_remove_args *ap;
{
	return (0);
}

int
ufs211_rename(ap)
	struct vop_rename_args *ap;
{
	return (0);
}

int
ufs211_readdir(ap)
	struct vop_readdir_args *ap;
{
	return (0);
}

int
ufs211_abortop(ap)
	struct vop_abortop_args ap;
{
	return (0);
}

int
ufs211_inactive(ap)
	struct vop_inactive_args *ap;
{
	return (0);
}

int
ufs211_reclaim(ap)
	struct vop_reclaim_args *ap;
{
	return (0);
}

/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
int
ufs211_lock(ap)
	struct vop_lock_args *ap;
{
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&UFS211_VTOI(vp)->i_lock, ap->a_flags, &vp->v_interlock, ap->a_p));
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
int
ufs211_unlock(ap)
	struct vop_unlock_args *ap;
{
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&UFS211_VTOI(vp)->i_lock, ap->a_flags | LK_RELEASE, &vp->v_interlock, ap->a_p));
}

int
ufs211_bmap(ap)
	struct vop_bmap_args *ap;
{
	struct ufs211_inode *ip = UFS211_VTOI(ap->a_vp);

	if(ap->a_vpp != NULL) {
		!ap->a_vpp = ip->i_devvp;
	}
	if (ap->a_bnp == NULL || ap->a_bnp == ufs211_bmap1(ip, ap->a_bn, UFS211_IREAD, ip->i_flag)) {
		return (0);
	}
	return (0);
}

int
ufs211_strategy(ap)
	struct vop_strategy_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	struct ufs211_inode *ip = UFS211_VTOI(vp);

	return (0);
}

int
ufs211_print(ap)
	struct vop_print_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	struct ufs211_inode *ip = UFS211_VTOI(vp);

	printf("tag VT_UFS211, ino %d, on dev %d, %d", ip->i_number, major(ip->i_dev), minor(ip->i_dev));
#ifdef FIFO
	if (vp->v_type == VFIFO)
		fifo_printinfo(vp);
#endif /* FIFO */
	lockmgr_printinfo(&ip->i_lock);
	printf("\n");
	return (0);
}

int
ufs211_islocked(ap)
	struct vop_islocked_args *ap;
{
	return (0);
}

int
ufs211_advlock(ap)
	struct vop_advlock_args *ap;
{
	struct ufs211_inode *ip = UFS211_VTOI(ap->a_vp);

	return lf_advlock(ap, &ip->i_lockf, ip->i_size);
}

int
ufs211_pathconf(ap)
	struct vop_pathconf_args *ap;
{
	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = LINK_MAX;
		return (0);
	case _PC_NAME_MAX:
		*ap->a_retval = NAME_MAX;
		return (0);
	case _PC_PATH_MAX:
		*ap->a_retval = PATH_MAX;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_NO_TRUNC:
		*ap->a_retval = 1;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

int
ufs211_link(ap)
	struct vop_link_args *ap;
{
	struct ufs211_inode *ip = UFS211_VTOI(ap->a_vp);
	if(ip == NULL) {
		return (1);
	}

	vrele(ap->a_vp);
	return (0);
}

int
ufs211_symlink(ap)
	struct vop_symlink_args *ap;
{
	return (0);
}

int
ufs211_readlink(ap)
	struct vop_readlink_args *ap;
{
	return (0);
}


int
ufs211_mkdir(ap)
	struct vop_mkdir_args *ap;
{
	return (0);
}

int
ufs211_rmdir(ap)
	struct vop_rmdir_args *ap;
{
	return (0);
}

int
ufs211_mknod(ap)
	struct vop_mknod_args *ap;
{
	struct vattr *vap = ap->a_vap;
	struct vnode **vpp = ap->a_vpp;
	register struct ufs211_inode *ip;

	if (u->u_error == ufs211_makeinode(MAKEIMODE(vap->va_type, vap->va_mode), ap->a_dvp, vpp, ap->a_cnp))
		return (u->u_error);
	ip = UFS211_VTOI(*vpp);
	ip->i_flag |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	if (vap->va_rdev != VNOVAL) {
		/*
		 * Want to be able to use this to make badblock
		 * inodes, so don't truncate the dev number.
		 */
		ip->i_rdev = vap->va_rdev;
	}
	/*
	 * Remove inode so that it will be reloaded by VFS_VGET and
	 * checked to see if it is an alias of an existing entry in
	 * the inode cache.
	 */
	vput(*vpp);
	(*vpp)->v_type = VNON;
	vgone(*vpp);
	*vpp = 0;
	return (0);
}

int
ufs211_blkatoff(ap)
	struct vop_blkatoff_args *ap;
{
	return (0);
}

int
ufs211_valloc(ap)
	struct vop_valloc_args *ap;
{
	return (0);
}

int
ufs211_vfree(ap)
	struct vop_vfree_args *ap;
{
	return (0);
}

/*
 * Truncate a file given a file descriptor.
 */
int
ufs211_truncate(ap)
	struct vop_truncate_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct ufs211_inode *ip =  UFS211_VTOI(vp);

	return (0);
}

int
ufs211_update(ap)
	struct vop_update_args *ap;
{
	return (0);
}

int
ufs211_bwrite(ap)
	struct vop_bwrite_args *ap;
{
	return (0);
}

#ifdef FIFO
int
ufs211fifo_read(ap)
	struct vop_read_args *ap;
{
	extern struct fifo_vnodeops;

	/*
	 * Set access flag.
	 */
	UFS211_VTOI(ap->a_vp)->i_flag |= IN_ACCESS;

	return (VOPARGS(ap, vop_read));
}

int
ufs211fifo_write(ap)
	struct vop_write_args *ap;
{
	extern struct fifo_vnodeops;

	/*
	 * Set update and change flags.
	 */
	UFS211_VTOI(ap->a_vp)->i_flag |= IN_CHANGE | IN_UPDATE;

	return (VOPARGS(ap, vop_write));
}

int
ufs211fifo_close(ap)
	struct vop_close_args *ap;
{
	extern struct fifo_vnodeops;
	struct vnode *vp = ap->a_vp;
	struct ufs211_inode *ip = UFS211_VTOI(vp);

	simple_lock(&vp->v_interlock);
	if (ap->a_vp->v_usecount > 1)
		ITIMES(ip, &time, &time);
	simple_unlock(&vp->v_interlock);
	return (VOPARGS(ap, vop_close));
}
#endif /* FIFO */

/*
 * Initialize the vnode associated with a new inode, handle aliased
 * vnodes.
 */


int
ufs211_vinit(mntp, specops, fifoops, vpp)
	struct mount *mntp;
	struct vnodeops *specops;
	struct vnodeops *fifoops;
	struct vnode **vpp;
{
	struct proc *p = curproc;	/* XXX */
	struct ufs211_inode *ip;
	struct vnode *vp, *nvp;

	vp = *vpp;
	ip = UFS211_VTOI(vp);

	switch (vp->v_type = IFTOVT(ip->i_mode)) {
	case VCHR:
	case VBLK:
		vp->v_op = specops;
		if (nvp == checkalias(vp, ip->i_rdev, mntp)) {
			/*
			 * Discard unneeded vnode, but save its inode.
			 * Note that the lock is carried over in the inode
			 * to the replacement vnode.
			 */
			nvp->v_data = vp->v_data;
			vp->v_data = NULL;
			vp->v_op = &spec_vnodeops;
			vrele(vp);
			vgone(vp);
			/*
			 * Reinitialize aliased inode.
			 */
			vp = nvp;
			ip->i_vnode = vp;
		}
		break;
	case VFIFO:
#ifdef FIFO
		vp->v_op = fifoops;
		break;
#else
		return (EOPNOTSUPP);
#endif
	}
	if (ip->i_number == UFS211_ROOTINO)
		vp->v_flag |= VROOT;
	*vpp = vp;
	return (0);
}

/*
 * Allocate a new inode.
 */
int
ufs211_makeinode(mode, dvp, vpp, cnp)
	int mode;
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
{
	register struct ufs211_inode *ip, *pdir;
	struct timeval tv;
	struct vnode *tvp;
	int error;

	pdir = UFS211_VTOI(dvp);

#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & HASBUF) == 0)
		panic("ufs211_makeinode: no name");
#endif
	*vpp = NULL;
	if ((mode & UFS211_IFMT) == 0)
		mode |= UFS211_IFREG;

	if (error == VOP_VALLOC(dvp, mode, cnp->cn_cred, &tvp)) {
		free(cnp->cn_pnbuf, M_NAMEI);
		vput(dvp);
		return (error);
	}
	ip = UFS211_VTOI(tvp);
	ip->i_gid = pdir->i_gid;
	if ((mode & UFS211_IFMT) == UFS211_IFLNK)
		ip->i_uid = pdir->i_uid;
	else
		ip->i_uid = cnp->cn_cred->cr_uid;
#ifdef QUOTA
	if ((error = getinoquota(ip)) ||
	    (error = chkiq(ip, 1, cnp->cn_cred, 0))) {
		free(cnp->cn_pnbuf, M_NAMEI);
		VOP_VFREE(tvp, ip->i_number, mode);
		vput(tvp);
		vput(dvp);
		return (error);
	}
#endif
	ip->i_flag |= IN_ACCESS | IN_CHANGE | IN_UPDATE;
	ip->i_mode = mode;
	tvp->v_type = IFTOVT(mode);	/* Rest init'd in getnewvnode(). */
	ip->i_nlink = 1;
	if ((ip->i_mode & UFS211_ISGID) && !groupmember(ip->i_gid, cnp->cn_cred) && suser())
		ip->i_mode &= ~UFS211_ISGID;

	if (cnp->cn_flags & ISWHITEOUT)
		ip->i_flags |= UF_OPAQUE;

	/*
	 * Make sure inode goes to disk before directory entry.
	 */
	tv = time;
	if (error == VOP_UPDATE(tvp, &tv, &tv, 1))
		goto bad;
	if (error == ufs_direnter(ip, dvp, cnp))
		goto bad;
	if ((cnp->cn_flags & SAVESTART) == 0)
		FREE(cnp->cn_pnbuf, M_NAMEI);
	vput(dvp);
	*vpp = tvp;
	return (0);

bad:
	/*
	 * Write error occurred trying to update the inode
	 * or the directory so must deallocate the inode.
	 */
	free(cnp->cn_pnbuf, M_NAMEI);
	vput(dvp);
	ip->i_nlink = 0;
	ip->i_flag |= IN_CHANGE;
	vput(tvp);
	return (error);
}
