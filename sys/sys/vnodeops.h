/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed
 * to Berkeley by John Heidemann of the UCLA Ficus project.
 *
 * Source: * @(#)i405_init.c 2.10 92/04/27 UCLA Ficus project
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
 *	@(#)vnode.h	8.17 (Berkeley) 5/20/95
 */

#ifndef _SYS_VNODEOPS_H_
#define _SYS_VNODEOPS_H_

/*
 * A generic structure.
 * This can be used by bypass routines to identify generic arguments.
 */
struct vop_generic_args {
	struct vnodeops 		*a_ops;
	//struct vnodeop_desc		*a_desc;
};

struct vop_lookup_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
};

struct vop_create_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};

struct vop_whiteout_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct componentname 	*a_cnp;
	int 					a_flags;
};

struct vop_mknod_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};

struct vop_open_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_close_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_fflag;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_access_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_getattr_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct vattr 			*a_vap;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_setattr_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct vattr 			*a_vap;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_read_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	int 					a_ioflag;
	struct ucred 			*a_cred;
};

struct vop_write_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	int 					a_ioflag;
	struct ucred 			*a_cred;
};

struct vop_lease_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
	struct ucred 			*a_cred;
	int 					a_flag;
};

struct vop_ioctl_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	u_long 					a_command;
	caddr_t 				a_data;
	int 					a_fflag;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_select_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_which;
	int 					a_fflags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_poll_args {
	struct vop_generic_args a_head;
	struct vnode 			*a_vp;
	int 					a_fflags;
	int 					a_events;
	struct proc 			*a_p;
};

struct vop_kqfilter_args {
	struct vop_generic_args a_head;
	struct vnode 			*a_vp;
	struct knote			*a_kn;
};

struct vop_revoke_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_flags;
};

struct vop_mmap_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_fflags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_fsync_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct ucred 			*a_cred;
	int 					a_waitfor;
	int						a_flags;
	struct proc 			*a_p;
};

struct vop_seek_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	off_t 					a_oldoff;
	off_t 					a_newoff;
	struct ucred 			*a_cred;
};

struct vop_remove_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			*a_vp;
	struct componentname 	*a_cnp;
};

struct vop_link_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct vnode 			*a_tdvp;
	struct componentname 	*a_cnp;
};

struct vop_rename_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_fdvp;
	struct vnode 			*a_fvp;
	struct componentname 	*a_fcnp;
	struct vnode 			*a_tdvp;
	struct vnode 			*a_tvp;
	struct componentname 	*a_tcnp;
};

struct vop_mkdir_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
};

struct vop_rmdir_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			*a_vp;
	struct componentname 	*a_cnp;
};

struct vop_symlink_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct vnode 			**a_vpp;
	struct componentname 	*a_cnp;
	struct vattr 			*a_vap;
	char 					*a_target;
};

struct vop_readdir_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	struct ucred 			*a_cred;
	int 					*a_eofflag;
	int 					*a_ncookies;
	u_long 					**a_cookies;
};

struct vop_readlink_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct uio 				*a_uio;
	struct ucred 			*a_cred;
};

struct vop_abortop_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_dvp;
	struct componentname	*a_cnp;
};

struct vop_inactive_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
};

struct vop_reclaim_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct proc 			*a_p;
};

struct vop_lock_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_flags;
	struct proc 			*a_p;
};

struct vop_unlock_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_flags;
	struct proc 			*a_p;
};

struct vop_bmap_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	daddr_t 				a_bn;
	struct vnode 			**a_vpp;
	daddr_t 				*a_bnp;
	int 					*a_runp;
};

struct vop_print_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
};

struct vop_islocked_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
};

struct vop_pathconf_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	int 					a_name;
	register_t 				*a_retval;
};

struct vop_advlock_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	caddr_t 				a_id;
	int 					a_op;
	struct flock 			*a_fl;
	int 					a_flags;
};

struct vop_blkatoff_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	off_t 					a_offset;
	char 					**a_res;
	struct buf 				**a_bpp;
};

struct vop_valloc_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_pvp;
	int 					a_mode;
	struct ucred 			*a_cred;
	struct vnode 			**a_vpp;
};

struct vop_reallocblks_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct cluster_save 	*a_buflist;
};

struct vop_vfree_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_pvp;
	ino_t 					a_ino;
	int 					a_mode;
};

