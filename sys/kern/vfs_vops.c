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
 * @(#)vfs_vops.c	1.2
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/dirent.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/ucred.h>
#include <sys/malloc.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>
#include <sys/event.h>

#include <vm/include/vm.h>
#include <miscfs/specfs/specdev.h>

#define VNOP(vp, vop_field)	((vp)->v_op->vop_field)
#define DO_OPS(ops, error, ap, vop_field)	\
	error = ops->vop_field(ap)

struct vnodeops default_vnodeops = {
		.vop_default = vop_default_error,	/* default */
		.vop_lookup = vop_lookup,			/* lookup */
		.vop_create = vop_create,			/* create */
		.vop_mknod = vop_mknod,				/* mknod */
		.vop_open = vop_open,				/* open */
		.vop_close = vop_close,				/* close */
		.vop_access = vop_access,			/* access */
		.vop_getattr = vop_getattr,			/* getattr */
		.vop_setattr = vop_setattr,			/* setattr */
		.vop_read = vop_read,				/* read */
		.vop_write = vop_write,				/* write */
		.vop_lease = vop_lease,				/* lease */
		.vop_ioctl = vop_ioctl,				/* ioctl */
		.vop_select = vop_select,			/* select */
		.vop_poll = vop_poll,				/* poll */
		.vop_kqfilter = vop_kqfilter,		/* kqfilter */
		.vop_revoke = vop_revoke,			/* revoke */
		.vop_mmap = vop_mmap,				/* mmap */
		.vop_fsync = vop_fsync,				/* fsync */
		.vop_seek = vop_seek,				/* seek */
		.vop_remove = vop_remove,			/* remove */
		.vop_link = vop_link,				/* link */
		.vop_rename = vop_rename,			/* rename */
		.vop_mkdir = vop_mkdir,				/* mkdir */
		.vop_rmdir = vop_rmdir,				/* rmdir */
		.vop_symlink = vop_symlink,			/* symlink */
		.vop_readdir = vop_readdir,			/* readdir */
		.vop_readlink = vop_readlink,		/* readlink */
		.vop_abortop = vop_abortop,			/* abortop */
		.vop_inactive = vop_inactive,		/* inactive */
		.vop_reclaim = vop_reclaim,			/* reclaim */
		.vop_lock = vop_lock,				/* lock */
		.vop_unlock = vop_unlock,			/* unlock */
		.vop_bmap = vop_bmap,				/* bmap */
		.vop_strategy = vop_strategy,		/* strategy */
		.vop_print = vop_print,				/* print */
		.vop_islocked = vop_islocked,		/* islocked */
		.vop_pathconf = vop_pathconf,		/* pathconf */
		.vop_advlock = vop_advlock,			/* advlock */
		.vop_blkatoff = vop_blkatoff,		/* blkatoff */
		.vop_valloc = vop_valloc,			/* valloc */
		.vop_reallocblks = vop_reallocblks,	/* reallocblks */
		.vop_vfree = vop_vfree,				/* vfree */
		.vop_truncate = vop_truncate,		/* truncate */
		.vop_update = vop_update,			/* update */
		.vop_getpages = vop_getpages,			/* getpages */
		.vop_putpages = vop_putpages,			/* putpages */
		.vop_bwrite = vop_bwrite,			/* bwrite */
};

/*
 * A miscellaneous routine.
 * A generic "default" routine that just returns an error.
 */
int
vop_default_error(ap)
	struct vop_generic_args *ap;
{
	return (EOPNOTSUPP);
}

int
vop_badop(v)
	void *v;
{
	panic("vop_badop called %s", __func__);

	return (1);
}

int
vop_ebadf(void)
{
	return (EBADF);
}

int
vop_lookup(dvp, vpp, cnp)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
{
	struct vop_lookup_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_lookup);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;

	if (VNOP(dvp, vop_lookup) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(dvp->v_op, error, &a, vop_lookup);

	return (error);
}

int
vop_create(dvp, vpp, cnp, vap)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct vattr *vap;
{
	struct vop_create_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_create);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;

	if(VNOP(dvp, vop_create) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(dvp->v_op, error, &a, vop_create);

	return (error);
}

int
vop_whiteout(dvp, cnp, flags)
	struct vnode *dvp;
	struct componentname *cnp;
	int flags;
{
	struct vop_whiteout_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_whiteout);
	a.a_dvp = dvp;
	a.a_cnp = cnp;
	a.a_flags = flags;

	if(VNOP(dvp, vop_whiteout) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(dvp->v_op, error, &a, vop_whiteout);

	return (error);
}

int
vop_mknod(dvp, vpp, cnp, vap)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct vattr *vap;
{
	struct vop_mknod_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_mknod);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;

	if(VNOP(dvp, vop_mknod) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(dvp->v_op, error, &a, vop_mknod);

	return (error);
}

int
vop_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_open_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_open);
	a.a_vp = vp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_open) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_open);

	return (error);
}

