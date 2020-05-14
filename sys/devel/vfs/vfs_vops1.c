/*
 * vfs_vops1.c
 *
 *  Created on: 15 May 2020
 *      Author: marti
 */


#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <vfs/vops1.h>

#include <sys/user.h>
#include <sys/malloc.h>

struct vnodeops vops;
/* add M_VNODEOPS to malloc */

/* initilize vnodeops */
void
vop_init()
{
	struct vnodeops *vops = &vops; //(struct vnodeops *) malloc(sizeof(struct vnodeops *), M_VNODE, M_WAITOK);
	MALLOC(vops, struct vnodeops *, sizeof(struct vnodeops *), M_VNODEOPS, M_WAITOK);
}

int
vop_lookup(dvp, vpp, cnp)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
{
	struct vop_lookup_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;

	if(vops->vop_lookup(dvp, vpp, cnp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_lookup(dvp, vpp, cnp);

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

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;

	if(vops->vop_create(dvp, vpp, cnp, vap) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_create(dvp, vpp, cnp, vap);

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

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_cnp = cnp;
	a.a_flags = flags;

	if(vops->vop_whiteout(dvp, cnp, flags) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_whiteout(dvp, cnp, flags);

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

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;

	if(vops->vop_mknod(dvp, vpp, cnp, vap) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_mknod(dvp, vpp, cnp, vap);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_open(vp, mode, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_open(vp, mode, cred, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_fflag = fflag;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_close(vp, fflag, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_close(vp, fflag, cred, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_access(vp, mode, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_access(vp, mode, cred, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_vap = vap;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_getattr(vp, vap, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_getattr(vp, vap, cred, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_vap = vap;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_setattr(vp, vap, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_setattr(vp, vap, cred, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_ioflag = ioflag;
	a.a_cred = cred;

	if(vops->vop_read(vp, uio, ioflag, cred) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_read(vp, uio, ioflag, cred);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_ioflag = ioflag;
	a.a_cred = cred;

	if(vops->vop_write(vp, uio, ioflag, cred) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_write(vp, uio, ioflag, cred);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_p = p;
	a.a_cred = cred;
	a.a_flag = flag;

	if(vops->vop_lease(vp, p, cred, flag) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_lease(vp, p, cred, flag);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_command = command;
	a.a_data = data;
	a.a_fflag = fflag;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_ioctl(vp, command, data, fflag, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_ioctl(vp, command, data, fflag, cred, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_which = which;
	a.a_fflags = fflags;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_select(vp, which, fflags, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_select(vp, which, fflags, cred, p);

	return (error);
}

int
vop_revoke(vp, flags)
	struct vnode *vp;
	int flags;
{
	struct vop_revoke_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_flags = flags;

	if(vops->vop_revoke(vp, flags) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_revoke(vp, flags);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_fflags = fflags;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_mmap(vp, fflags, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_mmap(vp, fflags, cred, p);

	return (error);
}

int
vop_fsync(vp, cred, waitfor, p)
	struct vnode *vp;
	struct ucred *cred;
	int waitfor;
	struct proc *p;
{
	struct vop_fsync_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_cred = cred;
	a.a_waitfor = waitfor;
	a.a_p = p;

	if(vops->vop_fsync(vp, cred, waitfor, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_fsync(vp, cred, waitfor, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_oldoff = oldoff;
	a.a_newoff = newoff;
	a.a_cred = cred;

	if(vops->vop_seek(vp, oldoff, newoff, cred) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_seek(vp, oldoff, newoff, cred);

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

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_vp = vp;
	a.a_cnp = cnp;

	if(vops->vop_remove(dvp, vp, cnp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_remove(dvp, vp, cnp);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_tdvp = tdvp;
	a.a_cnp = cnp;

	if(vops->vop_link(vp, tdvp, cnp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_link(vp, tdvp, cnp);

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

	a.a_head.a_ops = &vops;
	a.a_fdvp = fdvp;
	a.a_fvp = fvp;
	a.a_fcnp = fcnp;
	a.a_tdvp = tdvp;
	a.a_tvp = tvp;
	a.a_tcnp = tcnp;

	if(vops->vop_rename(fdvp, fvp, fcnp, tdvp, tvp, tcnp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_rename(fdvp, fvp, fcnp, tdvp, tvp, tcnp);

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

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;

	if(vops->vop_mkdir(dvp, vpp, cnp, vap) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_mkdir(dvp, vpp, cnp, vap);

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

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_vp = vp;
	a.a_cnp = cnp;

	if(vops->vop_rmdir(dvp, vp, cnp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_rmdir(dvp, vp, cnp);

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

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	a.a_target = target;

	if(vops->vop_symlink(dvp, vpp, cnp, vap, target) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_symlink(dvp, vpp, cnp, vap, target);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_cred = cred;
	a.a_eofflag = eofflag;
	a.a_ncookies = ncookies;
	a.a_cookies = cookies;

	if(vops->vop_readdir(vp, uio, cred, eofflag, ncookies, cookies) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_readdir(vp, uio, cred, eofflag, ncookies, cookies);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_cred = cred;

	if(vops->vop_readlink(vp, uio, cred) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_readlink(vp, uio, cred);

	return (error);
}


int
vop_abortop(dvp, cnp)
	struct vnode *dvp;
	struct componentname *cnp;
{
	struct vop_abortop_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_dvp = dvp;
	a.a_cnp = cnp;

	if(vops->vop_abortop(dvp, cnp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_abortop(dvp, cnp);

	return (error);
}

int
vop_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	struct vop_inactive_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_p = p;

	if(vops->vop_inactive(vp, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_inactive(vp, p);

	return (error);
}

int
vop_reclaim(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	struct vop_reclaim_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_p = p;

	if(vops->vop_reclaim(vp, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_reclaim(vp, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_flags = flags;
	a.a_p = p;

	if(vops->vop_lock(vp, flags, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_lock(vp, flags, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_flags = flags;
	a.a_p = p;

	if(vops->vop_unlock(vp, flags, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_unlock(vp, flags, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_bn = bn;
	a.a_vpp = vpp;
	a.a_bnp = bnp;
	a.a_runp = runp;

	if(vops->vop_bmap(vp, bn, vpp, bnp, runp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_bmap(vp, bn, vpp, bnp, runp);

	return (error);
}

int
vop_print(vp)
	struct vnode *vp;
{
	struct vop_print_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_vp = vp;

	if(vops->vop_print(vp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_print(vp);

	return (error);
}

int
vop_islocked(vp)
	struct vnode *vp;
{
	struct vop_islocked_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_vp = vp;

	if(vops->vop_islocked(vp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_islocked(vp);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_name = name;
	a.a_retval = retval;

	if(vops->vop_pathconf(vp, name, retval) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_pathconf(vp, name, retval);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_id = id;
	a.a_op = op;
	a.a_fl = fl;
	a.a_flags = flags;

	if(vops->vop_advlock(vp, id, op, fl, flags) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_advlock(vp, id, op, fl, flags);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_offset = offset;
	a.a_res = res;
	a.a_bpp = bpp;

	if(vops->vop_blkatoff(vp, offset, res, bpp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_blkatoff(vp, offset, res, bpp);

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

	a.a_head.a_ops = &vops;
	a.a_pvp = pvp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_vpp = vpp;

	if(vops->vop_valloc(pvp, mode, cred, vpp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_valloc(pvp, mode, cred, vpp);

	return (error);
}

int
vop_reallocblks(vp, buflist)
	struct vnode *vp;
	struct cluster_save *buflist;
{
	struct vop_reallocblks_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_buflist = buflist;

	if(vops->vop_reallocblks(vp, buflist) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_reallocblks(vp, buflist);

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

	a.a_head.a_ops = &vops;
	a.a_pvp = pvp;
	a.a_ino = ino;
	a.a_mode = mode;

	if(vops->vop_vfree(pvp, ino, mode) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_vfree(pvp, ino, mode);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_length = length;
	a.a_flags = flags;
	a.a_cred = cred;
	a.a_p = p;

	if(vops->vop_truncate(vp, length, flags, cred, p) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_truncate(vp, length, flags, cred, p);

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

	a.a_head.a_ops = &vops;
	a.a_vp = vp;
	a.a_access = access;
	a.a_modify = modify;
	a.a_waitfor = waitfor;

	if(vops->vop_update(vp, access, modify, waitfor) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_update(vp, access, modify, waitfor);

	return (error);
}

/* Special cases: */
#include <sys/buf.h>

int
vop_strategy(bp)
	struct buf *bp;
{
	struct vop_strategy_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_bp = bp;

	if(vops->vop_strategy(bp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_strategy(bp);

	return (error);
}

int
vop_bwrite(bp)
	struct buf *bp;
{
	struct vop_bwrite_args a;
	int error;

	a.a_head.a_ops = &vops;
	a.a_bp = bp;

	if(vops->vop_bwrite(bp) == NULL) {
		return (EOPNOTSUPP);
	}

	error = vops->vop_bwrite(bp);

	return (error);
}

/* End of special cases. */