struct vop_truncate_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	off_t 					a_length;
	int 					a_flags;
	struct ucred 			*a_cred;
	struct proc 			*a_p;
};

struct vop_update_args {
	struct vop_generic_args	a_head;
	struct vnode 			*a_vp;
	struct timeval 			*a_access;
	struct timeval 			*a_modify;
	int 					a_waitfor;
};

/* Special cases: */
struct vop_strategy_args {
	struct vop_generic_args	a_head;
	struct buf 				*a_bp;
};

struct vop_bwrite_args {
	struct vop_generic_args	a_head;
	struct buf 				*a_bp;
};
/* End of special cases. */

struct vnodeops {
	int	(*vop_lookup)		(struct vop_lookup_args *);
	int	(*vop_create)		(struct vop_create_args *);
	int (*vop_whiteout)		(struct vop_whiteout_args *);
	int	(*vop_mknod)		(struct vop_mknod_args *);
	int	(*vop_open)	    	(struct vop_open_args *);
	int	(*vop_close)		(struct vop_close_args *);
	int	(*vop_access)		(struct vop_access_args *);
	int	(*vop_getattr)		(struct vop_getattr_args *);
	int	(*vop_setattr)		(struct vop_setattr_args *);
	int	(*vop_read)	    	(struct vop_read_args *);
	int	(*vop_write)		(struct vop_write_args *);
	int (*vop_lease)		(struct vop_lease_args *);
	int	(*vop_ioctl)		(struct vop_ioctl_args *);
	int	(*vop_select)		(struct vop_select_args *);
	int	(*vop_poll)			(struct vop_poll_args *);
	int (*vop_kqfilter)		(struct vop_kqfilter_args *);
	int (*vop_revoke)		(struct vop_revoke_args *);
	int	(*vop_mmap)	    	(struct vop_mmap_args *);
	int	(*vop_fsync)		(struct vop_fsync_args *);
	int	(*vop_seek)	    	(struct vop_seek_args *);
	int	(*vop_remove)		(struct vop_remove_args *);
	int	(*vop_link)	    	(struct vop_link_args *);
	int	(*vop_rename)		(struct vop_rename_args *);
	int	(*vop_mkdir)		(struct vop_mkdir_args *);
	int	(*vop_rmdir)		(struct vop_rmdir_args *);
	int	(*vop_symlink)		(struct vop_symlink_args *);
	int	(*vop_readdir)		(struct vop_readdir_args *);
	int	(*vop_readlink)		(struct vop_readlink_args *);
	int	(*vop_abortop)		(struct vop_abortop_args *);
	int	(*vop_inactive)		(struct vop_inactive_args *);
	int	(*vop_reclaim)		(struct vop_reclaim_args *);
	int	(*vop_lock)	    	(struct vop_lock_args *);
	int	(*vop_unlock)		(struct vop_unlock_args *);
	int	(*vop_bmap)	    	(struct vop_bmap_args *);
	int	(*vop_print)		(struct vop_print_args *);
	int	(*vop_islocked)		(struct vop_islocked_args *);
	int (*vop_pathconf)		(struct vop_pathconf_args *);
	int	(*vop_advlock)		(struct vop_advlock_args *);
	int (*vop_blkatoff)		(struct vop_blkatoff_args *);
	int (*vop_valloc)		(struct vop_valloc_args *);
	int (*vop_reallocblks)	(struct vop_reallocblks_args *);
	int (*vop_vfree)		(struct vop_vfree_args *);
	int (*vop_truncate)		(struct vop_truncate_args *);
	int (*vop_update)		(struct vop_update_args *);
	int	(*vop_strategy)		(struct vop_strategy_args *);
	int (*vop_bwrite)		(struct vop_bwrite_args *);
};

#ifdef _KERNEL

