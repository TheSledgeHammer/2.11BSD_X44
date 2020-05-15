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
 * @(#)vnode_ops.h	1.00
 */

#ifndef SYS_VNODEOPS_H_
#define SYS_VNODEOPS_H_

#include <sys/vnode.h>

/*
 * A generic structure.
 * This can be used by bypass routines to identify generic arguments.
 */
struct vop_generic_args {
	struct vnodeops 		*a_ops;
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
#include <sys/buf.h>

struct vop_strategy_args {
	struct vop_generic_args	a_head;
	struct buf 				*a_bp;
};

struct vop_bwrite_args {
	struct vop_generic_args	a_head;
	struct buf 				*a_bp;
};
/* End of special cases. */

#ifdef _KERNEL
/* vnodeops.h */
void 	vop_init();
void 	vop_alloc(struct vnodeops *);
#endif /* KERNEL */
#endif /* SYS_VNODEOPS_H_ */
