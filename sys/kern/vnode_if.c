
/*
 * Copyright (c) 1992, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
 *
 * from: NetBSD: vnode_if.sh,v 1.7 1994/08/25 03:04:28 cgd Exp $
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>

struct vnodeop_desc vop_default_desc = {
	0,
	"default",
	0,
	NULL,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_lookup_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_lookup_args, a_dvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_lookup_desc = {
	0,
	"vop_lookup",
	0,
	vop_lookup_vp_offsets,
	VOPARG_OFFSETOF(struct vop_lookup_args, a_vpp),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_lookup_args, a_cnp),
	NULL,
};

int vop_create_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_create_args, a_dvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_create_desc = {
	0,
	"vop_create",
	0 | VDESC_VP0_WILLRELE,
	vop_create_vp_offsets,
	VOPARG_OFFSETOF(struct vop_create_args, a_vpp),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_create_args, a_cnp),
	NULL,
};

int vop_whiteout_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_whiteout_args, a_dvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_whiteout_desc = {
	0,
	"vop_whiteout",
	0 | VDESC_VP0_WILLRELE,
	vop_whiteout_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_whiteout_args, a_cnp),
	NULL,
};

int vop_mknod_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_mknod_args, a_dvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_mknod_desc = {
	0,
	"vop_mknod",
	0 | VDESC_VP0_WILLRELE | VDESC_VPP_WILLRELE,
	vop_mknod_vp_offsets,
	VOPARG_OFFSETOF(struct vop_mknod_args, a_vpp),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_mknod_args, a_cnp),
	NULL,
};

int vop_open_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_open_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_open_desc = {
	0,
	"vop_open",
	0,
	vop_open_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_open_args, a_cred),
	VOPARG_OFFSETOF(struct vop_open_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_close_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_close_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_close_desc = {
	0,
	"vop_close",
	0,
	vop_close_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_close_args, a_cred),
	VOPARG_OFFSETOF(struct vop_close_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_access_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_access_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_access_desc = {
	0,
	"vop_access",
	0,
	vop_access_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_access_args, a_cred),
	VOPARG_OFFSETOF(struct vop_access_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_getattr_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_getattr_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_getattr_desc = {
	0,
	"vop_getattr",
	0,
	vop_getattr_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_getattr_args, a_cred),
	VOPARG_OFFSETOF(struct vop_getattr_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_setattr_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_setattr_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_setattr_desc = {
	0,
	"vop_setattr",
	0,
	vop_setattr_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_setattr_args, a_cred),
	VOPARG_OFFSETOF(struct vop_setattr_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_read_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_read_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_read_desc = {
	0,
	"vop_read",
	0,
	vop_read_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_read_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_write_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_write_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_write_desc = {
	0,
	"vop_write",
	0,
	vop_write_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_write_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_lease_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_lease_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_lease_desc = {
	0,
	"vop_lease",
	0,
	vop_lease_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_lease_args, a_cred),
	VOPARG_OFFSETOF(struct vop_lease_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_ioctl_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_ioctl_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_ioctl_desc = {
	0,
	"vop_ioctl",
	0,
	vop_ioctl_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_ioctl_args, a_cred),
	VOPARG_OFFSETOF(struct vop_ioctl_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_select_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_select_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_select_desc = {
	0,
	"vop_select",
	0,
	vop_select_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_select_args, a_cred),
	VOPARG_OFFSETOF(struct vop_select_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_poll_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_poll_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_poll_desc = {
	0,
	"vop_poll",
	0,
	vop_poll_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_kqfilter_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_kqfilter_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_kqfilter_desc = {
	0,
	"vop_kqfilter",
	0,
	vop_kqfilter_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_revoke_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_revoke_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_revoke_desc = {
	0,
	"vop_revoke",
	0,
	vop_revoke_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_mmap_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_mmap_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_mmap_desc = {
	0,
	"vop_mmap",
	0,
	vop_mmap_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_mmap_args, a_cred),
	VOPARG_OFFSETOF(struct vop_mmap_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_fsync_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_fsync_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_fsync_desc = {
	0,
	"vop_fsync",
	0,
	vop_fsync_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_fsync_args, a_cred),
	VOPARG_OFFSETOF(struct vop_fsync_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_seek_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_seek_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_seek_desc = {
	0,
	"vop_seek",
	0,
	vop_seek_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_seek_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_remove_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_remove_args, a_dvp),
	VOPARG_OFFSETOF(struct vop_remove_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_remove_desc = {
	0,
	"vop_remove",
	0 | VDESC_VP0_WILLRELE | VDESC_VP1_WILLRELE,
	vop_remove_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_remove_args, a_cnp),
	NULL,
};

int vop_link_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_link_args, a_vp),
	VOPARG_OFFSETOF(struct vop_link_args, a_tdvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_link_desc = {
	0,
	"vop_link",
	0 | VDESC_VP0_WILLRELE,
	vop_link_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_link_args, a_cnp),
	NULL,
};

int vop_rename_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_rename_args, a_fdvp),
	VOPARG_OFFSETOF(struct vop_rename_args, a_fvp),
	VOPARG_OFFSETOF(struct vop_rename_args, a_tdvp),
	VOPARG_OFFSETOF(struct vop_rename_args, a_tvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_rename_desc = {
	0,
	"vop_rename",
	0 | VDESC_VP0_WILLRELE | VDESC_VP1_WILLRELE | VDESC_VP2_WILLRELE | VDESC_VP3_WILLRELE,
	vop_rename_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_rename_args, a_fcnp),
	NULL,
};

int vop_mkdir_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_mkdir_args, a_dvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_mkdir_desc = {
	0,
	"vop_mkdir",
	0 | VDESC_VP0_WILLRELE,
	vop_mkdir_vp_offsets,
	VOPARG_OFFSETOF(struct vop_mkdir_args, a_vpp),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_mkdir_args, a_cnp),
	NULL,
};

int vop_rmdir_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_rmdir_args, a_dvp),
	VOPARG_OFFSETOF(struct vop_rmdir_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_rmdir_desc = {
	0,
	"vop_rmdir",
	0 | VDESC_VP0_WILLRELE | VDESC_VP1_WILLRELE,
	vop_rmdir_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_rmdir_args, a_cnp),
	NULL,
};

int vop_symlink_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_symlink_args, a_dvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_symlink_desc = {
	0,
	"vop_symlink",
	0 | VDESC_VP0_WILLRELE | VDESC_VPP_WILLRELE,
	vop_symlink_vp_offsets,
	VOPARG_OFFSETOF(struct vop_symlink_args, a_vpp),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_symlink_args, a_cnp),
	NULL,
};

int vop_readdir_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_readdir_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_readdir_desc = {
	0,
	"vop_readdir",
	0,
	vop_readdir_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_readdir_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_readlink_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_readlink_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_readlink_desc = {
	0,
	"vop_readlink",
	0,
	vop_readlink_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_readlink_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_abortop_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_abortop_args, a_dvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_abortop_desc = {
	0,
	"vop_abortop",
	0,
	vop_abortop_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_abortop_args, a_cnp),
	NULL,
};

int vop_inactive_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_inactive_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_inactive_desc = {
	0,
	"vop_inactive",
	0,
	vop_inactive_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_inactive_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_reclaim_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_reclaim_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_reclaim_desc = {
	0,
	"vop_reclaim",
	0,
	vop_reclaim_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_reclaim_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_lock_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_lock_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_lock_desc = {
	0,
	"vop_lock",
	0,
	vop_lock_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_lock_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_unlock_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_unlock_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_unlock_desc = {
	0,
	"vop_unlock",
	0,
	vop_unlock_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_unlock_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_bmap_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_bmap_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_bmap_desc = {
	0,
	"vop_bmap",
	0,
	vop_bmap_vp_offsets,
	VOPARG_OFFSETOF(struct vop_bmap_args, a_vpp),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_print_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_print_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_print_desc = {
	0,
	"vop_print",
	0,
	vop_print_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_islocked_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_islocked_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_islocked_desc = {
	0,
	"vop_islocked",
	0,
	vop_islocked_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_pathconf_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_pathconf_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_pathconf_desc = {
	0,
	"vop_pathconf",
	0,
	vop_pathconf_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_advlock_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_advlock_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_advlock_desc = {
	0,
	"vop_advlock",
	0,
	vop_advlock_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_blkatoff_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_blkatoff_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_blkatoff_desc = {
	0,
	"vop_blkatoff",
	0,
	vop_blkatoff_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_valloc_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_valloc_args, a_pvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_valloc_desc = {
	0,
	"vop_valloc",
	0,
	vop_valloc_vp_offsets,
	VOPARG_OFFSETOF(struct vop_valloc_args, a_vpp),
	VOPARG_OFFSETOF(struct vop_valloc_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_reallocblks_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_reallocblks_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_reallocblks_desc = {
	0,
	"vop_reallocblks",
	0,
	vop_reallocblks_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_vfree_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_vfree_args, a_pvp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_vfree_desc = {
	0,
	"vop_vfree",
	0,
	vop_vfree_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_truncate_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_truncate_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_truncate_desc = {
	0,
	"vop_truncate",
	0,
	vop_truncate_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_truncate_args, a_cred),
	VOPARG_OFFSETOF(struct vop_truncate_args, a_p),
	VDESC_NO_OFFSET,
	NULL,
};

int vop_update_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_update_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_update_desc = {
	0,
	"vop_update",
	0,
	vop_update_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_getpages_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_getpages_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_getpages_desc = {
	0,
	"vop_getpages",
	0,
	vop_getpages_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_putpages_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_putpages_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_putpages_desc = {
	0,
	"vop_putpages",
	0,
	vop_putpages_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_getacl_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_getacl_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_getacl_desc = {
	0,
	"vop_getacl",
	0,
	vop_getacl_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_getacl_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_setacl_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_setacl_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_setacl_desc = {
	0,
	"vop_setacl",
	0,
	vop_setacl_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_setacl_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_aclcheck_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_aclcheck_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_aclcheck_desc = {
	0,
	"vop_aclcheck",
	0,
	vop_aclcheck_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_aclcheck_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_getextattr_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_getextattr_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_getextattr_desc = {
	0,
	"vop_getextattr",
	0,
	vop_getextattr_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_getextattr_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_setextattr_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_setextattr_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_setextattr_desc = {
	0,
	"vop_setextattr",
	0,
	vop_setextattr_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_setextattr_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_deleteextattr_vp_offsets[] = {
	VOPARG_OFFSETOF(struct vop_deleteextattr_args, a_vp),
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_deleteextattr_desc = {
	0,
	"vop_deleteextattr",
	0,
	vop_deleteextattr_vp_offsets,
	VDESC_NO_OFFSET,
	VOPARG_OFFSETOF(struct vop_deleteextattr_args, a_cred),
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

/* Special cases: */