int
vop_close(vp, fflag, cred, p)
	struct vnode *vp;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_close_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_close);
	a.a_vp = vp;
	a.a_fflag = fflag;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_close) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_close);

	return (error);
}

int
vop_access(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_access_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_access);
	a.a_vp = vp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_access) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_access);

	return (error);
}

int
vop_getattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_getattr_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_getattr);
	a.a_vp = vp;
	a.a_vap = vap;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_getattr) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_getattr);

	return (error);
}

int
vop_setattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_setattr_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_setattr);
	a.a_vp = vp;
	a.a_vap = vap;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_setattr) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_setattr);

	return (error);
}

int
vop_read(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	struct vop_read_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_read);
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_ioflag = ioflag;
	a.a_cred = cred;

	if(VNOP(vp, vop_read) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_read);

	return (error);
}

int
vop_write(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	struct vop_write_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_write);
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_ioflag = ioflag;
	a.a_cred = cred;

	if(VNOP(vp, vop_write) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_write);

	return (error);
}

int
vop_lease(vp, p, cred, flag)
	struct vnode *vp;
	struct proc *p;
	struct ucred *cred;
	int flag;
{
	struct vop_lease_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_lease);
	a.a_vp = vp;
	a.a_p = p;
	a.a_cred = cred;
	a.a_flag = flag;

	if(VNOP(vp, vop_lease) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_lease);

	return (error);
}

int
vop_ioctl(vp, command, data, fflag, cred, p)
	struct vnode *vp;
	u_long command;
	caddr_t data;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_ioctl_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_ioctl);
	a.a_vp = vp;
	a.a_command = command;
	a.a_data = data;
	a.a_fflag = fflag;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_ioctl) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_ioctl);

	return (error);
}

int
vop_select(vp, which, fflags, cred, p)
	struct vnode *vp;
	int which;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_select_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_select);
	a.a_vp = vp;
	a.a_which = which;
	a.a_fflags = fflags;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_select) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_select);

	return (error);
}

int
vop_poll(vp, fflags, events, p)
	struct vnode *vp;
	int fflags;
	int events;
	struct proc *p;
{
	struct vop_poll_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_poll);
	a.a_vp = vp;
	a.a_fflags = fflags;
	a.a_events = events;
	a.a_p = p;

	if(VNOP(vp, vop_poll) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_poll);

	return (error);
}

int
vop_kqfilter(vp, kn)
	struct vnode *vp;
	struct knote *kn;
{
	struct vop_kqfilter_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_kqfilter);
	a.a_vp = vp;
	a.a_kn = kn;

	if(VNOP(vp, vop_kqfilter) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_kqfilter);

	return (error);
}

int
vop_revoke(vp, flags)
	struct vnode *vp;
	int flags;
{
	struct vop_revoke_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_revoke);
	a.a_vp = vp;
	a.a_flags = flags;

	if(VNOP(vp, vop_revoke) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_revoke);

	return (error);
}

int
vop_mmap(vp, fflags, cred, p)
	struct vnode *vp;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_mmap_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_mmap);
	a.a_vp = vp;
	a.a_fflags = fflags;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_mmap) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_mmap);

	return (error);
}

