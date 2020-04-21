/*
 * vfs_vops.c
 *
 *  Created on: 24 Feb 2020
 *      Author: marti
 */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include "vfs/vops.h"


static int
vop_lookup(dvp, vpp, cnp)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
{
	struct vop_lookup_args a;
	a.a_desc = &vop_lookup_desc;
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;

	return (VCALL(dvp, VOFFSET(vop_lookup), &a));
}

static int
vop_create(dvp, vpp, cnp, vap)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct vattr *vap;
{
	struct vop_create_args a;
	a.a_desc = VDESC(vop_create);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;

	return (VCALL(dvp, VOFFSET(vop_create), &a));
}

static int
vop_whiteout(dvp, cnp, flags)
	struct vnode *dvp;
	struct componentname *cnp;
	int flags;
{
	struct vop_whiteout_args a;
	a.a_desc = VDESC(vop_whiteout);
	a.a_dvp = dvp;
	a.a_cnp = cnp;
	a.a_flags = flags;
	return (VCALL(dvp, VOFFSET(vop_whiteout), &a));
}

static int
vop_mknod(dvp, vpp, cnp, vap)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct vattr *vap;
{
	struct vop_mknod_args a;
	a.a_desc = VDESC(vop_mknod);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	return (VCALL(dvp, VOFFSET(vop_mknod), &a));
}

static int
vop_open(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_open_args a;
	a.a_desc = VDESC(vop_open);
	a.a_vp = vp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_open), &a));
}

static int
vop_close(vp, fflag, cred, p)
	struct vnode *vp;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_close_args a;
	a.a_desc = VDESC(vop_close);
	a.a_vp = vp;
	a.a_fflag = fflag;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_close), &a));
}

static int
vop_access(vp, mode, cred, p)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_access_args a;
	a.a_desc = VDESC(vop_access);
	a.a_vp = vp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_access), &a));
}

static int
vop_getattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_getattr_args a;
	a.a_desc = VDESC(vop_getattr);
	a.a_vp = vp;
	a.a_vap = vap;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_getattr), &a));
}

static int
vop_setattr(vp, vap, cred, p)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_setattr_args a;
	a.a_desc = VDESC(vop_setattr);
	a.a_vp = vp;
	a.a_vap = vap;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_setattr), &a));
}

static int
vop_read(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	struct vop_read_args a;
	a.a_desc = VDESC(vop_read);
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_ioflag = ioflag;
	a.a_cred = cred;
	return (VCALL(vp, VOFFSET(vop_read), &a));
}

static int
vop_write(vp, uio, ioflag, cred)
	struct vnode *vp;
	struct uio *uio;
	int ioflag;
	struct ucred *cred;
{
	struct vop_write_args a;
	a.a_desc = VDESC(vop_write);
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_ioflag = ioflag;
	a.a_cred = cred;
	return (VCALL(vp, VOFFSET(vop_write), &a));
}

static int
vop_lease(vp, p, cred, flag)
	struct vnode *vp;
	struct proc *p;
	struct ucred *cred;
	int flag;
{
	struct vop_lease_args a;
	a.a_desc = VDESC(vop_lease);
	a.a_vp = vp;
	a.a_p = p;
	a.a_cred = cred;
	a.a_flag = flag;
	return (VCALL(vp, VOFFSET(vop_lease), &a));
}

static int
vop_ioctl(vp, command, data, fflag, cred, p)
	struct vnode *vp;
	u_long command;
	caddr_t data;
	int fflag;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_ioctl_args a;
	a.a_desc = VDESC(vop_ioctl);
	a.a_vp = vp;
	a.a_command = command;
	a.a_data = data;
	a.a_fflag = fflag;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_ioctl), &a));
}

static int
vop_select(vp, which, fflags, cred, p)
	struct vnode *vp;
	int which;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_select_args a;
	a.a_desc = VDESC(vop_select);
	a.a_vp = vp;
	a.a_which = which;
	a.a_fflags = fflags;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_select), &a));
}

static int
vop_revoke(vp, flags)
	struct vnode *vp;
	int flags;
{
	struct vop_revoke_args a;
	a.a_desc = VDESC(vop_revoke);
	a.a_vp = vp;
	a.a_flags = flags;
	return (VCALL(vp, VOFFSET(vop_revoke), &a));
}

static int
vop_mmap(vp, fflags, cred, p)
	struct vnode *vp;
	int fflags;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_mmap_args a;
	a.a_desc = VDESC(vop_mmap);
	a.a_vp = vp;
	a.a_fflags = fflags;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_mmap), &a));
}

