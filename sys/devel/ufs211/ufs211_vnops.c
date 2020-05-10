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

#include <ufs211/ufs211_fs.h>
#include <ufs211/ufs211_quota.h>
#include <ufs211/ufs211_inode.h>
#include <ufs211/ufs211_dir.h>
#include <ufs211/ufs211_extern.h>
#include <ufs211/ufs211_mount.h>

#include <vm/include/vm.h>
#include <miscfs/specfs/specdev.h>

int (**ufs211_vnodeop_p)();
struct vnodeopv_entry_desc ufs211_vnodeop_entries[] = {
	{ &vop_default_desc, ufs211_default_error },
	{ &vop_lookup_desc, ufs211_lookup },		/* lookup */
	{ &vop_create_desc, ufs211_create },		/* create */
	{ &vop_mknod_desc, ufs211_mknod },			/* mknod */
	{ &vop_open_desc, ufs211_open },			/* open */
	{ &vop_close_desc, ufs211_close },			/* close */
	{ &vop_access_desc, ufs211_access },		/* access */
	{ &vop_getattr_desc, ufs211_getattr },		/* getattr */
	{ &vop_setattr_desc, ufs211_setattr },		/* setattr */
	{ &vop_read_desc, ufs211_read },			/* read */
	{ &vop_write_desc, ufs211_write },			/* write */
	{ &vop_lease_desc, ufs211_lease_check },	/* lease */
	{ &vop_ioctl_desc, ufs211_ioctl },			/* ioctl */
	{ &vop_select_desc, ufs211_select },		/* select */
	{ &vop_revoke_desc, ufs211_revoke },		/* revoke */
	{ &vop_mmap_desc, ufs211_mmap },			/* mmap */
	{ &vop_fsync_desc, ufs211_fsync },			/* fsync */
	{ &vop_seek_desc, ufs211_seek },			/* seek */
	{ &vop_remove_desc, ufs211_remove },		/* remove */
	{ &vop_link_desc, ufs211_link },			/* link */
	{ &vop_rename_desc, ufs211_rename },		/* rename */
	{ &vop_mkdir_desc, ufs211_mkdir },			/* mkdir */
	{ &vop_rmdir_desc, ufs211_rmdir },			/* rmdir */
	{ &vop_symlink_desc, ufs211_symlink },		/* symlink */
	{ &vop_readdir_desc, ufs211_readdir },		/* readdir */
	{ &vop_readlink_desc, ufs211_readlink },	/* readlink */
	{ &vop_abortop_desc, ufs211_abortop },		/* abortop */
	{ &vop_inactive_desc, ufs211_inactive },	/* inactive */
	{ &vop_reclaim_desc, ufs211_reclaim },		/* reclaim */
	{ &vop_lock_desc, ufs211_lock },			/* lock */
	{ &vop_unlock_desc, ufs211_unlock },		/* unlock */
	{ &vop_bmap_desc, ufs211_bmap },			/* bmap */
	{ &vop_strategy_desc, ufs211_strategy },	/* strategy */
	{ &vop_print_desc, ufs211_print },			/* print */
	{ &vop_islocked_desc, ufs211_islocked },	/* islocked */
	{ &vop_pathconf_desc, ufs211_pathconf },	/* pathconf */
	{ &vop_advlock_desc, ufs211_advlock },		/* advlock */
	{ &vop_blkatoff_desc, ufs211_blkatoff },	/* blkatoff */
	{ &vop_valloc_desc, ufs211_valloc },		/* valloc */
	{ &vop_vfree_desc, ufs211_vfree },			/* vfree */
	{ &vop_truncate_desc, ufs211_truncate },	/* truncate */
	{ &vop_update_desc, ufs211_update },		/* update */
	{ &vop_bwrite_desc, ufs211_bwrite },		/* bwrite */
	{ (struct vnodeop_desc*)NULL, (int(*)())NULL }
};
struct vnodeopv_desc ufs211_vnodeop_opv_desc =
	{ &ufs211_vnodeop_p, ufs211_vnodeop_entries };

