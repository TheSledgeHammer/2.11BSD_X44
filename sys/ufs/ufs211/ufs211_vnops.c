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
#include <sys/lock.h>
#include <sys/lockf.h>

#include <ufs/ufs211/ufs211_quota.h>
#include <ufs/ufs211/ufs211_fs.h>
#include <ufs/ufs211/ufs211_inode.h>
#include <ufs/ufs211/ufs211_mount.h>
#include <ufs/ufs211/ufs211_extern.h>

#include <vm/include/vm.h>
#include <miscfs/specfs/specdev.h>

/* Global vfs data structures for ufs211. */
struct vnodeops ufs211_vnodeops = {
		.vop_default = vop_default_error,	/* default */
		.vop_lookup = ufs211_lookup,		/* lookup */
		.vop_create = ufs211_create,		/* create */
		.vop_whiteout = ufs211_whiteout,	/* whiteout */
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
		.vop_poll = ufs211_poll,			/* poll */
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
		.vop_reallocblks = ufs211_reallocblks,/* reallocblks */
		.vop_vfree = ufs211_vfree,			/* vfree */
		.vop_truncate = ufs211_truncate,	/* truncate */
		.vop_update = ufs211_update,		/* update */
		.vop_bwrite = vn_bwrite,			/* bwrite */
};

struct vnodeops ufs211_specops = {
		.vop_default = vop_default_error,	/* default */
		.vop_lookup = spec_lookup,			/* lookup */
		.vop_create = spec_create,			/* create */
		.vop_mknod = spec_mknod,			/* mknod */
		.vop_open = spec_open,				/* open */
		.vop_close = ufs211spec_close,		/* close */
		.vop_access = ufs211_access,		/* access */
		.vop_getattr = ufs211_getattr,		/* getattr */
		.vop_setattr = ufs211_setattr,		/* setattr */
		.vop_read = ufs211spec_read,		/* read */
		.vop_write = ufs211spec_write,		/* write */
		.vop_lease = spec_lease_check,		/* lease */
		.vop_ioctl = spec_ioctl,			/* ioctl */
		.vop_select = spec_select,			/* select */
		.vop_poll = spec_poll,				/* poll */
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
		.vop_reallocblks = spec_reallocblks,/* reallocblks */
		.vop_vfree = ufs211_vfree,			/* vfree */
		.vop_truncate = spec_truncate,		/* truncate */
		.vop_update = ufs211_update,		/* update */
		.vop_bwrite = vn_bwrite,			/* bwrite */
};

#ifdef FIFO
struct vnodeops ufs211_fifoops = {
		.vop_default = vop_default_error,	/* default */
		.vop_lookup = fifo_lookup,			/* lookup */
		.vop_create = fifo_create,			/* create */
		.vop_mknod = fifo_mknod,			/* mknod */
		.vop_open = fifo_open,				/* open */
		.vop_close = ufs211fifo_close,		/* close */
		.vop_access = ufs211_access,		/* access */
		.vop_getattr = ufs211_getattr,		/* getattr */
		.vop_setattr = ufs211_setattr,		/* setattr */
		.vop_read = ufs211fifo_read,		/* read */
		.vop_write = ufs211fifo_write,		/* write */
		.vop_lease = fifo_lease_check,		/* lease */
		.vop_ioctl = fifo_ioctl,			/* ioctl */
		.vop_select = fifo_select,			/* select */
		.vop_poll = fifo_poll,				/* poll */
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
		.vop_reallocblks = fifo_reallocblks,/* reallocblks */
		.vop_vfree = ufs211_vfree,			/* vfree */
		.vop_truncate = fifo_truncate,		/* truncate */
		.vop_update = ufs211_update,		/* update */
		.vop_bwrite = vn_bwrite,			/* bwrite */
};
#endif /* FIFO */

void
ufs211_itimes(vp)
	struct vnode *vp;
{
	struct ufs211_inode *ip;
	struct timespec ts;

	ip = UFS211_VTOI(vp);
	if ((ip->i_flag & (UFS211_IACC | UFS211_ICHG | UFS211_IUPD)) == 0) {
		return;
	}
	if ((vp->v_type == VBLK || vp->v_type == VCHR) /* && !DOINGSOFTDEP(vp)*/) {
		ip->i_flag |= UFS211_INLAZYMOD;
	} else {
		ip->i_flag |= UFS211_IMOD;
	}
	if ((vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
		vfs_timestamp(&ts);
		if (ip->i_flag & UFS211_IACC) {
			ip->i_atime = ts.tv_sec;
			ip->i_atimensec = ts.tv_nsec;
		}
		if (ip->i_flag & UFS211_IUPD) {
			ip->i_mtime = ts.tv_sec;
			ip->i_mtimensec = ts.tv_nsec;
			ip->i_modrev++;
		}
		if (ip->i_flag & UFS211_ICHG) {
			ip->i_ctime = ts.tv_sec;
			ip->i_ctimensec = ts.tv_nsec;
		}
	}
	ip->i_flag &= ~(UFS211_IACC | UFS211_ICHG | UFS211_IUPD);
}

int
ufs211_create(ap)
	struct vop_create_args *ap;
{
	if(u.u_error == ufs211_makeinode(MAKEIMODE(ap->a_vap->va_type, ap->a_vap->va_mode), ap->a_dvp, ap->a_vpp, ap->a_cnp))
		return (u.u_error);
	return (0);
}

int
ufs211_open(ap)
	struct vop_open_args *ap;
{
    
	if ((UFS211_VTOI(ap->a_vp)->i_din->di_flag & APPEND) && (ap->a_mode & (FWRITE | O_APPEND)) == FWRITE)
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
		ufs211_itimes(vp);
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
	mode_t mask, mode = ap->a_mode;
	register gid_t *gp;

    mask = mode;
	if (mode & VWRITE) {
		switch (vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY) {
				return (EROFS);
			}
			break;
		}
		if ((mode & VWRITE) && (ip->i_flags & IMMUTABLE)) {
			u.u_error = EPERM;
			return (EPERM);
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
				u.u_error = EROFS;
				return (EROFS);
			}
		}
		/*
		 * If there's shared text associated with
		 * the inode, try to free it up once.  If
		 * we fail, we can't allow writing.
		 */
		/*
		if (ip->i_flag& ITEXT)
			vm_xuntext(ip->i_text);
		if (ip->i_flag & ITEXT) {
			u.u_error = ETXTBSY;
			return (1);
		}
		*/
	}
	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (u.u_uid == 0)
		return (0);

    mask = 0;

	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (u.u_uid != ip->i_uid) {
		if (mode & VEXEC)
			mask |= S_IXUSR;
		if (mode & VREAD)
			mask |= S_IRUSR;
		if (mode & VWRITE)
			mask |= S_IWUSR;
        
		mask >>= 3;
		gp = u.u_ucred->cr_groups;
		for (; gp < &u.u_ucred->cr_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (ip->i_gid == *gp)
			if (mode & VEXEC)
				mask |= S_IXGRP;
			if (mode & VREAD)
				mask |= S_IRGRP;
			if (mode & VWRITE)
				mask |= S_IWGRP;
			goto found;
		mask >>= 3;
found:
		;
	}
	if ((ip->i_mode & mask) != 0)
		return (0);
	u.u_error = EACCES;
	return (1);
}