static int
vop_fsync(vp, cred, waitfor, p)
	struct vnode *vp;
	struct ucred *cred;
	int waitfor;
	struct proc *p;
{
	struct vop_fsync_args a;
	a.a_desc = VDESC(vop_fsync);
	a.a_vp = vp;
	a.a_cred = cred;
	a.a_waitfor = waitfor;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_fsync), &a));
}

static int
vop_seek(vp, oldoff, newoff, cred)
	struct vnode *vp;
	off_t oldoff;
	off_t newoff;
	struct ucred *cred;
{
	struct vop_seek_args a;
	a.a_desc = VDESC(vop_seek);
	a.a_vp = vp;
	a.a_oldoff = oldoff;
	a.a_newoff = newoff;
	a.a_cred = cred;
	return (VCALL(vp, VOFFSET(vop_seek), &a));
}

static int
vop_remove(dvp, vp, cnp)
	struct vnode *dvp;
	struct vnode *vp;
	struct componentname *cnp;
{
	struct vop_remove_args a;
	a.a_desc = VDESC(vop_remove);
	a.a_dvp = dvp;
	a.a_vp = vp;
	a.a_cnp = cnp;
	return (VCALL(dvp, VOFFSET(vop_remove), &a));
}

static int
vop_link(vp, tdvp, cnp)
	struct vnode *vp;
	struct vnode *tdvp;
	struct componentname *cnp;
{
	struct vop_link_args a;
	a.a_desc = VDESC(vop_link);
	a.a_vp = vp;
	a.a_tdvp = tdvp;
	a.a_cnp = cnp;
	return (VCALL(vp, VOFFSET(vop_link), &a));
}

static int
vop_rename(fdvp, fvp, fcnp, tdvp, tvp, tcnp)
	struct vnode *fdvp;
	struct vnode *fvp;
	struct componentname *fcnp;
	struct vnode *tdvp;
	struct vnode *tvp;
	struct componentname *tcnp;
{
	struct vop_rename_args a;
	a.a_desc = VDESC(vop_rename);
	a.a_fdvp = fdvp;
	a.a_fvp = fvp;
	a.a_fcnp = fcnp;
	a.a_tdvp = tdvp;
	a.a_tvp = tvp;
	a.a_tcnp = tcnp;
	return (VCALL(fdvp, VOFFSET(vop_rename), &a));
}

static int
vop_mkdir(dvp, vpp, cnp, vap)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct vattr *vap;
{
	struct vop_mkdir_args a;
	a.a_desc = VDESC(vop_mkdir);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	return (VCALL(dvp, VOFFSET(vop_mkdir), &a));
}

static int
vop_rmdir(dvp, vp, cnp)
	struct vnode *dvp;
	struct vnode *vp;
	struct componentname *cnp;
{
	struct vop_rmdir_args a;
	a.a_desc = VDESC(vop_rmdir);
	a.a_dvp = dvp;
	a.a_vp = vp;
	a.a_cnp = cnp;
	return (VCALL(dvp, VOFFSET(vop_rmdir), &a));
}

static int
vop_symlink(dvp, vpp, cnp, vap, target)
	struct vnode *dvp;
	struct vnode **vpp;
	struct componentname *cnp;
	struct vattr *vap;
	char *target;
{
	struct vop_symlink_args a;
	a.a_desc = VDESC(vop_symlink);
	a.a_dvp = dvp;
	a.a_vpp = vpp;
	a.a_cnp = cnp;
	a.a_vap = vap;
	a.a_target = target;
	return (VCALL(dvp, VOFFSET(vop_symlink), &a));
}

static int
vop_readdir(vp, uio, cred, eofflag, ncookies, cookies)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
	int *eofflag;
	int *ncookies;
	u_long **cookies;
{
	struct vop_readdir_args a;
	a.a_desc = VDESC(vop_readdir);
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_cred = cred;
	a.a_eofflag = eofflag;
	a.a_ncookies = ncookies;
	a.a_cookies = cookies;
	return (VCALL(vp, VOFFSET(vop_readdir), &a));
}

static int
vop_readlink(vp, uio, cred)
	struct vnode *vp;
	struct uio *uio;
	struct ucred *cred;
{
	struct vop_readlink_args a;
	a.a_desc = VDESC(vop_readlink);
	a.a_vp = vp;
	a.a_uio = uio;
	a.a_cred = cred;
	return (VCALL(vp, VOFFSET(vop_readlink), &a));
}