int vop_strategy_vp_offsets[] = {
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_strategy_desc = {
	0,
	"vop_strategy",
	0,
	vop_strategy_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

int vop_bwrite_vp_offsets[] = {
	VDESC_NO_OFFSET
};
struct vnodeop_desc vop_bwrite_desc = {
	0,
	"vop_bwrite",
	0,
	vop_bwrite_vp_offsets,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	VDESC_NO_OFFSET,
	NULL,
};

/* End of special cases. */

struct vnodeop_desc *vfs_op_descs[] = {
	&vop_default_desc,	/* MUST BE FIRST */
	&vop_strategy_desc,	/* XXX: SPECIAL CASE */
	&vop_bwrite_desc,	/* XXX: SPECIAL CASE */

	&vop_lookup_desc,
	&vop_create_desc,
	&vop_whiteout_desc,
	&vop_mknod_desc,
	&vop_open_desc,
	&vop_close_desc,
	&vop_access_desc,
	&vop_getattr_desc,
	&vop_setattr_desc,
	&vop_read_desc,
	&vop_write_desc,
	&vop_lease_desc,
	&vop_ioctl_desc,
	&vop_select_desc,
	&vop_poll_desc,
	&vop_kqfilter_desc,
	&vop_revoke_desc,
	&vop_mmap_desc,
	&vop_fsync_desc,
	&vop_seek_desc,
	&vop_remove_desc,
	&vop_link_desc,
	&vop_rename_desc,
	&vop_mkdir_desc,
	&vop_rmdir_desc,
	&vop_symlink_desc,
	&vop_readdir_desc,
	&vop_readlink_desc,
	&vop_abortop_desc,
	&vop_inactive_desc,
	&vop_reclaim_desc,
	&vop_lock_desc,
	&vop_unlock_desc,
	&vop_bmap_desc,
	&vop_print_desc,
	&vop_islocked_desc,
	&vop_pathconf_desc,
	&vop_advlock_desc,
	&vop_blkatoff_desc,
	&vop_valloc_desc,
	&vop_reallocblks_desc,
	&vop_vfree_desc,
	&vop_truncate_desc,
	&vop_update_desc,
	&vop_getpages_desc,
	&vop_putpages_desc,
	&vop_getacl_desc,
	&vop_setacl_desc,
	&vop_aclcheck_desc,
	&vop_getextattr_desc,
	&vop_setextattr_desc,
	&vop_deleteextattr_desc,
	NULL
};