/* proto types */
int vop_badop(void *);
int	vop_ebadf(void);
int	vop_lookup(struct vnode *, struct vnode **, struct componentname *);
int	vop_create(struct vnode *, struct vnode **, struct componentname *, struct vattr *);
int vop_whiteout(struct vnode *, struct componentname *, int);
int	vop_mknod(struct vnode *, struct vnode **, struct componentname *, struct vattr *);
int	vop_open(struct vnode *, int, struct ucred *, struct proc *);
int	vop_close(struct vnode *, int, struct ucred *, struct proc *);
int	vop_access(struct vnode *, int, struct ucred *, struct proc *);
int	vop_getattr(struct vnode *, struct vattr *, struct ucred *, struct proc *);
int	vop_setattr(struct vnode *, struct vattr *, struct ucred *, struct proc *);
int	vop_read(struct vnode *, struct uio *, int, struct ucred *);
int	vop_write(struct vnode *, struct uio *, int, struct ucred *);
int vop_lease(struct vnode *, struct proc *, struct ucred *, int);
int	vop_ioctl(struct vnode *, u_long, caddr_t, int, struct ucred *, struct proc *);
int	vop_select(struct vnode *, int, int, struct ucred *, struct proc *);
int	vop_poll(struct vnode *, int, int, struct proc *);
int vop_kqfilter(struct vnode *, struct knote *);
int vop_revoke(struct vnode *, int);
int	vop_mmap(struct vnode *, int, struct ucred *, struct proc *);
int	vop_fsync(struct vnode *, int, struct ucred *, int, int, struct proc *);
int	vop_seek(struct vnode *, off_t, off_t, struct ucred *);
int	vop_remove(struct vnode *, struct vnode *, struct componentname *);
int	vop_link(struct vnode *, struct vnode *, struct componentname *);
int	vop_rename(struct vnode *, struct vnode *, struct componentname *, struct vnode *, struct vnode *, struct componentname *);
int	vop_mkdir(struct vnode *, struct vnode **, struct componentname *, struct vattr *);
int	vop_rmdir(struct vnode *, struct vnode *, struct componentname *);
int	vop_symlink(struct vnode *, struct vnode **, struct componentname *, struct vattr *, char *);
int	vop_readdir(struct vnode *, struct uio *, struct ucred *, int *, int *, u_long **);
int	vop_readlink(struct vnode *, struct uio *, struct ucred *);
int	vop_abortop(struct vnode *, struct componentname *);
int	vop_inactive(struct vnode *, struct proc *);
int	vop_reclaim(struct vnode *, struct proc *);
int	vop_lock(struct vnode *, int, struct proc *);
int	vop_unlock(struct vnode *, int, struct proc *);
int	vop_bmap(struct vnode *, daddr_t, struct vnode **, daddr_t *, int *);
int	vop_print(struct vnode *);
int	vop_islocked(struct vnode *);
int vop_pathconf(struct vnode *, int, register_t *);
int	vop_advlock(struct vnode *, caddr_t, int, struct flock *, int);
int vop_blkatoff(struct vnode *, off_t, char **, struct buf **);
int vop_valloc(struct vnode *, int, struct ucred *, struct vnode **);
int vop_reallocblks(struct vnode *, struct cluster_save *);
int vop_vfree(struct vnode *, ino_t, int);
int vop_truncate(struct vnode *, off_t, int, struct ucred *, struct proc *);
int vop_update(struct vnode *, struct timeval *, struct timeval *, int);
int	vop_strategy(struct buf *);
int vop_bwrite(struct buf *);

/* non/standard vops: used in particular cases */
int	vop_nokqfilter(struct vop_kqfilter_args *);
int	vop_noislocked(struct vop_islocked_args *);
int	vop_nolock(struct vop_lock_args *);
int	vop_nounlock(struct vop_unlock_args *);
int	vop_norevoke(struct vop_revoke_args *);
#endif /* _KERNEL */