int
ufs211_getattr(ap)
	struct vop_getattr_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct ufs211_inode *ip = UFS211_VTOI(vp);
	register struct vattr *vap = ap->a_vap;

	ufs211_itimes(vp);
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
	vap->va_size = ip->i_din->di_size;
	vap->va_atime.tv_sec = ip->i_atime;
	vap->va_atime.tv_nsec = ip->i_atimensec;
	vap->va_mtime.tv_sec = ip->i_mtime;
	vap->va_mtime.tv_nsec = ip->i_mtimensec;
	vap->va_ctime.tv_sec = ip->i_ctime;
	vap->va_ctime.tv_nsec = ip->i_ctimensec;
	vap->va_flags = ip->i_din->di_flag;
	vap->va_gen = ip->i_din->di_gen;
	/* this doesn't belong here */
	if (vp->v_type == VBLK)
		vap->va_blocksize = BLKDEV_IOSIZE;
	else if (vp->v_type == VCHR)
		vap->va_blocksize = MAXBSIZE;
	else
		vap->va_blocksize = vp->v_mount->mnt_stat.f_iosize;
	vap->va_bytes = dbtob((u_quad_t)ip->i_din->di_blocks);
	vap->va_type = vp->v_type;
	vap->va_filerev = ip->i_modrev; /* nfs */
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
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			return (EROFS);
		}
		if (u.u_uid != ip->i_uid && !suser())
			return (u.u_error);
		if (u.u_uid == 0) {
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
	if (vap->va_uid != (uid_t)VNOVAL || vap->va_gid != (gid_t)VNOVAL) {
		if (vp->v_mount->mnt_flag & MNT_RDONLY) {
			return (EROFS);
		}
		if ((error = ufs211_chown1(ip, vap->va_uid, vap->va_gid))) {
			return (error);
		}
	}
	if (vap->va_size != (off_t)VNOVAL) {
		switch (vp->v_type) {
		case VDIR:
			if ((ip->i_mode & UFS211_IFMT) == UFS211_IFDIR) {
				return (EISDIR);
			}
			return (EISDIR);
		case VLNK:
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY) {
				return (EROFS);
			}
			break;
		}
		ufs211_trunc(ip, vap->va_size, 0);
		if (u.u_error) {
			return (u.u_error);
		}
		if ((error = VOP_TRUNCATE(vp, vap->va_size, 0, u.u_ucred, u.u_procp))) {
			return (error);
		}
	}
	ufs211_itimes(vp);
	if (vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL) {
        if (vp->v_mount->mnt_flag & MNT_RDONLY) {
            return (EROFS);
        }
		if (u.u_uid != ip->i_uid && !suser() && ((vap->va_vaflags & VA_UTIMES_NULL) == 0 || VOP_ACCESS(vp, VWRITE, u.u_ucred, u.u_procp)))
			return (u.u_error);
		if (vap->va_atime.tv_sec != (time_t) VNOVAL && !(ip->i_fs->fs_flags & MNT_NOATIME))
			ip->i_flag |= UFS211_IACC;
		if (vap->va_mtime.tv_sec != VNOVAL)
			ip->i_flag |= (UFS211_IUPD | UFS211_ICHG);
		atimeval.tv_sec = vap->va_atime.tv_sec;
		mtimeval.tv_sec = vap->va_mtime.tv_sec;
		ufs211_updat(ip, &atimeval, &mtimeval, 1);
	}
	if (vap->va_mode != (mode_t) VNOVAL)
		return (ufs211_chmod1(vp, vap->va_mode));
	return(0);
}
/*
 * Change mode of a file given path name.
 */
#ifdef notyet
void
ufs211_chmod()
{
	register struct ufs211_inode *ip;
	register struct a {
		char	*fname;
		int		fmode;
	} *uap = (struct a *)u.u_ap;

	struct	vattr				vattr;
	struct	nameidata 			nd;
	register struct nameidata *ndp = &nd;

	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, uap->fname);
	ip = namei(ndp);
	if (!ip)
		return;
	VATTR_NULL(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = ufs_setattr(ip, &vattr);
	vput(ip);
}
#endif /* notyet */

int
ufs211_chmod1(vp, mode)
	register struct vnode *vp;
	register int mode;
{
	register struct ufs211_inode *ip;

	ip = UFS211_VTOI(vp);
	if (u.u_uid != ip->i_uid && !suser())
		return (u.u_error);
	if (u.u_uid) {
		if ((ip->i_mode & UFS211_IFMT) != UFS211_IFDIR && (mode & UFS211_ISVTX)) {
			return (EFTYPE);
		}
		if (!groupmember(ip->i_gid) && (mode & UFS211_ISGID)) {
			return (EPERM);
		}
	}
	ip->i_mode &= ~07777; /* why? */
	ip->i_mode |= mode & 07777;
	ip->i_flag |= UFS211_ICHG;

	if ((vp->v_flag & VTEXT) && (ip->i_mode & S_ISTXT) == 0) {
		(void) vnode_pager_uncache(vp);
	}

	return (0);
}

/*
 * Set ownership given a path name.
 */
#ifdef notyet
void
ufs211_chown(ap)
	struct vop_setattr_args *ap;
{
	struct vnode *vp;
	register struct ufs211_inode *ip;
	struct	nameidata nd;
	register struct	nameidata *ndp;

	ndp = &nd;
	NDINIT(ndp, LOOKUP, NOFOLLOW, UIO_USERSPACE, uap->fname);
	ip = namei(ndp);
	if (ip == NULL) {
		return;
	}
	u.u_error = ufs211_setattr(ap);
	vput(ip);
}
#endif /* notyet */

/*
 * Perform chown operation on inode ip.  This routine called from ufs_setattr.
 * inode must be locked prior to call.
 */