int
vop_fsync(vp, cred, waitfor, flag, p)
	struct vnode *vp;
	struct ucred *cred;
	int waitfor, flag;
	struct proc *p;
{
	struct vop_fsync_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_fsync);
	a.a_vp = vp;
	a.a_cred = cred;
	a.a_waitfor = waitfor;
	a.a_flags = flag;
	a.a_p = p;

	if(VNOP(vp, vop_fsync) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_fsync);

	return (error);
}

int
vop_seek(vp, oldoff, newoff, cred)
	struct vnode *vp;
	off_t oldoff;
	off_t newoff;
	struct ucred *cred;
{
	struct vop_seek_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_seek);
	a.a_vp = vp;
	a.a_oldoff = oldoff;
	a.a_newoff = newoff;
	a.a_cred = cred;

	if(VNOP(vp, vop_seek) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_seek);

	return (error);
}

int
vop_remove(dvp, vp, cnp)
	struct vnode *dvp;
	struct vnode *vp;
	struct componentname *cnp;
{
	struct vop_remove_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_remove);
	a.a_dvp = dvp;
	a.a_vp = vp;
	a.a_cnp = cnp;

	if(VNOP(dvp, vop_remove) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_remove);

	return (error);
}

int
vop_link(vp, tdvp, cnp)
	struct vnode *vp;
	struct vnode *tdvp;
	struct componentname *cnp;
{
	struct vop_link_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_link);
	a.a_vp = vp;
	a.a_tdvp = tdvp;
	a.a_cnp = cnp;

	if(VNOP(vp, vop_link) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_link);

	return (error);
}

int
vop_rename(fdvp, fvp, fcnp, tdvp, tvp, tcnp)
	struct vnode *fdvp;
	struct vnode *fvp;
	struct componentname *fcnp;
	struct vnode *tdvp;
	struct vnode *tvp;
	struct componentname *tcnp;
{
	struct vop_rename_args a;
	int error;

	a.a_head.a_ops = fdvp->v_op;
	a.a_head.a_desc = VDESC(vop_rename);
	a.a_fdvp = fdvp;
	a.a_fvp = fvp;
	a.a_fcnp = fcnp;
	a.a_tdvp = tdvp;
	a.a_tvp = tvp;
	a.a_tcnp = tcnp;

	if(VNOP(fdvp, vop_rename) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(fdvp->v_op, error, &a, vop_rename);

	return (error);
}

int
vop_mkdir(dvp, vpp, cnp, vap)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct vattr *vap;
{
	struct vop_mkdir_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_mkdir);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;

	if(VNOP(dvp, vop_mkdir) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(dvp->v_op, error, &a, vop_mkdir);

	return (error);
}

int
vop_rmdir(dvp, vp, cnp)
	struct vnode *dvp;
	struct vnode *vp;
	struct componentname *cnp;
{
	struct vop_rmdir_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_rmdir);
	a.a_dvp = dvp;
	a.a_vp = vp;
	a.a_cnp = cnp;

	if(VNOP(dvp, vop_rmdir) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_rmdir);

	return (error);
}

int
vop_symlink(dvp, vpp, cnp, vap, target)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct vattr *vap;
	char *target;
{
	struct vop_symlink_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_symlink);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	a.a_target = target;

	if(VNOP(dvp, vop_symlink) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(dvp->v_op, error, &a, vop_symlink);

	return (error);
}

int
vop_readdir(vp, uio, cred, eofflag, ncookies, cookies)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
	int *eofflag;
	int *ncookies;
	u_long **cookies;
{
	struct vop_readdir_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_readdir);
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_cred = cred;
	a.a_eofflag = eofflag;
	a.a_ncookies = ncookies;
	a.a_cookies = cookies;

	if(VNOP(vp, vop_readdir) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_readdir);

	return (error);
}

int
vop_readlink(vp, uio, cred)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
{
	struct vop_readlink_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_readlink);
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_cred = cred;

	if(VNOP(vp, vop_readlink) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_readlink);

	return (error);
}

int
vop_abortop(dvp, cnp)
	struct vnode *dvp;
	struct componentname *cnp;
{
	struct vop_abortop_args a;
	int error;