/* Macros to call vnodeops */
#define	VOP_LOOKUP(dvp, vpp, cnp)	    						vop_lookup(dvp, vpp, cnp)
#define	VOP_CREATE(dvp, vpp, cnp, vap)	    					vop_create(dvp, vpp, cnp, vap)
#define VOP_WHITEOUT(dvp, cnp, flags)							vop_whiteout(dvp, cnp, flags)
#define	VOP_MKNOD(dvp, vpp, cnp, vap)	    					vop_mknod(dvp, vpp, cnp, vap)
#define	VOP_OPEN(vp, mode, cred, p)	    						vop_open(vp, mode, cred, p)
#define	VOP_CLOSE(vp, fflag, cred, p)	    					vop_close(vp, fflag, cred, p)
#define	VOP_ACCESS(vp, mode, cred, p)	    					vop_access(vp, mode, cred, p)
#define	VOP_GETATTR(vp, vap, cred, p)							vop_getattr(vp, vap, cred, p)
#define	VOP_SETATTR(vp, vap, cred, p)							vop_setattr(vp, vap, cred, p)
#define	VOP_READ(vp, uio, ioflag, cred)	    					vop_read(vp, uio, ioflag, cred)
#define	VOP_WRITE(vp, uio, ioflag, cred)						vop_write(vp, uio, ioflag, cred)
#define VOP_LEASE(vp, p, cred, flag)							vop_lease(vp, p, cred, flag)
#define	VOP_IOCTL(vp, command, data, fflag, cred, p)			vop_ioctl(vp, command, data, fflag, cred, p)
#define	VOP_SELECT(vp, which, fflags, cred, p)					vop_select(vp, which, fflags, cred, p)
#define VOP_POLL(vp, fflag, events, p)							vop_poll(vp, fflag, events, p)
#define VOP_KQFILTER(vp, kn)									vop_kqfilter(vp, kn)
#define VOP_REVOKE(vp, flags)									vop_revoke(vp, flags)
#define	VOP_MMAP(vp, fflags, cred, p)		    				vop_mmap(vp, fflags, cred, p)
#define	VOP_FSYNC(vp, cred, waitfor, flag, p)					vop_fsync(vp, cred, waitfor, flag, p)
#define	VOP_SEEK(vp, oldoff, newoff, cred)	    				vop_seek(vp, oldoff, newoff, cred)
#define	VOP_REMOVE(dvp, vp, cnp)		    					vop_remove(dvp, vp, cnp)
#define	VOP_LINK(vp, tdvp, cnp)		    						vop_link(vp, tdvp, cnp)
#define	VOP_RENAME(fdvp, fvp, fcnp, tdvp, tvp, tcnp)			vop_rename(fdvp, fvp, fcnp, tdvp, tvp, tcnp)
#define	VOP_MKDIR(dvp, vpp, cnp, vap)	   						vop_mkdir(dvp, vpp, cnp, vap)
#define	VOP_RMDIR(dvp, vp, cnp)		    						vop_rmdir(dvp, vp, cnp)
#define	VOP_SYMLINK(dvp, vpp, cnp, vap, target)					vop_symlink(dvp, vpp, cnp, vap, target)
#define	VOP_READDIR(vp, uio, cred, eofflag, ncookies, cookies)	vop_readdir(vp, uio, cred, eofflag, ncookies, cookies)
#define	VOP_READLINK(vp, uio, cred)	    						vop_readlink(vp, uio, cred)
#define	VOP_ABORTOP(dvp, cnp)		    						vop_abortop(dvp, cnp)
#define	VOP_INACTIVE(vp, p)	    								vop_inactive(vp, p)
#define	VOP_RECLAIM(vp, p)		    							vop_reclaim(vp, p)
#define	VOP_LOCK(vp, flags, p)		        					vop_lock(vp, flags, p)
#define	VOP_UNLOCK(vp, flags, p)		    					vop_unlock(vp, flags, p)
#define	VOP_BMAP(vp, bn, vpp, bnp, runp)	    				vop_bmap(vp, bn, vpp, bnp, runp)
#define	VOP_PRINT(vp)		    								vop_print(vp)
#define	VOP_ISLOCKED(vp)		    							vop_islocked(vp)
#define VOP_PATHCONF(vp, name, retval)							vop_pathconf(vp, name, retval)
#define	VOP_ADVLOCK(vp, id, op, fl, flags)						vop_advlock(vp, id, op, fl, flags)
#define VOP_BLKATOFF(vp, offset, res, bpp)						vop_blkatoff(vp, offset, res, bpp)
#define VOP_VALLOC(pvp, mode, cred, vpp)						vop_valloc(pvp, mode, cred, vpp)
#define VOP_REALLOCBLKS(vp, buflist)							vop_reallocblks(vp, buflist)
#define VOP_VFREE(pvp, ino, mode)								vop_vfree(pvp, ino, mode)
#define VOP_TRUNCATE(vp, length, flags, cred, p)				vop_truncate(vp, length, flags, cred, p)
#define VOP_UPDATE(vp, access, modify, waitfor)					vop_update(vp, access, modify, waitfor)
#define	VOP_STRATEGY(bp)										vop_strategy(bp)
#define VOP_BWRITE(bp)											vop_bwrite(bp)

#endif /* _SYS_VNODEOPS_H_ */