int
ufs211_chown1(ip, uid, gid)
	register struct ufs211_inode *ip;
	register uid_t uid;
	register gid_t gid;
{
	register struct vnode *vp;
	uid_t ouid;
	gid_t ogid;
	int error;
#ifdef QUOTA
	register int i;
	long change;
#endif

	vp = UFS211_ITOV(ip);

	if (uid == -1)
		uid = ip->i_uid;
	if (gid == -1)
		gid = ip->i_gid;
	/*
	 * If we don't own the file, are trying to change the owner
	 * of the file, or are not a member of the target group,
	 * the caller must be superuser or the call fails.
	 */
	if ((u.u_uid != ip->i_uid || uid != ip->i_uid || !groupmember(gid)) && !suser()) {
		return (u.u_error);
	}
	ouid = ip->i_uid;
	ogid = ip->i_gid;
#ifdef QUOTA
	if (error = ufs211_getinoquota(ip)) {
		return (error);
	}
	if (ouid == uid) {
		ufs211_dqrele(vp, ip->i_dquot[USRQUOTA]);
		ip->i_dquot[USRQUOTA] = NODQUOT;
		change = 0;
	} else {
		change = ip->i_size;
	}
	if (ogid == gid) {
		ufs211_dqrele(vp, ip->i_dquot[GRPQUOTA]);
		ip->i_dquot[GRPQUOTA] = NODQUOT;
	} else {
		//change = ip->i_din->di_blocks;
	}
	(void)ufs211_chkdq(ip, -change, u.u_ucred, CHOWN);
	(void)ufs211_chkiq(ip, -1, u.u_ucred, CHOWN);
	for (i = 0; i < MAXQUOTAS; i++) {
		ufs211_dqrele(vp, ip->i_dquot[i]);
		ip->i_dquot[i] = NODQUOT;
	}
#endif
	ip->i_uid = uid;
	ip->i_gid = gid;
#ifdef QUOTA
	if ((error = ufs211_getinoquota(ip)) == 0) {
		if (ouid == uid) {
			ufs211_dqrele(vp, ip->i_dquot[USRQUOTA]);
			ip->i_dquot[USRQUOTA] = NODQUOT;
		}
		if (ogid == gid) {
			ufs211_dqrele(vp, ip->i_dquot[GRPQUOTA]);
			ip->i_dquot[GRPQUOTA] = NODQUOT;
		}
		if ((error = ufs211_chkdq(ip, change, u.u_ucred, CHOWN)) == 0) {
			if ((error = ufs211_chkiq(ip, 1, u.u_ucred, CHOWN)) == 0) {
				goto good;
			} else {
				(void) ufs211_chkdq(ip, -change, u.u_ucred, CHOWN | FORCE);
			}
		}
		for (i = 0; i < MAXQUOTAS; i++) {
			ufs211_dqrele(vp, ip->i_dquot[i]);
			ip->i_dquot[i] = NODQUOT;
		}
	}
	ip->i_gid = ogid;
	ip->i_uid = ouid;
	if (ufs211_getinoquota(ip) == 0) {
		if (ouid == uid) {
			ufs211_dqrele(vp, ip->i_dquot[USRQUOTA]);
			ip->i_dquot[USRQUOTA] = NODQUOT;
		}
		if (ogid == gid) {
			ufs211_dqrele(vp, ip->i_dquot[GRPQUOTA]);
			ip->i_dquot[GRPQUOTA] = NODQUOT;
		}
		(void) ufs211_chkdq(ip, change, u.u_ucred, FORCE|CHOWN);
		(void) ufs211_chkiq(ip, 1, u.u_ucred, FORCE|CHOWN);
		(void) ufs211_getinoquota(ip);
	}
	return (error);
good:
	if (ufs211_getinoquota(ip)) {
		panic("chown: lost quota");
	}
#endif
	if (ouid != uid || ogid != gid)
		ip->i_flag |= UFS211_ICHG;
	if (ouid != uid && u.u_uid != 0)
		ip->i_mode &= ~UFS211_ISUID;
	if (ogid != gid && u.u_uid != 0)
		ip->i_mode &= ~UFS211_ISGID;
	return (0);
}

int
ufs211_read(ap)
	struct vop_read_args *ap;
{
	/*
	 * Set access flag.
	 */
	int error, resid;
	struct ufs211_inode *ip;
	struct uio *uio;

	uio = ap->a_uio;
	resid = uio->uio_resid;
	ip = UFS211_VTOI(ap->a_vp);

	error = VOCALL(ap->a_vp->v_op, &ap->a_head);
	if (ip != NULL && (uio->uio_resid != resid || (error == 0 && resid != 0))) {
		ip->i_flag |= UFS211_IACC;
	}
	return (error);
}

int
ufs211_write(ap)
	struct vop_write_args *ap;
{
	/*
	 * Set update and change flags.
	 */
	int error, resid;
	struct ufs211_inode *ip;
	struct uio *uio;

	uio = ap->a_uio;
	resid = uio->uio_resid;
	ip = UFS211_VTOI(ap->a_vp);

	error = VOCALL(ap->a_vp->v_op, &ap->a_head);
	if (ip != NULL && (uio->uio_resid != resid || (error == 0 && resid != 0))) {
		ip->i_flag |= UFS211_ICHG | UFS211_IUPD;
	}
	return (error);
}

int
ufs211_ioctl(ap)
	struct vop_ioctl_args *ap;
{
	return (ENOTTY);
}

int
ufs211_select(ap)
	struct vop_select_args *ap;
{
	return (1);
}

int
ufs211_poll(ap)
	struct vop_poll_args *ap;
{
	return (1);
}

int
ufs211_mmap(ap)
	struct vop_mmap_args *ap;
{
	return (EINVAL);
}

int
ufs211_fsync(ap)
	struct vop_fsync_args *ap;
{
    struct timeval tv;

	ufs211_syncip(ap->a_vp);
	tv = time;
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
	struct ufs211_inode *ip;
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
	int error;

	ip = UFS211_VTOI(vp);
	if ((ip->i_flags & (IMMUTABLE | APPEND)) || (UFS211_VTOI(dvp)->i_flags & APPEND)) {
		error = EPERM;
		goto out;
	}
	if ((error = ufs211_dirremove(dvp, ap->a_cnp)) == 0) {
		ip->i_nlink--;
		ip->i_flag |= UFS211_ICHG;
	}
out:
	if (dvp == vp) {
		vrele(vp);
	} else {
		vput(vp);
	}
	vput(dvp);

	return (0);
}

int
ufs211_rename(ap)
	struct vop_rename_args *ap;
{
	struct vnode *tvp = ap->a_tvp;
	register struct vnode *tdvp = ap->a_tdvp;
	struct vnode *fvp = ap->a_fvp;
	struct vnode *fdvp = ap->a_fdvp;
	struct componentname *tcnp = ap->a_tcnp;
	struct componentname *fcnp = ap->a_fcnp;
	struct proc *p = fcnp->cn_proc;
	struct ufs211_inode *ip, *xp, *dp;
	struct ufs211_dirtemplate dirbuf;
	struct timeval tv;
	int doingdirectory = 0, oldparent = 0, newparent = 0;
	int error = 0;
	u_char namlen;

#ifdef DIAGNOSTIC
	if ((tcnp->cn_flags & HASBUF) == 0 ||
	    (fcnp->cn_flags & HASBUF) == 0)
		panic("ufs_rename: no name");
#endif
	/*
	 * Check for cross-device rename.
	 */
	if ((fvp->v_mount != tdvp->v_mount) ||
	    (tvp && (fvp->v_mount != tvp->v_mount))) {
		error = EXDEV;
abortit:
		VOP_ABORTOP(tdvp, tcnp); /* XXX, why not in NFS? */
		if (tdvp == tvp)
			vrele(tdvp);
		else
			vput(tdvp);
		if (tvp)
			vput(tvp);
		VOP_ABORTOP(fdvp, fcnp); /* XXX, why not in NFS? */
		vrele(fdvp);
		vrele(fvp);
		return (error);
	}