static int
vop_abortop(dvp, cnp)
	struct vnode *dvp;
	struct componentname *cnp;
{
	struct vop_abortop_args a;
	a.a_desc = VDESC(vop_abortop);
	a.a_dvp = dvp;
	a.a_cnp = cnp;
	return (VCALL(dvp, VOFFSET(vop_abortop), &a));
}

static int
vop_inactive(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	struct vop_inactive_args a;
	a.a_desc = VDESC(vop_inactive);
	a.a_vp = vp;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_inactive), &a));
}

static int
vop_reclaim(vp, p)
	struct vnode *vp;
	struct proc *p;
{
	struct vop_reclaim_args a;
	a.a_desc = VDESC(vop_reclaim);
	a.a_vp = vp;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_reclaim), &a));
}

static int
vop_lock(vp, flags, p)
	struct vnode *vp;
	int flags;
	struct proc *p;
{
	struct vop_lock_args a;
	a.a_desc = VDESC(vop_lock);
	a.a_vp = vp;
	a.a_flags = flags;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_lock), &a));
}

static int
vop_unlock(vp, flags, p)
	struct vnode *vp;
	int flags;
	struct proc *p;
{
	struct vop_unlock_args a;
	a.a_desc = VDESC(vop_unlock);
	a.a_vp = vp;
	a.a_flags = flags;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_unlock), &a));
}

static int
vop_bmap(vp, bn, vpp, bnp, runp)
	struct vnode *vp;
	daddr_t bn;
	struct vnode **vpp;
	daddr_t *bnp;
	int *runp;
{
	struct vop_bmap_args a;
	a.a_desc = VDESC(vop_bmap);
	a.a_vp = vp;
	a.a_bn = bn;
	a.a_vpp = vpp;
	a.a_bnp = bnp;
	a.a_runp = runp;
	return (VCALL(vp, VOFFSET(vop_bmap), &a));
}

static int
vop_print(vp)
	struct vnode *vp;
{
	struct vop_print_args a;
	a.a_desc = VDESC(vop_print);
	a.a_vp = vp;
	return (VCALL(vp, VOFFSET(vop_print), &a));
}

static int
vop_islocked(vp)
	struct vnode *vp;
{
	struct vop_islocked_args a;
	a.a_desc = VDESC(vop_islocked);
	a.a_vp = vp;
	return (VCALL(vp, VOFFSET(vop_islocked), &a));
}

static int
vop_pathconf(vp, name, retval)
	struct vnode *vp;
	int name;
	register_t *retval;
{
	struct vop_pathconf_args a;
	a.a_desc = VDESC(vop_pathconf);
	a.a_vp = vp;
	a.a_name = name;
	a.a_retval = retval;
	return (VCALL(vp, VOFFSET(vop_pathconf), &a));
}

static int
vop_advlock(vp, id, op, fl, flags)
	struct vnode *vp;
	caddr_t id;
	int op;
	struct flock *fl;
	int flags;
{
	struct vop_advlock_args a;
	a.a_desc = VDESC(vop_advlock);
	a.a_vp = vp;
	a.a_id = id;
	a.a_op = op;
	a.a_fl = fl;
	a.a_flags = flags;
	return (VCALL(vp, VOFFSET(vop_advlock), &a));
}

static int
vop_blkatoff(vp, offset, res, bpp)
	struct vnode *vp;
	off_t offset;
	char **res;
	struct buf **bpp;
{
	struct vop_blkatoff_args a;
	a.a_desc = VDESC(vop_blkatoff);
	a.a_vp = vp;
	a.a_offset = offset;
	a.a_res = res;
	a.a_bpp = bpp;
	return (VCALL(vp, VOFFSET(vop_blkatoff), &a));
}

static int
vop_valloc(pvp, mode, cred, vpp)
	struct vnode *pvp;
	int mode;
	struct ucred *cred;
	struct vnode **vpp;
{
	struct vop_valloc_args a;
	a.a_desc = VDESC(vop_valloc);
	a.a_pvp = pvp;
	a.a_mode = mode;
	a.a_cred = cred;
	a.a_vpp = vpp;
	return (VCALL(pvp, VOFFSET(vop_valloc), &a));
}

static int
vop_reallocblks(vp, buflist)
	struct vnode *vp;
	struct cluster_save *buflist;
{
	struct vop_reallocblks_args a;
	a.a_desc = VDESC(vop_reallocblks);
	a.a_vp = vp;
	a.a_buflist = buflist;
	return (VCALL(vp, VOFFSET(vop_reallocblks), &a));
}