	a.a_head.a_ops = dvp->v_op;
	a.a_head.a_desc = VDESC(vop_abortop);
	a.a_dvp = dvp;
	a.a_cnp = cnp;

	if(VNOP(dvp, vop_abortop) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(dvp->v_op, error, &a, vop_abortop);

	return (error);
}

int
vop_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	struct vop_inactive_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_inactive);
	a.a_vp = vp;
	a.a_p = p;

	if(VNOP(vp, vop_inactive) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_inactive);

	return (error);
}

int
vop_reclaim(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	struct vop_reclaim_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_reclaim);
	a.a_vp = vp;
	a.a_p = p;

	if(VNOP(vp, vop_reclaim) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_reclaim);

	return (error);
}

int
vop_lock(vp, flags, p)
	struct vnode *vp;
	int flags;
	struct proc *p;
{
	struct vop_lock_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_lock);
	a.a_vp = vp;
	a.a_flags = flags;
	a.a_p = p;

	if(VNOP(vp, vop_lock) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_lock);

	return (error);
}

int
vop_unlock(vp, flags, p)
	struct vnode *vp;
	int flags;
	struct proc *p;
{
	struct vop_unlock_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_unlock);
	a.a_vp = vp;
	a.a_flags = flags;
	a.a_p = p;

	if(VNOP(vp, vop_unlock) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_unlock);

	return (error);
}

int
vop_bmap(vp, bn, vpp, bnp, runp)
	struct vnode *vp;
	daddr_t bn;
	struct vnode **vpp;
	daddr_t *bnp;
	int *runp;
{
	struct vop_bmap_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_bmap);
	a.a_vp = vp;
	a.a_bn = bn;
	a.a_vpp = vpp;
	a.a_bnp = bnp;
	a.a_runp = runp;

	if(VNOP(vp, vop_bmap) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_bmap);

	return (error);
}

int
vop_print(vp)
	struct vnode *vp;
{
	struct vop_print_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_print);
	a.a_vp = vp;

	if(VNOP(vp, vop_print) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_print);

	return (error);
}

int
vop_islocked(vp)
	struct vnode *vp;
{
	struct vop_islocked_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_islocked);
	a.a_vp = vp;

	if(VNOP(vp, vop_islocked) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_islocked);

	return (error);
}

int
vop_pathconf(vp, name, retval)
	struct vnode *vp;
	int name;
	register_t *retval;
{
	struct vop_pathconf_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_pathconf);
	a.a_vp = vp;
	a.a_name = name;
	a.a_retval = retval;

	if(VNOP(vp, vop_pathconf) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_pathconf);

	return (error);
}

int
vop_advlock(vp, id, op, fl, flags)
	struct vnode *vp;
	caddr_t id;
	int op;
	struct flock *fl;
	int flags;
{
	struct vop_advlock_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_advlock);
	a.a_vp = vp;
	a.a_id = id;
	a.a_op = op;
	a.a_fl = fl;
	a.a_flags = flags;

	if(VNOP(vp, vop_advlock) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_advlock);

	return (error);
}

int
vop_blkatoff(vp, offset, res, bpp)
	struct vnode *vp;
	off_t offset;
	char **res;
	struct buf **bpp;
{
	struct vop_blkatoff_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_blkatoff);
	a.a_vp = vp;
	a.a_offset = offset;
	a.a_res = res;
	a.a_bpp = bpp;

	if(VNOP(vp, vop_blkatoff) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_blkatoff);

	return (error);
}

int
vop_valloc(pvp, mode, cred, vpp)
	struct vnode *pvp;
	int mode;
	struct ucred *cred;
	struct vnode **vpp;
{
	struct vop_valloc_args a;
	int error;

	a.a_head.a_ops = pvp->v_op;
	a.a_head.a_desc = VDESC(vop_valloc);
	a.a_pvp = pvp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_vpp = vpp;

	if(VNOP(pvp, vop_valloc) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(pvp->v_op, error, &a, vop_valloc);

	return (error);
}

int
vop_reallocblks(vp, buflist)
	struct vnode *vp;
	struct cluster_save *buflist;
{
	struct vop_reallocblks_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_reallocblks);
	a.a_vp = vp;
	a.a_buflist = buflist;