	/*
	 * Check if just deleting a link name.
	 */
	if (tvp && ((UFS211_VTOI(tvp)->i_flags & (IMMUTABLE | APPEND)) ||
	    (UFS211_VTOI(tdvp)->i_flags & APPEND))) {
		error = EPERM;
		goto abortit;
	}
	if (fvp == tvp) {
		if (fvp->v_type == VDIR) {
			error = EINVAL;
			goto abortit;
		}

		/* Release destination completely. */
		VOP_ABORTOP(tdvp, tcnp);
		vput(tdvp);
		vput(tvp);

		/* Delete source. */
		vrele(fdvp);
		vrele(fvp);
		fcnp->cn_flags &= ~MODMASK;
		fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
		if ((fcnp->cn_flags & SAVESTART) == 0)
			panic("ufs211_rename: lost from startdir");
		fcnp->cn_nameiop = DELETE;
		(void) relookup(fdvp, &fvp, fcnp);
		return (VOP_REMOVE(fdvp, fvp, fcnp));
	}
	if (error = vn_lock(fvp, LK_EXCLUSIVE, p))
		goto abortit;
	dp = UFS211_VTOI(fdvp);
	ip = UFS211_VTOI(fvp);
	if ((ip->i_flags & (IMMUTABLE | APPEND)) || (dp->i_flags & APPEND)) {
		VOP_UNLOCK(fvp, 0, p);
		error = EPERM;
		goto abortit;
	}
	if ((ip->i_mode & UFS211_IFMT) == UFS211_IFDIR) {
		/*
		 * Avoid ".", "..", and aliases of "." for obvious reasons.
		 */
		if ((fcnp->cn_namelen == 1 && fcnp->cn_nameptr[0] == '.') ||
		    dp == ip || (fcnp->cn_flags&ISDOTDOT) ||
		    (ip->i_flag & UFS211_IRENAME)) {
			VOP_UNLOCK(fvp, 0, p);
			error = EINVAL;
			goto abortit;
		}
		ip->i_flag |= UFS211_IRENAME;
		oldparent = dp->i_number;
		doingdirectory++;
	}
	vrele(fdvp);

	/*
	 * When the target exists, both the directory
	 * and target vnodes are returned locked.
	 */
	dp = UFS211_VTOI(tdvp);
	xp = NULL;
	if (tvp)
		xp = UFS211_VTOI(tvp);

	/*
	 * 1) Bump link count while we're moving stuff
	 *    around.  If we crash somewhere before
	 *    completing our work, the link count
	 *    may be wrong, but correctable.
	 */
	//ip->i_effnlink++;
	ip->i_nlink++;
	ip->i_din->di_nlink = ip->i_nlink;
	ip->i_flag |= UFS211_ICHG;
	tv = time;
	if (error = VOP_UPDATE(fvp, &tv, &tv, 1)) {
		VOP_UNLOCK(fvp, 0, p);
		goto bad;
	}

	/*
	 * If ".." must be changed (ie the directory gets a new
	 * parent) then the source directory must not be in the
	 * directory heirarchy above the target, as this would
	 * orphan everything below the source directory. Also
	 * the user must have write permission in the source so
	 * as to be able to change "..". We must repeat the call
	 * to namei, as the parent directory is unlocked by the
	 * call to checkpath().
	 */
	error = VOP_ACCESS(fvp, VWRITE, tcnp->cn_cred, tcnp->cn_proc);
	VOP_UNLOCK(fvp, 0, p);
	if (oldparent != dp->i_number)
		newparent = dp->i_number;
	if (doingdirectory && newparent) {
		if (error)	/* write access check above */
			goto bad;
		if (xp != NULL)
			vput(tvp);
		if ((error = ufs211_checkpath(ip, dp)))
			goto out;
		if ((tcnp->cn_flags & SAVESTART) == 0)
			panic("ufs_rename: lost to startdir");
		if ((error = relookup(tdvp, &tvp, tcnp)))
			goto out;
		dp = UFS211_VTOI(tdvp);
		xp = NULL;
		if (tvp)
			xp = UFS211_VTOI(tvp);
	}
	/*
	 * 2) If target doesn't exist, link the target
	 *    to the source and unlink the source.
	 *    Otherwise, rewrite the target directory
	 *    entry to reference the source inode and
	 *    expunge the original entry's existence.
	 */
	if (xp == NULL) {
		if (dp->i_dev != ip->i_dev)
			panic("rename: EXDEV");
		/*
		 * Account for ".." in new directory.
		 * When source and destination have the same
		 * parent we don't fool with the link count.
		 */
		if (doingdirectory && newparent) {
			if ((nlink_t)dp->i_nlink >= LINK_MAX) {
				error = EMLINK;
				goto bad;
			}
			//dp->i_effnlink++;
			dp->i_nlink++;
			dp->i_din->di_nlink = dp->i_nlink;
			dp->i_flag |= UFS211_ICHG;
			if ((error = VOP_UPDATE(tdvp, &tv, &tv, 1)))
				goto bad;
		}
		if ((error = ufs211_direnter(ip, tdvp, tcnp))) {
			if (doingdirectory && newparent) {
				dp->i_nlink--;
				dp->i_flag |= UFS211_ICHG;
				(void)VOP_UPDATE(tdvp, &tv, &tv, 1);
			}
			goto bad;
		}
		vput(tdvp);
	} else {
		if (xp->i_dev != dp->i_dev || xp->i_dev != ip->i_dev)
			panic("rename: EXDEV");
		/*
		 * Short circuit rename(foo, foo).
		 */
		if (xp->i_number == ip->i_number)
			panic("rename: same file");
		/*
		 * If the parent directory is "sticky", then the user must
		 * own the parent directory, or the destination of the rename,
		 * otherwise the destination may not be changed (except by
		 * root). This implements append-only directories.
		 */
		if ((dp->i_mode & S_ISTXT) && tcnp->cn_cred->cr_uid != 0 &&
		    tcnp->cn_cred->cr_uid != dp->i_uid &&
		    xp->i_uid != tcnp->cn_cred->cr_uid) {
			error = EPERM;
			goto bad;
		}
		/*
		 * Target must be empty if a directory and have no links
		 * to it. Also, ensure source and target are compatible
		 * (both directories, or both not directories).
		 */
		if ((xp->i_mode & UFS211_IFMT) == UFS211_IFDIR) {
			if (!ufs211_dirempty(xp, dp->i_number) || xp->i_nlink > 2) {
				error = ENOTEMPTY;
				goto bad;
			}
			if (!doingdirectory) {
				error = ENOTDIR;
				goto bad;
			}
			cache_purge(tdvp);
		} else if (doingdirectory) {
			error = EISDIR;
			goto bad;
		}
		if ((error = ufs211_dirrewrite(dp, ip, tcnp)))
			goto bad;
		/*
		 * If the target directory is in the same
		 * directory as the source directory,
		 * decrement the link count on the parent
		 * of the target directory.
		 */
		 if (doingdirectory && !newparent) {
			dp->i_nlink--;
			dp->i_flag |= UFS211_ICHG;
		}
		vput(tdvp);
		/*
		 * Adjust the link count of the target to
		 * reflect the dirrewrite above.  If this is
		 * a directory it is empty and there are
		 * no links to it, so we can squash the inode and
		 * any space associated with it.  We disallowed
		 * renaming over top of a directory with links to
		 * it above, as the remaining link would point to
		 * a directory without "." or ".." entries.
		 */
		xp->i_nlink--;
		if (doingdirectory) {
			if (--xp->i_nlink != 0)
				panic("rename: linked directory");
			error = VOP_TRUNCATE(tvp, (off_t)0, IO_SYNC, tcnp->cn_cred, tcnp->cn_proc);
		}
		xp->i_flag |= UFS211_ICHG;
		vput(tvp);
		xp = NULL;
	}

	/*
	 * 3) Unlink the source.
	 */
	fcnp->cn_flags &= ~MODMASK;
	fcnp->cn_flags |= LOCKPARENT | LOCKLEAF;
	if ((fcnp->cn_flags & SAVESTART) == 0)
		panic("ufs_rename: lost from startdir");
	(void) relookup(fdvp, &fvp, fcnp);
	if (fvp != NULL) {
		xp = UFS211_VTOI(fvp);
		dp = UFS211_VTOI(fdvp);
	} else {
		/*
		 * From name has disappeared.
		 */
		if (doingdirectory)
			panic("rename: lost dir entry");
		vrele(ap->a_fvp);
		return (0);
	}
	/*
	 * Ensure that the directory entry still exists and has not
	 * changed while the new name has been entered. If the source is
	 * a file then the entry may have been unlinked or renamed. In
	 * either case there is no further work to be done. If the source
	 * is a directory then it cannot have been rmdir'ed; its link
	 * count of three would cause a rmdir to fail with ENOTEMPTY.
	 * The IRENAME flag ensures that it cannot be moved by another
	 * rename.
	 */