int
ufs211_lookup(ap)
	struct vop_lookup_args ap;
{

}

int
ufs211_create(ap)
	struct vop_create_args ap;
{

}

int
ufs211_open(ap)
	struct vop_open_args *ap;
{
	if ((VTOI(ap->a_vp)->di_flags & APPEND) && (ap->a_mode & (FWRITE | O_APPEND)) == FWRITE)
		return (EPERM);
	return (0);
}

int
ufs211_close(ap)
	struct vop_close_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct ufs211_inode *ip = VTOI(vp);

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
	register struct ufs211_inode *ip = VTOI(vp);
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
			if ((ip->i_mode & UFS211_FMT) != UFS211_FCHR &&
			    (ip->i_mode & UFS211_FMT) != UFS211_FBLK) {
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
	register struct ufs211_inode *ip = VTOI(vp);
	register struct vattr *vap = ap->a_vap;

	ITIMES(ip, &time, &time);
	/*
	 * Copy from inode table
	 */
	vap->va_fsid = ip->i_dev;
	vap->va_fileid = ip->i_number;
	vap->va_mode = ip->i_mode & ~IFMT;
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
	struct ufs211_inode *ip = VTOI(vp);
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
		ip->i_flag |= ICHG;
		if (vap->va_flags & (IMMUTABLE | APPEND))
			return (0);
	}
	if (ip->i_flags & (IMMUTABLE | APPEND))
		return (EPERM);
	/*
	 * Go thru the fields (other than 'flags') and update iff not VNOVAL.
	 */
	if (vap->va_uid != (uid_t) VNOVAL || vap->va_gid != (gid_t) VNOVAL)
		if (error == chown1(ip, vap->va_uid, vap->va_gid))
			return (error);
	if (vap->va_size != (off_t) VNOVAL) {
		if ((ip->i_mode & UFS211_FMT) == IFDIR)
			return (EISDIR);
		itrunc(ip, vap->va_size, 0);
		if (u->u_error)
			return (u->u_error);
	}
	if (vap->va_atime != (time_t) VNOVAL || vap->va_mtime != (time_t) VNOVAL) {
		if (u->u_uid != ip->i_uid && !ufs211_suser()
				&& ((vap->va_vaflags & VA_UTIMES_NULL) == 0
						|| access(ip, UFS211_WRITE)))
			return (u->u_error);
		if (vap->va_atime != (time_t) VNOVAL
				&& !(ip->i_fs->fs_flags & MNT_NOATIME))
			ip->i_flag |= IACC;
		if (vap->va_mtime != (time_t) VNOVAL)
			ip->i_flag |= (IUPD | ICHG);
		atimeval.tv_sec = vap->va_atime;
		mtimeval.tv_sec = vap->va_mtime;
		iupdat(ip, &atimeval, &mtimeval, 1);
	}
	if (vap->va_mode != (mode_t) VNOVAL)
		return (chmod1(ip, vap->va_mode));
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
		return (1);
	}
	u->u_error = EPERM;
	return (0);
}

int
ufs211_read(ap)
	struct vop_read_args *ap;
{

}

int
ufs211_write(ap)
	struct vop_write_args *ap;
{

}

int
ufs211_fsync(ap)
	struct vop_fsync_args *ap;
{

}

int
ufs211_remove(ap)
	struct vop_remove_args *ap;
{

}

int
ufs211_rename(ap)
	struct vop_rename_args *ap;
{

}

int
ufs211_readdir(ap)
	struct vop_readdir_args *ap;
{

}

int
ufs211_inactive(ap)
	struct vop_inactive_args *ap;
{

}

int
ufs211_reclaim(ap)
	struct vop_reclaim_args *ap;
{

}