	if(VNOP(vp, vop_reallocblks) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_reallocblks);

	return (error);
}

int
vop_vfree(pvp, ino, mode)
	struct vnode *pvp;
	ino_t ino;
	int mode;
{
	struct vop_vfree_args a;
	int error;

	a.a_head.a_ops = pvp->v_op;
	a.a_head.a_desc = VDESC(vop_vfree);
	a.a_pvp = pvp;
	a.a_ino = ino;
	a.a_mode = mode;

	if(VNOP(pvp, vop_vfree) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(pvp->v_op, error, &a, vop_vfree);

	return (error);
}

int
vop_truncate(vp, length, flags, cred, p)
	struct vnode *vp;
	off_t length;
	int flags;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_truncate_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_truncate);
	a.a_vp = vp;
	a.a_length = length;
	a.a_flags = flags;
	a.a_cred = cred;
	a.a_p = p;

	if(VNOP(vp, vop_truncate) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_truncate);

	return (error);
}

int
vop_update(vp, access, modify, waitfor)
	struct vnode *vp;
	struct timeval *access;
	struct timeval *modify;
	int waitfor;
{
	struct vop_update_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_update);
	a.a_vp = vp;
	a.a_access = access;
	a.a_modify = modify;
	a.a_waitfor = waitfor;

	if(VNOP(vp, vop_update) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_update);

	return (error);
}

int
vop_getpages(vp, offset, m, count, centeridx, access_type, advice, flags)
	struct vnode *vp;
	off_t offset;
	struct vm_page **m;
	int *count;
	int centeridx;
	vm_prot_t access_type;
	int advice;
	int flags;
{
	struct vop_getpages_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_getpages);
	a.a_vp = vp;
	a.a_offset = offset;
	a.a_m = m;
	a.a_count = count;
	a.a_centeridx = centeridx;
	a.a_access_type = access_type;
	a.a_flags = flags;

	if(VNOP(vp, vop_getpages) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_getpages);

	return (error);
}

int
vop_putpages(vp, offlo, offhi, flags)
	struct vnode *vp;
	off_t offlo;
	off_t offhi;
	int flags;
{
	struct vop_putpages_args a;
	int error;

	a.a_head.a_ops = vp->v_op;
	a.a_head.a_desc = VDESC(vop_putpages);
	a.a_vp = vp;
	a.a_offlo = offlo;
	a.a_offhi = offhi;
	a.a_flags = flags;

	if(VNOP(vp, vop_putpages) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(vp->v_op, error, &a, vop_putpages);

	return (error);
}

/* Special cases: */

int
vop_strategy(bp)
	struct buf *bp;
{
	struct vop_strategy_args a;
	int error;

	a.a_head.a_ops = bp->b_vp->v_op;
	a.a_head.a_desc = VDESC(vop_strategy);
	a.a_bp = bp;

	if(VNOP(bp->b_vp, vop_strategy) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(bp->b_vp->v_op, error, &a, vop_strategy);

	return (error);
}

int
vop_bwrite(bp)
	struct buf *bp;
{
	struct vop_bwrite_args a;
	int error;

	a.a_head.a_ops = bp->b_vp->v_op;
	a.a_head.a_desc = VDESC(vop_bwrite);
	a.a_bp = bp;

	if(VNOP(bp->b_vp, vop_bwrite) == NULL) {
		return (EOPNOTSUPP);
	}

	DO_OPS(bp->b_vp->v_op, error, &a, vop_bwrite);

	return (error);
}

/* End of special cases. */

/*
 * non/standard vnodeops
 */
/*
 * vnodeop kqfilter methods
 */
static int	filt_vfsread(struct knote *kn, long hint);
static int	filt_vfswrite(struct knote *kn, long hint);
static int	filt_vfsvnode(struct knote *kn, long hint);
static void	filt_vfsdetach(struct knote *kn);

static struct filterops vfsread_filtops = {
		.f_isfd = 1,
		.f_detach = filt_vfsdetach,
		.f_event = filt_vfsread
};
static struct filterops vfswrite_filtops = {
		.f_isfd = 1,
		.f_detach = filt_vfsdetach,
		.f_event = filt_vfswrite
};
static struct filterops vfsvnode_filtops = {
		.f_isfd = 1,
		.f_detach = filt_vfsdetach,
		.f_event = filt_vfsvnode
};

int
vop_nokqfilter(ap)
	struct vop_kqfilter_args *ap;
{
	struct knote *kn = ap->a_kn;
	struct vnode *vp = ap->a_vp;
	struct klist *klist;

	KASSERT(vp->v_type != VFIFO || (kn->kn_filter != EVFILT_READ &&
		    kn->kn_filter != EVFILT_WRITE),
		    ("READ/WRITE filter on a FIFO leaked through"));
	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &vfsread_filtops;
		break;
	case EVFILT_WRITE:
		kn->kn_fop = &vfswrite_filtops;
		break;
	case EVFILT_VNODE:
		kn->kn_fop = &vfsvnode_filtops;
		break;
	default:
		return (EINVAL);
	}

	kn->kn_hook = (caddr_t)vp;

	klist = &vp->v_sel.si_klist;
	VHOLD(vp);
	SIMPLEQ_INSERT_HEAD(klist, kn, kn_selnext);

	return (0);
}

/*
 * Detach knote from vnode
 */
static void
filt_vfsdetach(kn)
	struct knote *kn;
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	KASSERT(vp->v_sel != NULL, ("Missing v_sel"));
	SIMPLEQ_REMOVE_HEAD(&vp->v_sel.si_klist, kn);
	//vdrop(vp);
}