	if (xp != ip) {
		if (doingdirectory)
			panic("rename: lost dir entry");
	} else {
		/*
		 * If the source is a directory with a
		 * new parent, the link count of the old
		 * parent directory must be decremented
		 * and ".." set to point to the new parent.
		 */
		if (doingdirectory && newparent) {
			dp->i_nlink--;
			dp->i_flag |= UFS211_ICHG;
			error = vn_rdwr(UIO_READ, fvp, (caddr_t)&dirbuf,
				sizeof (struct ufs211_dirtemplate), (off_t)0,
				UIO_SYSSPACE, IO_NODELOCKED,
				tcnp->cn_cred, (int *)0, (struct proc *)0);
			if (error == 0) {
#if (BYTE_ORDER == LITTLE_ENDIAN)
					if (fvp->v_mount->mnt_maxsymlinklen <= 0) {
						namlen = dirbuf.dotdot_type;
					} else
						namlen = dirbuf.dotdot_namlen;
#else
					namlen = dirbuf.dotdot_namlen;
#endif
				if (namlen != 2 ||
				    dirbuf.dotdot_name[0] != '.' ||
				    dirbuf.dotdot_name[1] != '.') {
					ufs211_dirbad(xp, (ufs211_doff_t)12,
					    "rename: mangled dir");
				} else {
					dirbuf.dotdot_ino = newparent;
					(void) vn_rdwr(UIO_WRITE, fvp,
					    (caddr_t)&dirbuf,
					    sizeof (struct ufs211_dirtemplate),
					    (off_t)0, UIO_SYSSPACE,
					    IO_NODELOCKED|IO_SYNC,
					    tcnp->cn_cred, (int *)0,
					    (struct proc *)0);
					cache_purge(fdvp);
				}
			}
		}
		error = ufs211_dirremove(fdvp, fcnp);
		if (!error) {
			xp->i_nlink--;
			xp->i_flag |= UFS211_ICHG;
		}
		xp->i_flag &= ~UFS211_IRENAME;
	}
	if (dp)
		vput(fdvp);
	if (xp)
		vput(fvp);
	vrele(ap->a_fvp);
	return (error);

bad:
	if (xp)
		vput(UFS211_ITOV(xp));
	vput(UFS211_ITOV(dp));
out:
	if (doingdirectory)
		ip->i_flag &= ~UFS211_IRENAME;
	if (vn_lock(fvp, LK_EXCLUSIVE, p) == 0) {
		//ip->i_effnlink--;
		ip->i_nlink--;
		ip->i_din->di_nlink = ip->i_nlink;
		ip->i_flag |= UFS211_ICHG;
		ip->i_flag &= ~UFS211_IRENAME;
		vput(fvp);
	} else
		vrele(fvp);
	return (error);
}

int
ufs211_readdir(ap)
	struct vop_readdir_args *ap;
{
	register struct uio *uio = ap->a_uio;
	int error;
	size_t count, lost;
	off_t off = uio->uio_offset;

	count = uio->uio_resid;
	/* Make sure we don't return partial entries. */
	count -= (uio->uio_offset + count) & (UFS211_DIRBLKSIZ - 1);
	if (count <= 0)
		return (EINVAL);
	lost = uio->uio_resid - count;
	uio->uio_resid = count;
	uio->uio_iov->iov_len = count;
#if (BYTE_ORDER == LITTLE_ENDIAN)
	if (ap->a_vp->v_mount->mnt_maxsymlinklen > 0) {
		error = VOP_READ(ap->a_vp, uio, 0, ap->a_cred);
	} else {
		struct dirent *dp, *edp;
		struct uio auio;
		struct iovec aiov;
		caddr_t dirbuf;
		int readcnt;
		u_char tmp;

		auio = *uio;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_segflg = UIO_SYSSPACE;
		aiov.iov_len = count;
		MALLOC(dirbuf, caddr_t, count, M_TEMP, M_WAITOK);
		aiov.iov_base = dirbuf;
		error = VOP_READ(ap->a_vp, &auio, 0, ap->a_cred);
		if (error == 0) {
			readcnt = count - auio.uio_resid;
			edp = (struct dirent*) &dirbuf[readcnt];
			for (dp = (struct dirent*) dirbuf; dp < edp;) {
				tmp = dp->d_namlen;
				dp->d_namlen = dp->d_type;
				dp->d_type = tmp;
				if (dp->d_reclen > 0) {
					dp = (struct dirent*) ((char*) dp + dp->d_reclen);
				} else {
					error = EIO;
					break;
				}
			}
			if (dp >= edp)
				error = uiomove(dirbuf, readcnt, uio);
		}
		FREE(dirbuf, M_TEMP);
	}
#else
		error = VOP_READ(ap->a_vp, uio, 0, ap->a_cred);
#endif
	if (!error && ap->a_ncookies) {
		struct dirent *dp, *dpstart;
		off_t offstart;
		u_long *cookies;
		int ncookies;

		/*
		 * Only the NFS server uses cookies, and it loads the
		 * directory block into system space, so we can just look at
		 * it directly.
		 */
		if (uio->uio_segflg != UIO_SYSSPACE || uio->uio_iovcnt != 1)
			panic("ufs_readdir: lost in space");
		dpstart = (struct dirent*) (uio->uio_iov->iov_base
				- (uio->uio_offset - off));
		offstart = off;
		for (dp = dpstart, ncookies = 0; off < uio->uio_offset;) {
			if (dp->d_reclen == 0)
				break;
			off += dp->d_reclen;
			ncookies++;
			dp = (struct dirent*) ((caddr_t) dp + dp->d_reclen);
		}
		lost += uio->uio_offset - off;
		uio->uio_offset = off;
		MALLOC(cookies, u_long *, ncookies * sizeof(u_long), M_TEMP, M_WAITOK);
		*ap->a_ncookies = ncookies;
		*ap->a_cookies = cookies;
		for (off = offstart, dp = dpstart; off < uio->uio_offset;) {
			*(cookies++) = off;
			off += dp->d_reclen;
			dp = (struct dirent*) ((caddr_t) dp + dp->d_reclen);
		}
	}
	uio->uio_resid += lost;
	*ap->a_eofflag = UFS211_VTOI(ap->a_vp)->i_size <= uio->uio_offset;
	return (error);
}

int
ufs211_abortop(ap)
	struct vop_abortop_args *ap;
{
	if ((ap->a_cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF)
		FREE(ap->a_cnp->cn_pnbuf, M_NAMEI);
	return (0);
}

int
ufs211_inactive(ap)
	struct vop_inactive_args *ap;
{
	struct vnode *vp = ap->a_vp;
	struct ufs211_inode *ip = UFS211_VTOI(vp);
	struct proc *p = ap->a_p;
	struct timeval tv;
	int mode, error = 0;
	extern int prtactive;

	if (prtactive && vp->v_usecount != 0)
		vprint("ufs211_inactive: pushing active", vp);