static int
vop_vfree(pvp, ino, mode)
	struct vnode *pvp;
	ino_t ino;
	int mode;
{
	struct vop_vfree_args a;
	a.a_desc = VDESC(vop_vfree);
	a.a_pvp = pvp;
	a.a_ino = ino;
	a.a_mode = mode;
	return (VCALL(pvp, VOFFSET(vop_vfree), &a));
}

static int
vop_truncate(vp, length, flags, cred, p)
	struct vnode *vp;
	off_t length;
	int flags;
	struct ucred *cred;
	struct proc *p;
{
	struct vop_truncate_args a;
	a.a_desc = VDESC(vop_truncate);
	a.a_vp = vp;
	a.a_length = length;
	a.a_flags = flags;
	a.a_cred = cred;
	a.a_p = p;
	return (VCALL(vp, VOFFSET(vop_truncate), &a));
}

static int
vop_update(vp, access, modify, waitfor)
	struct vnode *vp;
	struct timeval *access;
	struct timeval *modify;
	int waitfor;
{
	struct vop_update_args a;
	a.a_desc = VDESC(vop_update);
	a.a_vp = vp;
	a.a_access = access;
	a.a_modify = modify;
	a.a_waitfor = waitfor;
	return (VCALL(vp, VOFFSET(vop_update), &a));
}

/* Special cases: */
#include <sys/buf.h>

static int
vop_strategy(bp)
	struct buf *bp;
{
	struct vop_strategy_args a;
	a.a_desc = VDESC(vop_strategy);
	a.a_bp = bp;
	return (VCALL(bp->b_vp, VOFFSET(vop_strategy), &a));
}

static int
vop_bwrite(bp)
	struct buf *bp;
{
	struct vop_bwrite_args a;
	a.a_desc = VDESC(vop_bwrite);
	a.a_bp = bp;
	return (VCALL(bp->b_vp, VOFFSET(vop_bwrite), &a));
}

/* End of special cases. */

struct vnodeop_desc *
create_vdesc(vdesc, vdesc_offset, vdesc_name, vdesc_flags, vdesc_vp_offsets,
		vdesc_vpp_offset, vdesc_cred_offset, vdesc_proc_offset, vdesc_componentname_offset,
		vdesc_transports)
	struct vnodeop_desc *vdesc;
	int vdesc_offset, vdesc_flags, *vdesc_vp_offsets, vdesc_vpp_offset,
	vdesc_cred_offset, vdesc_proc_offset, vdesc_componentname_offset;
	char *vdesc_name;
	caddr_t	*vdesc_transports;
{
	vdesc->vdesc_offset = vdesc_offset;
	vdesc->vdesc_name = vdesc_name;
	vdesc->vdesc_flags = vdesc_flags;
	vdesc->vdesc_vp_offsets = vdesc_vp_offsets;
	vdesc->vdesc_vpp_offset = vdesc_vpp_offset;
	vdesc->vdesc_cred_offset = vdesc_cred_offset;
	vdesc->vdesc_proc_offset = vdesc_proc_offset;
	vdesc->vdesc_componentname_offset = vdesc_componentname_offset;
	vdesc->vdesc_transports = vdesc_transports;
	return (vdesc);
}