/*ARGSUSED*/
static int
filt_vfsread(kn, hint)
	struct knote *kn;
	long hint;
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;
	struct vattr va;
	int res;

	/*
	 * filesystem is gone, so set the EOF flag and schedule
	 * the knote for deletion.
	 */
	if (hint == NOTE_REVOKE || (hint == 0 && vp->v_type == VBAD)) {
		//VI_LOCK(vp);
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
		//VI_UNLOCK(vp);
		return (1);
	}

	if (VOP_GETATTR(vp, &va, u->u_ucred, curproc))
		return (0);

	//VI_LOCK(vp);
	kn->kn_data = va.va_size - kn->kn_fp->f_offset;
	res = (kn->kn_sfflags & NOTE_FILE_POLL) != 0 || kn->kn_data != 0;
	//VI_UNLOCK(vp);
	return (res);
}

/*ARGSUSED*/
static int
filt_vfswrite(kn, hint)
	struct knote *kn;
	long hint;
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;

	//VI_LOCK(vp);

	/*
	 * filesystem is gone, so set the EOF flag and schedule
	 * the knote for deletion.
	 */
	if (hint == NOTE_REVOKE || (hint == 0 && vp->v_type == VBAD))
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);

	kn->kn_data = 0;
	//VI_UNLOCK(vp);
	return (1);
}

static int
filt_vfsvnode(kn, hint)
	struct knote *kn;
	long hint;
{
	struct vnode *vp = (struct vnode *)kn->kn_hook;
	int res;

	//VI_LOCK(vp);
	if (kn->kn_sfflags & hint)
		kn->kn_fflags |= hint;
	if (hint == NOTE_REVOKE || (hint == 0 && vp->v_type == VBAD)) {
		kn->kn_flags |= EV_EOF;
		//VI_UNLOCK(vp);
		return (1);
	}
	res = (kn->kn_fflags != 0);
	//VI_UNLOCK(vp);
	return (res);
}

/*
 * Stubs to use when there is no locking to be done on the underlying object.
 * A minimal shared lock is necessary to ensure that the underlying object
 * is not revoked while an operation is in progress. So, an active shared
 * count is maintained in an auxillary vnode lock structure.
 */