	/*
	 * Ignore inodes related to stale file handles.
	 */
	if (ip->i_mode == 0)
		goto out;
	if (ip->i_nlink <= 0 && (vp->v_mount->mnt_flag & MNT_RDONLY) == 0) {
#ifdef QUOTA
        if (!ufs211_getinoquota(ip)) {
            (void)ufs211_chkiq(ip, -1, NOCRED, 0);
        }
#endif
		error = VOP_TRUNCATE(vp, (off_t)0, 0, NOCRED, p);
		ip->i_rdev = 0;
		mode = ip->i_mode;
		ip->i_mode = 0;
		ip->i_flag |= UFS211_ICHG | UFS211_IUPD;
		VOP_VFREE(vp, ip->i_number, mode);
	}
	if (ip->i_flag & (UFS211_IACC | UFS211_ICHG | UFS211_IMOD | UFS211_IUPD)) {
		tv = time;
		VOP_UPDATE(vp, &tv, &tv, 0);
	}
out:
	VOP_UNLOCK(vp, 0, p);
	/*
	 * If we are done with the inode, reclaim it
	 * so that it can be reused immediately.
	 */
	if (ip->i_mode == 0)
		vrecycle(vp, 0, p);
	return (error);
}

int
ufs211_reclaim(ap)
	struct vop_reclaim_args *ap;
{
	register struct ufs211_inode *ip;
	struct vnode *vp = ap->a_vp;
	int i;
	extern int prtactive;

	if (prtactive && vp->v_usecount != 0)
		vprint("ufs_reclaim: pushing active", vp);
	/*
	 * Remove the inode from its hash chain.
	 */
	ip = UFS211_VTOI(vp);
	ufs211_ihashrem(ip);
	/*
	 * Purge old data structures associated with the inode.
	 */
	cache_purge(vp);
	if (ip->i_devvp) {
		vrele(ip->i_devvp);
		ip->i_devvp = 0;
	}
#ifdef QUOTA
	for (i = 0; i < MAXQUOTAS; i++) {
		if (ip->i_dquot[i] != NODQUOT) {
			ufs211_dqrele(vp, ip->i_dquot[i]);
			ip->i_dquot[i] = NODQUOT;
		}
	}
#endif
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

	return (lockmgr(&UFS211_VTOI(vp)->i_lock, ap->a_flags, &vp->v_interlock, ap->a_p->p_pid));
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
int
ufs211_unlock(ap)
	struct vop_unlock_args *ap;
{
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&UFS211_VTOI(vp)->i_lock, ap->a_flags | LK_RELEASE, &vp->v_interlock, ap->a_p->p_pid));
}

int
ufs211_bmap(ap)
	struct vop_bmap_args *ap;
{
	struct ufs211_inode *ip = UFS211_VTOI(ap->a_vp);

	if(ap->a_vpp != NULL) {
		ap->a_vpp = &ip->i_devvp;
	}
	if (ap->a_bnp == NULL || ufs211_bmap1(ip, ap->a_bn, UFS211_IREAD, ip->i_flag)) {
		return (0);
	}
	return (0);
}

int
ufs211_strategy(ap)
	struct vop_strategy_args *ap;
{
	register struct buf *bp = ap->a_bp;
	register struct vnode *vp = bp->b_vp;
	struct ufs211_inode *ip = UFS211_VTOI(vp);
	daddr_t blkno;
	int error;

	if (vp->v_type == VBLK || vp->v_type == VCHR)
			panic("ufs211_strategy: spec");
	if (bp->b_blkno == bp->b_lblkno) {
		//error = ufs211_bmap1(ip, bp->b_lblkno, &blkno, NULL);
		error = VOP_BMAP(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL);
		bp->b_blkno = blkno;
		if (error) {
			bp->b_error = error;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return (error);
		}
		if ((long) bp->b_blkno == -1)
			clrbuf(bp);
	}
	if ((long) bp->b_blkno == -1) {
		biodone(bp);
		return (0);
	}
	vp = ip->i_devvp;
	bp->b_dev = vp->v_rdev;
	VOCALL(vp->v_op, &ap->a_head);
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
	return (lockstatus(&UFS211_VTOI(ap->a_vp)->i_lock));
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
	struct vnode *vp = ap->a_vp;
	struct vnode *tdvp = ap->a_tdvp;
	struct componentname *cnp = ap->a_cnp;
	struct proc *p = cnp->cn_proc;
	struct ufs211_inode *ip;
	struct timeval tv;
	int error;

#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & HASBUF) == 0)
		panic("ufs_link: no name");
#endif
	if (tdvp->v_mount != vp->v_mount) {
		VOP_ABORTOP(tdvp, cnp);
		error = EXDEV;
		goto out2;
	}
	if (tdvp != vp && (error = vn_lock(vp, LK_EXCLUSIVE, p))) {
		VOP_ABORTOP(tdvp, cnp);
		goto out2;
	}
	ip = UFS211_VTOI(vp);
	if ((nlink_t)ip->i_nlink >= LINK_MAX) {
		VOP_ABORTOP(tdvp, cnp);
		error = EMLINK;
		goto out1;
	}
	if (ip->i_flags & (IMMUTABLE | APPEND)) {
		VOP_ABORTOP(tdvp, cnp);
		error = EPERM;
		goto out1;
	}
	ip->i_nlink++;
	ip->i_flag |= UFS211_ICHG;
	tv = time;
	error = VOP_UPDATE(vp, &tv, &tv, 1);
	if (!error)
		error = ufs211_direnter(ip, tdvp, cnp);
	if (error) {
		ip->i_nlink--;
		ip->i_flag |= UFS211_ICHG;
	}
	FREE(cnp->cn_pnbuf, M_NAMEI);
out1:
	if (tdvp != vp)
		VOP_UNLOCK(vp, 0, p);
out2:
	vput(tdvp);
	return (error);
}

/*
 * whiteout vnode call
 */
int
ufs211_whiteout(ap)
	struct vop_whiteout_args /* {
		struct vnode *a_dvp;
		struct componentname *a_cnp;
		int a_flags;
	} */ *ap;
{
	struct ufs211_inode *ip;
	struct vnode *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	struct direct newdir;
	int error;

	ip = UFS211_VTOI(dvp);
	switch (ap->a_flags) {
	case LOOKUP:
		/* 4.4 format directories support whiteout operations */
		if (dvp->v_mount->mnt_maxsymlinklen > 0)
			return (0);
		return (EOPNOTSUPP);

	case CREATE:
		/* create a new directory whiteout */
#ifdef DIAGNOSTIC
		if ((cnp->cn_flags & SAVENAME) == 0)
			panic("ufs_whiteout: missing name");
		if (dvp->v_mount->mnt_maxsymlinklen <= 0)
			panic("ufs_whiteout: old format filesystem");
#endif

		newdir.d_ino = UFS211_WINO;
		newdir.d_namlen = cnp->cn_namelen;
		bcopy(cnp->cn_nameptr, newdir.d_name, (unsigned)cnp->cn_namelen + 1);
		newdir.d_type = DT_WHT;
		error = ufs211_direnter2(ip, &newdir, dvp, cnp);
		break;

	case DELETE:
		/* remove an existing directory whiteout */
#ifdef DIAGNOSTIC
		if (dvp->v_mount->mnt_maxsymlinklen <= 0)
			panic("ufs_whiteout: old format filesystem");
#endif

		cnp->cn_flags &= ~DOWHITEOUT;
		error = ufs211_dirremove(dvp, cnp);
		break;
	}
	if (cnp->cn_flags & HASBUF) {
		FREE(cnp->cn_pnbuf, M_NAMEI);
		cnp->cn_flags &= ~HASBUF;
	}
	return (error);
}

