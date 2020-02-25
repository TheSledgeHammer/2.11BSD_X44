/*
 * vops.h
 *
 *  Created on: 24 Feb 2020
 *      Author: marti
 */

#ifndef SYS_TEST_VOPS_H_
#define SYS_TEST_VOPS_H_

#include <sys/vnode.h>
#ifndef _KERNEL
#include <stdbool.h>
#endif

struct buf;

extern struct vnodeop_desc *vfs_op_descs[];

struct vops {
	int (vop_lookup)(void *);
	int (vop_create)(void *);
	int (vop_whiteout)(void *);
	int (vop_mknod)(void *);
	int (vop_open)(void *);
	int (vop_close)(void *);
	int (vop_acess)(void *);
	int (vop_getattr)(void *);
	int (vop_setattr)(void *);
	int (vop_read)(void *);
	int (vop_write)(void *);
	int (vop_lease)(void *);
	int (vop_ioctl)(void *);
	int (vop_select)(void *);
	int (vop_revoke)(void *);
	int (vop_mmap)(void *);
	int (vop_fsync)(void *);
	int (vop_seek)(void *);
	int (vop_remove)(void *);
	int (vop_link)(void *);
	int (vop_rename)(void *);
	int (vop_mkdir)(void *);
	int (vop_rmdir)(void *);
	int (vop_symlink)(void *);
	int (vop_readdir)(void *);
	int (vop_readlink)(void *);
	int (vop_abortop)(void *);
	int (vop_inactive)(void *);
	int (vop_reclaim)(void *);
	int (vop_lock)(void *);
	int (vop_unlock)(void *);
	int (vop_bmap)(void *);
	int (vop_print)(void *);
	int (vop_islocked)(void *);
	int (vop_pathconf)(void *);
	int (vop_advlock)(void *);
	int (vop_blkatoff)(void *);
	int (vop_valloc)(void *);
	int (vop_reallocblks)(void *);
	int (vop_vfree)(void *);
	int (vop_truncate)(void *);
	int (vop_update)(void *);
};

/* vnodeop_desc */
extern struct vnodeop_desc vop_default_desc;	/* MUST BE FIRST */

struct vop_lookup_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
};
extern struct vnodeop_desc vop_lookup_desc;

struct vop_create_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};
extern struct vnodeop_desc vop_create_desc;

struct vop_whiteout_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct componentname 	*a_cnp;
	int 					a_flags;
};
extern struct vnodeop_desc vop_whiteout_desc;

struct vop_mknod_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};
extern struct vnodeop_desc vop_mknod_desc;

struct vop_open_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_open_desc;

struct vop_close_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_fflag;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_close_desc;

struct vop_access_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_access_desc;

struct vop_getattr_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct vattr 			*a_vap;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_getattr_desc;

struct vop_setattr_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct vattr 			*a_vap;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_setattr_desc;

struct vop_read_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	int 					a_ioflag;
	struct ucred 			*a_cred;
};
extern struct vnodeop_desc vop_read_desc;

struct vop_write_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	int 					a_ioflag;
	struct ucred 			*a_cred;
};
extern struct vnodeop_desc vop_write_desc;

struct vop_lease_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
	struct ucred 			*a_cred;
	int 					a_flag;
};
extern struct vnodeop_desc vop_lease_desc;

struct vop_ioctl_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	u_long 					a_command;
	caddr_t 				a_data;
	int 					a_fflag;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_ioctl_desc;

struct vop_select_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_which;
	int 					a_fflags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_select_desc;

struct vop_revoke_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_flags;
};
extern struct vnodeop_desc vop_revoke_desc;

struct vop_mmap_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_fflags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_mmap_desc;

struct vop_fsync_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct ucred 			*a_cred;
	int 					a_waitfor;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_fsync_desc;

struct vop_seek_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	off_t 					a_oldoff;
	off_t 					a_newoff;
	struct ucred 			*a_cred;
};
extern struct vnodeop_desc vop_seek_desc;

struct vop_remove_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct vnode 			*a_vp;
	struct componentname 	*a_cnp;
};
extern struct vnodeop_desc vop_remove_desc;

struct vop_link_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct vnode 			*a_tdvp;
	struct componentname 	*a_cnp;
};
extern struct vnodeop_desc vop_link_desc;

struct vop_rename_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_fdvp;
	struct vnode 			*a_fvp;
	struct componentname 	*a_fcnp;
	struct vnode 			*a_tdvp;
	struct vnode 			*a_tvp;
	struct componentname 	*a_tcnp;
};
extern struct vnodeop_desc vop_rename_desc;