int
vop_nolock(ap)
	struct vop_lock_args *ap;
{
	/*
	 * This code cannot be used until all the non-locking filesystems
	 * (notably NFS) are converted to properly lock and release nodes.
	 * Also, certain vnode operations change the locking state within
	 * the operation (create, mknod, remove, link, rename, mkdir, rmdir,
	 * and symlink). Ideally these operations should not change the
	 * lock state, but should be changed to let the caller of the
	 * function unlock them. Otherwise all intermediate vnode layers
	 * (such as union, umapfs, etc) must catch these functions to do
	 * the necessary locking at their layer. Note that the inactive
	 * and lookup operations also change their lock state, but this
	 * cannot be avoided, so these two operations will always need
	 * to be handled in intermediate layers.
	 */
	struct vnode *vp = ap->a_vp;
	int vnflags, flags = ap->a_flags;

	if (vp->v_vnlock == NULL) {
		if ((flags & LK_TYPE_MASK) == LK_DRAIN)
			return (0);
		MALLOC(vp->v_vnlock, struct lock *, sizeof(struct lock), M_VNODE, M_WAITOK);
		lockinit(vp->v_vnlock, PVFS, "vnlock", 0, 0);
	}
	switch (flags & LK_TYPE_MASK) {
	case LK_DRAIN:
		vnflags = LK_DRAIN;
		break;
	case LK_EXCLUSIVE:
	case LK_SHARED:
		vnflags = LK_SHARED;
		break;
	case LK_UPGRADE:
	case LK_EXCLUPGRADE:
	case LK_DOWNGRADE:
		return (0);
	case LK_RELEASE:
	default:
		panic("vop_nolock: bad operation %d", flags & LK_TYPE_MASK);
	}
	if (flags & LK_INTERLOCK)
		vnflags |= LK_INTERLOCK;
	return (lockmgr(vp->v_vnlock, vnflags, &vp->v_interlock, ap->a_p->p_pid));
}

/*
 * Decrement the active use count.
 */
int
vop_nounlock(ap)
	struct vop_unlock_args *ap;
{
	struct vnode *vp = ap->a_vp;

	if (vp->v_vnlock == NULL)
		return (0);
	return (lockmgr(vp->v_vnlock, LK_RELEASE, &vp->v_interlock, ap->a_p->p_pid));
}

/*
 * Return whether or not the node is in use.
 */
int
vop_noislocked(ap)
	struct vop_islocked_args *ap;
{
	struct vnode *vp = ap->a_vp;

	if (vp->v_vnlock == NULL)
		return (0);
	return (lockstatus(vp->v_vnlock));
}

/*
 * Eliminate all activity associated with  the requested vnode
 * and with all vnodes aliased to the requested vnode.
 */
int
vop_norevoke(ap)
	struct vop_revoke_args *ap;
{
	struct vnode *vp, *vq;
	struct proc *p = curproc;	/* XXX */

#ifdef DIAGNOSTIC
	if ((ap->a_flags & REVOKEALL) == 0)
		panic("vop_revoke");
#endif

	vp = ap->a_vp;
	simple_lock(&vp->v_interlock);

	if (vp->v_flag & VALIASED) {
		/*
		 * If a vgone (or vclean) is already in progress,
		 * wait until it is done and return.
		 */
		if (vp->v_flag & VXLOCK) {
			vp->v_flag |= VXWANT;
			simple_unlock(&vp->v_interlock);
			tsleep((caddr_t)vp, PINOD, "vop_revokeall", 0);
			return (0);
		}
		/*
		 * Ensure that vp will not be vgone'd while we
		 * are eliminating its aliases.
		 */
		vp->v_flag |= VXLOCK;
		simple_unlock(&vp->v_interlock);
		while (vp->v_flag & VALIASED) {
			simple_lock(&spechash_slock);
			for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
				if (vq->v_rdev != vp->v_rdev ||
				    vq->v_type != vp->v_type || vp == vq)
					continue;
				simple_unlock(&spechash_slock);
				vgone(vq);
				break;
			}
			if (vq == NULLVP) {
				simple_unlock(&spechash_slock);
			}
		}
		/*
		 * Remove the lock so that vgone below will
		 * really eliminate the vnode after which time
		 * vgone will awaken any sleepers.
		 */
		simple_lock(&vp->v_interlock);
		vp->v_flag &= ~VXLOCK;
	}
	vgonel(vp, p);
	return (0);
}