int
ufs211_symlink(ap)
	struct vop_symlink_args *ap;
{
	register struct vnode *vp, **vpp = ap->a_vpp;
	register struct ufs211_inode *ip;
	int len, error;

	if ((error = ufs211_makeinode(UFS211_IFLNK | ap->a_vap->va_mode, ap->a_dvp, vpp, ap->a_cnp)))
		return (error);
	vp = *vpp;
	len = strlen(ap->a_target);
	if (len < vp->v_mount->mnt_maxsymlinklen) {
		ip = UFS211_VTOI(vp);
		bcopy(ap->a_target, (char*) ip->i_db, len);
		ip->i_size = len;
		ip->i_flag |= UFS211_ICHG | UFS211_IUPD;
	} else {
		error = vn_rdwr(UIO_WRITE, vp, ap->a_target, len, (off_t) 0,
				UIO_SYSSPACE, IO_NODELOCKED, ap->a_cnp->cn_cred, (int*) 0,
				(struct proc*) 0);
	}
	vput(vp);
	return (error);
}

int
ufs211_readlink(ap)
	struct vop_readlink_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct ufs211_inode *ip = UFS211_VTOI(vp);
	int isize;

	isize = ip->i_size;
	if (isize < vp->v_mount->mnt_maxsymlinklen || ip->i_din->di_blocks == 0) {
		uiomove((char *)ip->i_db, isize, ap->a_uio);
		return (0);
	}
	return (VOP_READ(vp, ap->a_uio, 0, ap->a_cred));
}

/*
 * A virgin directory (no blushing please).
 */
static struct ufs211_dirtemplate mastertemplate = {
	0, 12, DT_DIR, 1, ".",
	0, UFS211_DIRBLKSIZ - 12, DT_DIR, 2, ".."
};
static struct ufs211_odirtemplate omastertemplate = {
	0, 12, 1, ".",
	0, UFS211_DIRBLKSIZ - 12, 2, ".."
};

int
ufs211_mkdir(ap)
	struct vop_mkdir_args *ap;
{
	register struct vnode *dvp = ap->a_dvp;
	register struct vattr *vap = ap->a_vap;
	register struct componentname *cnp = ap->a_cnp;
	register struct ufs211_inode *ip, *dp;
	struct vnode *tvp;
	struct ufs211_dirtemplate dirtemplate, *dtp;
	struct timeval tv;
	int error, dmode;

#ifdef DIAGNOSTIC
	if ((cnp->cn_flags & HASBUF) == 0)
		panic("ufs_mkdir: no name");
#endif
	dp = UFS211_VTOI(dvp);
	if ((nlink_t)dp->i_nlink >= LINK_MAX) {
		error = EMLINK;
		goto out;
	}
	dmode = vap->va_mode & 0777;
	dmode |= UFS211_IFDIR;
	/*
	 * Must simulate part of ufs_makeinode here to acquire the inode,
	 * but not have it entered in the parent directory. The entry is
	 * made later after writing "." and ".." entries.
	 */
	if ((error = VOP_VALLOC(dvp, dmode, cnp->cn_cred, &tvp)))
		goto out;
	ip = UFS211_VTOI(tvp);
	ip->i_uid = cnp->cn_cred->cr_uid;
	ip->i_gid = dp->i_gid;
#ifdef QUOTA
	if ((error = ufs211_getinoquota(ip)) || (error = ufs211_chkiq(ip, 1, cnp->cn_cred, 0))) {
		free(cnp->cn_pnbuf, M_NAMEI);
		VOP_VFREE(tvp, ip->i_number, dmode);
		vput(tvp);
		vput(dvp);
		return (error);
	}
#endif
	ip->i_flag |= UFS211_IACC | UFS211_ICHG | UFS211_IUPD;
	ip->i_mode = dmode;
	tvp->v_type = VDIR;	/* Rest init'd in getnewvnode(). */
	ip->i_nlink = 2;
	if (cnp->cn_flags & ISWHITEOUT)
		ip->i_flags |= UF_OPAQUE;
	tv = time;
	error = VOP_UPDATE(tvp, &tv, &tv, 1);

	/*
	 * Bump link count in parent directory
	 * to reflect work done below.  Should
	 * be done before reference is created
	 * so reparation is possible if we crash.
	 */
	dp->i_nlink++;
	dp->i_flag |= UFS211_ICHG;
	if ((error = VOP_UPDATE(dvp, &tv, &tv, 1)))
		goto bad;

	/* Initialize directory with "." and ".." from static template. */
	if (dvp->v_mount->mnt_maxsymlinklen > 0)
		dtp = &mastertemplate;
	else
		dtp = (struct ufs211_dirtemplate *)&omastertemplate;
	dirtemplate = *dtp;
	dirtemplate.dot_ino = ip->i_number;
	dirtemplate.dotdot_ino = dp->i_number;
	error = vn_rdwr(UIO_WRITE, tvp, (caddr_t)&dirtemplate,
	    sizeof (dirtemplate), (off_t)0, UIO_SYSSPACE,
	    IO_NODELOCKED|IO_SYNC, cnp->cn_cred, (int *)0, (struct proc *)0);
	if (error) {
		dp->i_nlink--;
		dp->i_flag |= UFS211_ICHG;
		goto bad;
	}
	if (UFS211_DIRBLKSIZ > VFSTOUFS211(dvp->v_mount)->m_mountp->mnt_stat.f_bsize)
		panic("ufs_mkdir: blksize"); /* XXX should grow with balloc() */
	else {
		ip->i_size = UFS211_DIRBLKSIZ;
		ip->i_flag |= UFS211_ICHG;
	}

	/* Directory set up, now install it's entry in the parent directory. */
	if (error = ufs211_direnter(ip, dvp, cnp)) {
		dp->i_nlink--;
		dp->i_flag |= UFS211_ICHG;
	}
bad:
	/*
	 * No need to do an explicit VOP_TRUNCATE here, vrele will do this
	 * for us because we set the link count to 0.
	 */
	if (error) {
		ip->i_nlink = 0;
		ip->i_flag |= UFS211_ICHG;
		vput(tvp);
	} else
		*ap->a_vpp = tvp;
out:
	FREE(cnp->cn_pnbuf, M_NAMEI);
	vput(dvp);
	return (error);
}

int
ufs211_rmdir(ap)
	struct vop_rmdir_args *ap;
{
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
	struct componentname *cnp = ap->a_cnp;
	struct ufs211_inode *ip, *dp;
	int error;