struct vop_mkdir_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};
extern struct vnodeop_desc vop_mkdir_desc;

struct vop_rmdir_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct vnode 			*a_vp;
	struct componentname 	*a_cnp;
};
extern struct vnodeop_desc vop_rmdir_desc;

struct vop_symlink_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
	char 					*a_target;
};
extern struct vnodeop_desc vop_symlink_desc;

struct vop_readdir_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	struct ucred 			*a_cred;
	int 					*a_eofflag;
	int 					*a_ncookies;
	u_long 					**a_cookies;
};
extern struct vnodeop_desc vop_readdir_desc;

struct vop_readlink_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	struct ucred 			*a_cred;
};
extern struct vnodeop_desc vop_readlink_desc;

struct vop_abortop_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_dvp;
	struct componentname	*a_cnp;
};
extern struct vnodeop_desc vop_abortop_desc;

struct vop_inactive_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_inactive_desc;

struct vop_reclaim_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_reclaim_desc;

struct vop_lock_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_flags;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_lock_desc;

struct vop_unlock_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_flags;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_unlock_desc;

struct vop_bmap_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	daddr_t 				a_bn;
	struct vnode 			**a_vpp;
	daddr_t 				*a_bnp;
	int 					*a_runp;
};
extern struct vnodeop_desc vop_bmap_desc;

struct vop_print_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
};
extern struct vnodeop_desc vop_print_desc;

struct vop_islocked_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
};
extern struct vnodeop_desc vop_islocked_desc;

struct vop_pathconf_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	int 					a_name;
	register_t 				*a_retval;
};
extern struct vnodeop_desc vop_pathconf_desc;

struct vop_advlock_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	caddr_t 				a_id;
	int 					a_op;
	struct flock 			*a_fl;
	int 					a_flags;
};
extern struct vnodeop_desc vop_advlock_desc;

struct vop_blkatoff_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	off_t 					a_offset;
	char 					**a_res;
	struct buf 				**a_bpp;
};
extern struct vnodeop_desc vop_blkatoff_desc;

struct vop_valloc_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_pvp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct vnode 			**a_vpp;
};
extern struct vnodeop_desc vop_valloc_desc;

struct vop_reallocblks_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct cluster_save 	*a_buflist;
};
extern struct vnodeop_desc vop_reallocblks_desc;

struct vop_vfree_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_pvp;
	ino_t 					a_ino;
	int 					a_mode;
};
extern struct vnodeop_desc vop_vfree_desc;

struct vop_truncate_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	off_t 					a_length;
	int 					a_flags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};
extern struct vnodeop_desc vop_truncate_desc;

struct vop_update_args {
	struct vnodeop_desc 	*a_desc;
	struct vnode 			*a_vp;
	struct timeval 			*a_access;
	struct timeval 			*a_modify;
	int 					a_waitfor;
};
extern struct vnodeop_desc vop_update_desc;

/* Special cases: */
#include <sys/buf.h>

struct vop_strategy_args {
	struct vnodeop_desc 	*a_desc;
	struct buf 				*a_bp;
};
extern struct vnodeop_desc vop_strategy_desc;	/* XXX: SPECIAL CASE */

struct vop_bwrite_args {
	struct vnodeop_desc 	*a_desc;
	struct buf 				*a_bp;
};
extern struct vnodeop_desc vop_bwrite_desc;		/* XXX: SPECIAL CASE */

/* End of special cases. */