void
vops_desc_init()
{
    struct vnodeop_desc *vfs_op_descs[] = {
    		create_vdesc(&vop_default_desc, 0, "default", 0, NULL, VDESC_NO_OFFSET, VDESC_NO_OFFSET, VDESC_NO_OFFSET, VDESC_NO_OFFSET, NULL),
			create_vdesc(&vop_strategy_desc, 0, "vop_strategy", 0, vop_strategy_vp_offsets, VDESC_NO_OFFSET, VDESC_NO_OFFSET, VDESC_NO_OFFSET, VDESC_NO_OFFSET, NULL),
			create_vdesc(&vop_bwrite_desc, 0, "vop_bwrite", 0, vop_bwrite_vp_offsets, VDESC_NO_OFFSET, VDESC_NO_OFFSET, VDESC_NO_OFFSET, VDESC_NO_OFFSET, NULL),
			create_vdesc(&vop_lookup_desc, 0, "vop_lookup", 0, vop_lookup_vp_offsets, VOPARG_OFFSETOF(struct vop_lookup_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_lookup_args, a_cnp), NULL),
			create_vdesc(&vop_bwrite_desc, 0, "vop_create", 0 | VDESC_VP0_WILLRELE, vop_create_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_cnp), NULL),
			create_vdesc(&vop_whiteout_desc, 0, "vop_whiteout", 0 | VDESC_VP0_WILLRELE, vop_whiteout_vp_offsets, VDESC_NO_OFFSET, VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_whiteout_args, a_cnp), NULL),
			create_vdesc(&vop_mknod_desc, 0, "vop_mknod", 0 | VDESC_VP0_WILLRELE | VDESC_VPP_WILLRELE, vop_mknod_vp_offsets, VOPARG_OFFSETOF(struct vop_mknod_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_mknod_args, a_cnp), NULL),
			create_vdesc(&vop_open_desc, 0, "vop_open", 0 | VDESC_VP0_WILLRELE, vop_open_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_cnp), NULL),
			create_vdesc(&vop_close_desc, 0, "vop_close", 0 | VDESC_VP0_WILLRELE, vop_close_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_cnp), NULL),
			create_vdesc(&vop_access_desc, 0, "vop_access", 0 | VDESC_VP0_WILLRELE, vop_access_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_cnp), NULL),
			create_vdesc(&vop_getattr_desc, 0, "vop_getattr", 0 | VDESC_VP0_WILLRELE, vop_getattr_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_setattr_desc, 0, "vop_setattr", 0 | VDESC_VP0_WILLRELE, vop_setattr_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_read_desc, 0, "vop_read", 0 | VDESC_VP0_WILLRELE, vop_read_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_write_desc, 0, "vop_write", 0 | VDESC_VP0_WILLRELE, vop_write_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_lease_desc, 0, "vop_lease", 0 | VDESC_VP0_WILLRELE, vop_lease_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_ioctl_desc, 0, "vop_ioctl", 0 | VDESC_VP0_WILLRELE, vop_ioctl_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_select_desc, 0, "vop_select", 0 | VDESC_VP0_WILLRELE, vop_select_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_revoke_desc, 0, "vop_revoke", 0 | VDESC_VP0_WILLRELE, vop_revoke_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_mmap_desc, 0, "vop_mmap", 0 | VDESC_VP0_WILLRELE, vop_mmap_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_fsync_desc, 0, "vop_fsync", 0 | VDESC_VP0_WILLRELE, vop_fsync_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_seek_desc, 0, "vop_seek", 0 | VDESC_VP0_WILLRELE, vop_seek_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_remove_desc, 0, "vop_remove", 0 | VDESC_VP0_WILLRELE, vop_remove_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_link_desc, 0, "vop_link", 0 | VDESC_VP0_WILLRELE, vop_link_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_rename_desc, 0, "vop_rename", 0 | VDESC_VP0_WILLRELE, vop_rename_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_mkdir_desc, 0, "vop_mkdir", 0 | VDESC_VP0_WILLRELE, vop_mkdir_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_rmdir_desc, 0, "vop_rmdir", 0 | VDESC_VP0_WILLRELE, vop_rmdir_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_symlink_desc, 0, "vop_symlink", 0 | VDESC_VP0_WILLRELE, vop_symlink_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_readdir_desc, 0, "vop_readdir", 0 | VDESC_VP0_WILLRELE, vop_readdir_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_readlink_desc, 0, "vop_readlink", 0 | VDESC_VP0_WILLRELE, vop_readlink_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_abortop_desc, 0, "vop_abortop", 0 | VDESC_VP0_WILLRELE, vop_abortop_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_inactive_desc, 0, "vop_inactive", 0 | VDESC_VP0_WILLRELE, vop_inactive_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_reclaim_desc, 0, "vop_reclaim", 0 | VDESC_VP0_WILLRELE, vop_reclaim_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_lock_desc, 0, "vop_lock", 0 | VDESC_VP0_WILLRELE, vop_lock_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_unlock_desc, 0, "vop_unlock", 0 | VDESC_VP0_WILLRELE, vop_unlock_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_bmap_desc, 0, "vop_bmap", 0 | VDESC_VP0_WILLRELE, vop_bmap_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_print_desc, 0, "vop_print", 0 | VDESC_VP0_WILLRELE, vop_print_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
			create_vdesc(&vop_islocked_desc, 0, "vop_islocked", 0 | VDESC_VP0_WILLRELE, vop_islocked_vp_offsets, VOPARG_OFFSETOF(struct vop_create_args, a_vpp), VDESC_NO_OFFSET, VDESC_NO_OFFSET, VOPARG_OFFSETOF(struct vop_create_args, a_vnp), NULL),
    };
}