/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
int
ufs211_lock(ap)
	struct vop_lock_args *ap;
{
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&VTOI(vp)->i_lock, ap->a_flags, &vp->v_interlock, ap->a_p));
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
int
ufs211_unlock(ap)
	struct vop_unlock_args *ap;
{
	struct vnode *vp = ap->a_vp;

	return (lockmgr(&VTOI(vp)->i_lock, ap->a_flags | LK_RELEASE, &vp->v_interlock, ap->a_p));
}

int
ufs211_bmap(ap)
	struct vop_bmap_args *ap;
{

}

int
ufs211_strategy(ap)
	struct vop_strategy_args *ap;
{

}

int
ufs211_print(ap)
	struct vop_print_args *ap;
{
	return (0);
}

int
ufs211_advlock(ap)
	struct vop_advlock_args *ap;
{
	struct ufs211_inode *ip = VTOI(ap->a_vp);
	return lf_advlock(ap, &ip->i_lockf, ip->i_size);
}

int
ufs211_pathconf(ap)
	struct vop_pathconf_args *ap;
{

}

int
ufs211_link(ap)
	struct vop_link_args *ap;
{

}

int
ufs211_symlink(ap)
	struct vop_symlink_args *ap;
{

}

int
ufs211_readlink(ap)
	struct vop_readlink_args *ap;
{

}


int
ufs211_mkdir(ap)
	struct vop_mkdir_args *ap;
{

}

int
ufs211_rmdir(ap)
	struct vop_rmdir_args *ap;
{

}

int
ufs211_mknod(ap)
	struct vop_mknod_args *ap;
{
	register struct ufs211_inode *ip;

}

/*
 * Truncate a file given a file descriptor.
 */
int
ufs211_truncate(ap)
	struct vop_truncate_args *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct ufs211_inode *ip =  VTOI(vp);

}

int
ufs211spec_read(ap)
	struct vop_read_args *ap;
{
	//if ((ap->a_vp->v_mount->mnt_flag & MNT_NODEVMTIME) == 0)
		VTOI(ap->a_vp)->i_flag |= IN_ACCESS;
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_read), ap));
}

/*
 * Write wrapper for special devices.
 */
int
ufs211spec_write(ap)
	struct vop_write_args *ap;
{
	//if ((ap->a_vp->v_mount->mnt_flag & MNT_NODEVMTIME) == 0)
		VTOI(ap->a_vp)->i_flag |= IN_MODIFY;
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_write), ap));
}

int
ufs211spec_close(ap)
	struct vop_close_args *ap;
{
	struct vnode *vp = ap->a_vp;
	struct ufs211_inode *ip = VTOI(vp);

	simple_lock(&vp->v_interlock);
	if (ap->a_vp->v_usecount > 1)
		ITIMES(ip, &time, &time);
	simple_unlock(&vp->v_interlock);
	return (VOCALL (spec_vnodeop_p, VOFFSET(vop_close), ap));
}

#ifdef FIFO
int
ufs211fifo_read(ap)
	struct vop_read_args *ap;
{
	extern int (**fifo_vnodeop_p)();

	VTOI(ap->a_vp)->i_flag |= IN_ACCESS;
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_read), ap));
}

int
ufs211fifo_write(ap)
	struct vop_write_args *ap;
{
	extern int (**fifo_vnodeop_p)();

	VTOI(ap->a_vp)->i_flag |= IN_CHANGE | IN_UPDATE;
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_write), ap));
}

int
ufs211fifo_close(ap)
	struct vop_close_args *ap;
{
	extern int (**fifo_vnodeop_p)();
	struct vnode *vp = ap->a_vp;
	struct ufs211_inode *ip = VTOI(vp);

	simple_lock(&vp->v_interlock);
	if (ap->a_vp->v_usecount > 1)
		ITIMES(ip, &time, &time);
	simple_unlock(&vp->v_interlock);
	return (VOCALL (fifo_vnodeop_p, VOFFSET(vop_close), ap));
}
#endif /* FIFO */