//#ifdef _KERNEL
static int vop_lookup(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp);
static int vop_create(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
static int vop_whiteout(struct vnode *dvp, struct componentname *cnp, int flags);
static int vop_mknod(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
static int vop_open(struct vnode *vp, int mode, struct ucred *cred, struct proc *p);
static int vop_close(struct vnode *vp, int fflag, struct ucred *cred, struct proc *p);
static int vop_acess(struct vnode *vp, int mode, struct ucred *cred, struct proc *p);
static int vop_getattr(struct vnode *vp, struct vattr *vap, struct ucred *cred, struct proc *p);
static int vop_setattr(struct vnode *vp, struct vattr *vap, struct ucred *cred, struct proc *p);
static int vop_read(struct vnode *vp, struct uio *uio, int ioflag, struct ucred *cred);
static int vop_write(struct vnode *vp, struct uio *uio, int ioflag, struct ucred *cred);
static int vop_lease(struct vnode *vp, struct proc *p, struct ucred *cred, int flag);
static int vop_ioctl(struct vnode *vp, u_long command, caddr_t data, int fflag, struct ucred *cred, struct proc *p);
static int vop_select(struct vnode *vp, int which, int fflags, struct ucred *cred, struct proc *p);
static int vop_revoke(struct vnode *vp, int flags);
static int vop_mmap(struct vnode *vp, int fflags, struct ucred *cred, struct proc *p);
static int vop_fsync(struct vnode *vp, struct ucred *cred, int waitfor, struct proc *p);
static int vop_seek(struct vnode *vp, off_t oldoff, off_t newoff, struct ucred *cred);
static int vop_remove(struct vnode *dvp, struct vnode *vp, struct componentname *cnp);
static int vop_link(struct vnode *vp, struct vnode *tdvp, struct componentname *cnp);
static int vop_rename(struct vnode *fdvp, struct vnode *fvp, struct componentname *fcnp, struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp);
static int vop_mkdir(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
static int vop_rmdir(struct vnode *dvp, struct vnode *vp, struct componentname *cnp);
static int vop_symlink(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap, char *target);
static int vop_readdir(struct vnode *vp, struct uio *uio, struct ucred *cred, int *eofflag, int *ncookies, u_long **cookies);
static int vop_readlink(struct vnode *vp, struct uio *uio, struct ucred *cred);
static int vop_abortop(struct vnode *dvp, struct componentname *cnp);
static int vop_inactive(struct vnode *vp, struct proc *p);
static int vop_reclaim(struct vnode *vp, struct proc *p);
static int vop_lock(struct vnode *vp, int flags, struct proc *p);
static int vop_unlock(struct vnode *vp, int flags, struct proc *p);
static int vop_bmap(struct vnode *vp, daddr_t bn, struct vnode **vpp, daddr_t *bnp, int *runp);
static int vop_print(struct vnode *vp);
static int vop_islocked(struct vnode *vp);
static int vop_pathconf(struct vnode *vp, int name, register_t *retval);
static int vop_advlock(struct vnode *vp, caddr_t id, int op, struct flock *fl, int flags);
static int vop_blkatoff(struct vnode *vp, off_t offset, char **res, struct buf **bpp);
static int vop_valloc(pvp, int mode, struct ucred *cred, struct vnode **vpp);
static int vop_reallocblks(struct vnode *vp, struct cluster_save *buflist);
static int vop_vfree(struct vnode *pvp, ino_t ino, int mode);
static int vop_truncate(struct vnode *vp, off_t length, int flags, struct ucred *cred, struct proc *p);
static int vop_update(struct vnode *vp, struct timeval *access, struct timeval *modify, int waitfor);
static int vop_strategy(struct buf *bp);
static int vop_bwrite(struct buf *bp);

struct vnodeop_desc create_vdesc(struct vnodeop_desc *vdesc, int vdesc_offset, char *vdesc_name, int vdesc_flags, int *vdesc_vp_offsets,
		int vdesc_vpp_offset, int vdesc_cred_offset, int vdesc_proc_offset, int vdesc_componentname_offset, caddr_t *vdesc_transports);

#define VOP_LOOKUP(dvp, vpp, cnp) \
		vop_lookup(dvp, vpp, cnp)

#define VOP_CREATE(dvp, vpp, cnp, vap) \
		vop_create(dvp, vpp, cnp, vap)

#define VOP_WHITEOUT(dvp, cnp, flags) \
		vop_whiteout(dvp, cnp, flags)

#define VOP_MKNOD(dvp, vpp, cnp, vap) \
		vop_mknod(dvp, vpp, cnp, vap)

#define VOP_OPEN(vp, mode, cred, p) \
		vop_open(vp, mode, cred, p)

#define VOP_CLOSE(vp, fflag, cred, p) \
		vop_close(vp, fflag, cred, p)

#define VOP_ACCESS(vp, mode, cred, p) \
		vop_acess(vp, mode, cred, p)

#define VOP_GETATTR(vp, vap, cred, p) \
		vop_getattr(vp, vap, cred, p)

#define VOP_SETATTR(vp, vap, cred, p) \
		vop_setattr(vp, vap, cred, p)

#define VOP_READ(vp, uio, ioflag, cred) \
		vop_read(vp, uio, ioflag, cred)

#define VOP_WRITE(vp, uio, ioflag, cred) \
		vop_write(vp, uio, ioflag, cred)

#define VOP_LEASE(vp, p, cred, flag) \
		vop_lease(vp, p, cred, flag)

#define VOP_IOCTL(vp, command, data, fflag, cred, p) \
		vop_ioctl(vp, command, data, fflag, cred, p)

#define VOP_SELECT(vp, which, fflags, cred, p) \
		vop_select(vp, which, fflags, cred, p)

#define VOP_REVOKE(vp, flags) \
		vop_revoke(vp, flags)

#define VOP_MMAP(vp, fflags, cred, p) \
		vop_mmap(vp, fflags, cred, p)

#define VOP_FSYNC(vp, cred, waitfor, p) \
		vop_fsync(vp, cred, waitfor, p)

#define VOP_SEEK(vp, oldoff, newoff, cred) \
		vop_seek(vp, oldoff, newoff, cred)

#define VOP_REMOVE(dvp, vp, cnp) \
		vop_remove(dvp, vp, cnp)

#define VOP_LINK(vp, tdvp, cnp) \
		vop_link(vp, tdvp, cnp)

#define VOP_RENAME(fdvp, fvp, fcnp, tdvp, tvp, tcnp) \
		op_rename(fdvp, fvp, fcnp, tdvp, tvp, tcnp)

#define VOP_MKDIR(dvp, vpp, cnp, vap) \
		vop_mkdir(dvp, vpp, cnp, vap)

#define VOP_RMDIR(dvp, vp, cnp) \
		vop_rmdir(dvp, vp, cnp)

#define VOP_SYMLINK(dvp, vpp, cnp, vap, target) \
		vop_symlink(dvp, vpp, cnp, vap, target)

#define VOP_READDIR(vp, uio, cred, eofflag, ncookies, cookies) \
		vop_readdir(vp, uio, cred, eofflag, ncookies, cookies)

#define VOP_READLINK(vp, uio, cred) \
		vop_readlink(vp, uio, cred)

#define VOP_ABORTOP(dvp, cnp) \
		vop_abortop(dvp, cnp)

#define VOP_INACTIVE(vp, p) \
		vop_inactive(vp, p)

#define VOP_RECLAIM(vp, p) \
		vop_reclaim(vp, p)

#define VOP_LOCK(vp, flags, p) \
		vop_lock(vp, flags, p)

#define VOP_UNLOCK(vp, flags, p) \
		vop_unlock(vp, flags, p)

#define VOP_BMAP(vp, bn, vpp, bnp, runp) \
		vop_bmap(vp, bn, vpp, bnp, runp)

#define VOP_PRINT(vp) \
		vop_print(vp)

#define VOP_ISLOCKED(vp) \
		vop_islocked(vp)

#define VOP_PATHCONF(vp, name, retval) \
		vop_pathconf(vp, name, retval)

#define VOP_ADVLOCK(vp, id, op, fl, flags) \
		vop_advlock(vp, id, op, fl, flags)

#define VOP_BLKATOFF(vp, offset, res, bpp) \
		vop_blkatoff(vp, offset, res, bpp)

#define VOP_VALLOC(pvp, mode, cred, vpp) \
		vop_valloc(pvp, mode, cred, vpp)

#define VOP_REALLOCBLKS(vp, buflist) \
		vop_reallocblks(vp, buflist)

#define VOP_VFREE(pvp, ino, mode) \
		vop_vfree(pvp, ino, mode)

#define VOP_TRUNCATE(vp, length, flags, cred, p) \
		vop_truncate(vp, length, flags, cred, p)

#define VOP_UPDATE(vp, access, modify, waitfor) \
		vop_update(vp, access, modify, waitfor)

#define VOP_STRATEGY(bp) \
		vop_strategy(bp)

#define VOP_BWRITE(bp) \
		vop_bwrite(bp)

#define CREATE_VDESC(vdesc, vdesc_offset, vdesc_name, vdesc_flags, vdesc_vp_offsets, vdesc_vpp_offset, vdesc_cred_offset, vdesc_proc_offset, vdesc_componentname_offset, vdesc_transports) \
	create_vnodeop_desc(vdesc, vdesc_offset, vdesc_name, vdesc_flags, vdesc_vp_offsets, vdesc_vpp_offset, vdesc_cred_offset, vdesc_proc_offset, vdesc_componentname_offset, vdesc_transports)

//#endif /* _KERNEL_ */

#endif /* SYS_TEST_VOPS_H_ */