	ip = UFS211_VTOI(vp);
	dp = UFS211_VTOI(dvp);
	/*
	 * No rmdir "." please.
	 */
	if (dp == ip) {
		vrele(dvp);
		vput(vp);
		return (EINVAL);
	}
	/*
	 * Verify the directory is empty (and valid).
	 * (Rmdir ".." won't be valid since
	 *  ".." will contain a reference to
	 *  the current directory and thus be
	 *  non-empty.)
	 */
	error = 0;
	if (ip->i_nlink != 2 ||
	    !ufs211_dirempty(ip, dp->i_number)) {
		error = ENOTEMPTY;
		goto out;
	}
	if ((dp->i_flags & APPEND) || (ip->i_flags & (IMMUTABLE | APPEND))) {
		error = EPERM;
		goto out;
	}
	/*
	 * Delete reference to directory before purging
	 * inode.  If we crash in between, the directory
	 * will be reattached to lost+found,
	 */
	if ((error = ufs211_dirremove(dvp, cnp)))
		goto out;
	dp->i_nlink--;
	dp->i_flag |= UFS211_ICHG;
	cache_purge(dvp);
	vput(dvp);
	dvp = NULL;
	/*
	 * Truncate inode.  The only stuff left
	 * in the directory is "." and "..".  The
	 * "." reference is inconsequential since
	 * we're quashing it.  The ".." reference
	 * has already been adjusted above.  We've
	 * removed the "." reference and the reference
	 * in the parent directory, but there may be
	 * other hard links so decrement by 2 and
	 * worry about them later.
	 */
	ip->i_nlink -= 2;
	error = VOP_TRUNCATE(vp, (off_t)0, IO_SYNC, cnp->cn_cred, cnp->cn_proc);
	cache_purge(UFS211_ITOV(ip));
out:
	if (dvp)
		vput(dvp);
	vput(vp);
	return (error);
}

int
ufs211_mknod(ap)
	struct vop_mknod_args *ap;
{
	struct vattr *vap = ap->a_vap;
	struct vnode **vpp = ap->a_vpp;
	register struct ufs211_inode *ip;

	if (u.u_error == ufs211_makeinode(MAKEIMODE(vap->va_type, vap->va_mode), ap->a_dvp, vpp, ap->a_cnp))
		return (u.u_error);
	ip = UFS211_VTOI(*vpp);
	ip->i_flag |= UFS211_IACC | UFS211_ICHG | UFS211_IUPD;
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
	struct ufs211_inode *ip;
	register struct ufs211_fs *fs;
	struct buf *bp;
	daddr_t lbn, bn;
	char *junk;
	int error;

	ip = UFS211_VTOI(ap->a_vp);
	fs = ip->i_fs;
	lbn = lblkno(ap->a_offset);
	bn = ufs211_bmap1(ip, lbn, B_READ, 0);

	if (u.u_error) {
		return (u.u_error);
	}
	if (bn == (daddr_t)-1) {
		ufs211_dirbad(ip, ap->a_offset, "hole in dir");
	}

	if (error = bread(ap->a_vp, lbn, bn, NOCRED, &bp)) {
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (error);
		}
	}
	junk = (caddr_t)bp->b_data;
	if (ap->a_res) {
		*ap->a_res = junk + (u_int)blkoff(ap->a_offset);
	}
	*ap->a_bpp = bp;
	return (0);
}

int
ufs211_valloc(ap)
	struct vop_valloc_args *ap;
{
	/* TODO:
	 * a modified version of ufs211's balloc(ip, flags)
	 */

	return (0);
}

int
ufs211_vfree(ap)
	struct vop_vfree_args *ap;
{
	/* TODO:
	 * a modified version of ufs211's free(ip, bno)
	 */
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
	register struct ufs211_inode *ip;

	ip = UFS211_VTOI(ap->a_vp);
	ufs211_trunc(ip, ap->a_length, ap->a_flags);
	return (0);
}

int
ufs211_update(ap)
	struct vop_update_args *ap;
{
	register struct ufs211_inode *ip;

	ip = UFS211_VTOI(ap->a_vp);
	ufs211_updat(ip, ap->a_access, ap->a_modify, ap->a_waitfor);
	return (0);
}

int
ufs211_bwrite(ap)
	struct vop_bwrite_args *ap;
{
	return (0);
}

/*
 * Read wrapper for special devices.
 */
int
ufs211spec_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	/*
	 * Set access flag.
	 */
	int error, resid;
	struct ufs211_inode *ip;
	struct uio *uio;

	uio = ap->a_uio;
	resid = uio->uio_resid;
	error = VOCALL(&spec_vnodeops, &ap->a_head);

	ip = UFS211_VTOI(ap->a_vp);
	if (ip != NULL && (uio->uio_resid != resid || (error == 0 && resid != 0))) {
		ip->i_flag |= UFS211_IACC;
	}
	return (error);
}

/*
 * Write wrapper for special devices.
 */
int
ufs211spec_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	/*
	 * Set update and change flags.
	 */
	int error, resid;
	struct ufs211_inode *ip;
	struct uio *uio;

	uio = ap->a_uio;
	resid = uio->uio_resid;
	error = VOCALL(&spec_vnodeops, &ap->a_head);

	ip = UFS211_VTOI(ap->a_vp);
	if (ip != NULL && (uio->uio_resid != resid || (error == 0 && resid != 0))) {
		UFS211_VTOI(ap->a_vp)->i_flag |= UFS211_ICHG | UFS211_IUPD;
	}
	return (error);
}

/*
 * Close wrapper for special devices.
 *
 * Update the times on the inode then do device close.
 */
int
ufs211spec_close(ap)
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	simple_lock(&vp->v_interlock);
	if (ap->a_vp->v_usecount > 1)
		ufs211_itimes(vp);
	simple_unlock(&vp->v_interlock);
	return (VOCALL(&spec_vnodeops, &ap->a_head));
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
	UFS211_VTOI(ap->a_vp)->i_flag |= UFS211_IACC;

	return (VOCALL(&fifo_vnodeops, &ap->a_head));
}

int
ufs211fifo_write(ap)
	struct vop_write_args *ap;
{
	extern struct fifo_vnodeops;

	/*
	 * Set update and change flags.
	 */
	UFS211_VTOI(ap->a_vp)->i_flag |= UFS211_ICHG | UFS211_IUPD;

	return (VOCALL(&fifo_vnodeops, &ap->a_head));
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
		ufs211_itimes(vp);
	simple_unlock(&vp->v_interlock);
	return (VOCALL(&fifo_vnodeops, &ap->a_head));
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

	if ((error = VOP_VALLOC(dvp, mode, cnp->cn_cred, &tvp))) {
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
	if ((error = ufs211_getinoquota(ip)) ||
	    (error = ufs211_chkiq(ip, 1, cnp->cn_cred, 0))) {
		free(cnp->cn_pnbuf, M_NAMEI);
		VOP_VFREE(tvp, ip->i_number, mode);
		vput(tvp);
		vput(dvp);
		return (error);
	}
#endif
	ip->i_flag |= UFS211_IACC | UFS211_ICHG | UFS211_IUPD;
	ip->i_mode = mode;
	tvp->v_type = IFTOVT(mode);	/* Rest init'd in getnewvnode(). */
	ip->i_nlink = 1;
	if ((ip->i_mode & UFS211_ISGID) && !groupmember(ip->i_gid) && suser())
		ip->i_mode &= ~UFS211_ISGID;

	if (cnp->cn_flags & ISWHITEOUT)
		ip->i_flags |= UF_OPAQUE;

	/*
	 * Make sure inode goes to disk before directory entry.
	 */
	tv = time;
	if (error = VOP_UPDATE(tvp, &tv, &tv, 1))
		goto bad;
	if (error = ufs211_direnter(ip, dvp, cnp))
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
	ip->i_flag |= UFS211_ICHG;
	vput(tvp);
	return (error);
}
