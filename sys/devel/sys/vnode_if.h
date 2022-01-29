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

#ifndef _SYS_VNODE_IF_H_
#define _SYS_VNODE_IF_H_

/* TODO:
 * - add: default vnodeop_desc
 * - re-add: vp_offsets to each vnodeop_desc
 * - re-design: vnodeopv_entry_desc to register each vnodeops operation (int (*opve_op)(void *))
 *  	with its vnodeop_desc
 * - change: vnodeopv_desc to work with vnodeopv_entry_desc changes
 *
 */
//#include <sys/vnode.h>

/*
 * This structure describes the vnode operation taking place.
 */
struct vnodeop_desc {
	int							vdesc_offset;
	char    					*vdesc_name;
	int							vdesc_flags;				/* VDESC_* flags */

	/*
	 * These ops are used by bypass routines to map and locate arguments.
	 * Creds and procs are not needed in bypass routines, but sometimes
	 * they are useful to (for example) transport layers.
	 * Nameidata is useful because it has a cred in it.
	 */
	int							*vdesc_vp_offsets;			/* list ended by VDESC_NO_OFFSET */
	int							vdesc_vpp_offset;			/* return vpp location */
	int							vdesc_cred_offset;			/* cred location, if any */
	int							vdesc_proc_offset;			/* proc location, if any */
	int							vdesc_componentname_offset; /* if any */
};

typedef int (*opve_impl_t)(void *);

union vnodeopv_entry_desc {
	struct vnodeops				*opve_vops;			/* vnode operations */
	struct vnodeop_desc 		*opve_op;  			/* which operation this is */
	opve_impl_t					opve_impl;
};

struct vnodeopv_desc_list;
LIST_HEAD(vnodeopv_desc_list, vnodeopv_desc);
struct vnodeopv_desc {
	LIST_ENTRY(vnodeopv_desc)	opv_entry;
    const char                  *opv_fsname;
    int                         opv_voptype;
    union vnodeopv_entry_desc 	opv_desc_ops;   		/* null terminated list */
};

/* vnodeops voptype */
#define D_NOOPS  	0   /* vops not set */
#define D_VNODEOPS  1   /* vops vnodeops */
#define D_SPECOPS   2   /* vops specops */
#define D_FIFOOPS   3   /* vops fifoops */

void 						vnodeopv_desc_create(struct vnodeopv_desc *, const char *, int, struct vnodeops *, struct vnodeop_desc *);
struct vnodeopv_desc 		*vnodeopv_desc_lookup(struct vnodeopv_desc *, const char *, int);
union vnodeopv_entry_desc	vnodeopv_entry_desc(struct vnodeopv_desc *, const char *, int);
struct vnodeops 			*vnodeopv_entry_desc_get_vnodeops(struct vnodeopv_desc *, const char *, int);
struct vnodeop_desc 		*vnodeopv_entry_desc_get_vnodeop_desc(struct vnodeopv_desc *, const char *, int);

#define VNODEOPV_DESC_NAME(name, voptype)         name##_##voptype##_opv_desc
#define VNODEOPV_DESC_STRUCT(name, voptype) \
		struct vnodeopv_desc VNODEOPV_DESC_NAME(name, voptype);

extern struct vnodeopv_desc_list vfs_opv_descs;

extern struct vnodeop_desc vop_lookup_desc;
extern struct vnodeop_desc vop_create_desc;
extern struct vnodeop_desc vop_whiteout_desc;
extern struct vnodeop_desc vop_mknod_desc;
extern struct vnodeop_desc vop_open_desc;
extern struct vnodeop_desc vop_close_desc;
extern struct vnodeop_desc vop_access_desc;
extern struct vnodeop_desc vop_getattr_desc;
extern struct vnodeop_desc vop_setattr_desc;
extern struct vnodeop_desc vop_read_desc;
extern struct vnodeop_desc vop_write_desc;
extern struct vnodeop_desc vop_lease_desc;
extern struct vnodeop_desc vop_ioctl_desc;
extern struct vnodeop_desc vop_select_desc;
extern struct vnodeop_desc vop_poll_desc;
extern struct vnodeop_desc vop_kqfilter_desc;
extern struct vnodeop_desc vop_revoke_desc;
extern struct vnodeop_desc vop_mmap_desc;
extern struct vnodeop_desc vop_fsync_desc;
extern struct vnodeop_desc vop_seek_desc;
extern struct vnodeop_desc vop_remove_desc;
extern struct vnodeop_desc vop_link_desc;
extern struct vnodeop_desc vop_rename_desc;
extern struct vnodeop_desc vop_mkdir_desc;
extern struct vnodeop_desc vop_rmdir_desc;
extern struct vnodeop_desc vop_symlink_desc;
extern struct vnodeop_desc vop_readdir_desc;
extern struct vnodeop_desc vop_readlink_desc;
extern struct vnodeop_desc vop_aborttop_desc;
extern struct vnodeop_desc vop_inactive_desc;
extern struct vnodeop_desc vop_reclaim_desc;
extern struct vnodeop_desc vop_lock_desc;
extern struct vnodeop_desc vop_unlock_desc;
extern struct vnodeop_desc vop_bmap_desc;
extern struct vnodeop_desc vop_print_desc;
extern struct vnodeop_desc vop_islocked_desc;
extern struct vnodeop_desc vop_pathconf_desc;
extern struct vnodeop_desc vop_advlock_desc;
extern struct vnodeop_desc vop_blkatoff_desc;
extern struct vnodeop_desc vop_valloc_desc;
extern struct vnodeop_desc vop_reallocblks_desc;
extern struct vnodeop_desc vop_vfree_desc;
extern struct vnodeop_desc vop_truncate_desc;
extern struct vnodeop_desc vop_update_desc;
extern struct vnodeop_desc vop_strategy_desc;
extern struct vnodeop_desc vop_bwrite_desc;

#include <sys/stddef.h>
/* 4.4BSD-Lite2 */
/*
 * Flags for vdesc_flags:
 */
#define	VDESC_MAX_VPS		16
/* Low order 16 flag bits are reserved for willrele flags for vp arguments. */
#define VDESC_VP0_WILLRELE	0x0001
#define VDESC_VP1_WILLRELE	0x0002
#define VDESC_VP2_WILLRELE	0x0004
#define VDESC_VP3_WILLRELE	0x0008
#define VDESC_NOMAP_VPP		0x0100
#define VDESC_VPP_WILLRELE	0x0200

/*
 * VDESC_NO_OFFSET is used to identify the end of the offset list
 * and in places where no such field exists.
 */
#define VDESC_NO_OFFSET 	-1

#define VOPARG_OFFSET(p_type, field) 				\
	((int) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))

#define VOPARG_OFFSETOF(s_type, field) 				VOPARG_OFFSET(s_type*, field)
//#define	VOPARG_OFFSETOF(s_type, field)	            offsetof((s_type)*, field)
#define VOPARG_OFFSETTO(S_TYPE, S_OFFSET, STRUCT_P) ((S_TYPE)(((char*)(STRUCT_P))+(S_OFFSET)))

#define VOCALL(OPSV, OFF, AP) 		((*((OPSV)[(OFF)]))(AP))
#define VCALL(VP, OFF, AP) 			VOCALL((VP)->v_op,(OFF),(AP))
#define VCALL1(ERR, VP, OFF, AP) 	((ERR) = VOCALL((VP)->v_op,(OFF),(AP))) /* includes error */
#define VDESC(OP) 					(&(OP##_desc))
#define VOFFSET(OP) 				(VDESC(OP)->vdesc_offset)

struct vnodeop_desc vop_lookup_desc = {
		0,
		"vop_lookup",
		0,
		{
				VOPARG_OFFSETOF(struct vop_lookup_args, a_dvp),
				VDESC_NO_OFFSET
		},
		VOPARG_OFFSETOF(struct vop_lookup_args, a_vpp),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_lookup_args, a_cnp),
		NULL,
};

struct vnodeop_desc vop_create_desc = {
		0,
		"vop_create",
		0 | VDESC_VP0_WILLRELE,
		{
				VOPARG_OFFSETOF(struct vop_create_args, a_dvp),
				VDESC_NO_OFFSET
		},
		VOPARG_OFFSETOF(struct vop_create_args, a_vpp),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_create_args, a_cnp),
		NULL,
};

struct vnodeop_desc vop_whiteout_desc = {
		0,
		"vop_whiteout",
		0 | VDESC_VP0_WILLRELE,
		{
				VOPARG_OFFSETOF(struct vop_whiteout_args, a_dvp),
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_whiteout_args, a_cnp),
		NULL,
};

struct vnodeop_desc vop_mknod_desc = {
		0,
		"vop_mknod",
		0 | VDESC_VP0_WILLRELE | VDESC_VPP_WILLRELE,
		{
				VOPARG_OFFSETOF(struct vop_mknod_args, a_dvp),
				VDESC_NO_OFFSET
		},
		VOPARG_OFFSETOF(struct vop_mknod_args, a_vpp),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_mknod_args, a_cnp),
		NULL,
};

struct vnodeop_desc vop_open_desc = {
		0,
		"vop_open",
		0,
		{
				VOPARG_OFFSETOF(struct vop_open_args, a_vp),
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_open_args, a_cred),
		VOPARG_OFFSETOF(struct vop_open_args, a_p),
		VDESC_NO_OFFSET,
		NULL,
};

struct vnodeop_desc vop_close_desc = {
		0,
		"vop_close",
		0,
		{
				VOPARG_OFFSETOF(struct vop_close_args, a_vp),
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_close_args, a_cred),
		VOPARG_OFFSETOF(struct vop_close_args, a_p),
		VDESC_NO_OFFSET,
		NULL,
};

struct vnodeop_desc vop_access_desc = {
		0,
		"vop_access",
		0,
		{
				VOPARG_OFFSETOF(struct vop_access_args,a_vp),
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_access_args, a_cred),
		VOPARG_OFFSETOF(struct vop_access_args, a_p),
		VDESC_NO_OFFSET,
		NULL,
};

struct vnodeop_desc vop_getattr_desc = {
		0,
		"vop_getattr",
		0,
		{
				VOPARG_OFFSETOF(struct vop_getattr_args, a_vp),
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_getattr_args, a_cred),
		VOPARG_OFFSETOF(struct vop_getattr_args, a_p),
		VDESC_NO_OFFSET,
		NULL,
};

struct vnodeop_desc vop_setattr_desc = {
		0,
		"vop_setattr",
		0,
		{
			VOPARG_OFFSETOF(struct vop_setattr_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_setattr_args, a_cred),
		VOPARG_OFFSETOF(struct vop_setattr_args, a_p),
		VDESC_NO_OFFSET,
		NULL,
};

struct vnodeop_desc vop_read_desc = {
		0,
		"vop_read",
		0,
		{
			VOPARG_OFFSETOF(struct vop_read_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_read_args, a_cred),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_write_desc = {
		0,
		"vop_write",
		0,
		{
			VOPARG_OFFSETOF(struct vop_write_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_write_args, a_cred),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_lease_desc = {
		0,
		"vop_lease",
		0,
		{
			VOPARG_OFFSETOF(struct vop_lease_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_lease_args, a_cred),
		VOPARG_OFFSETOF(struct vop_lease_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_ioctl_desc = {
		0,
		"vop_ioctl",
		0,
		{
			VOPARG_OFFSETOF(struct vop_ioctl_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_ioctl_args, a_cred),
		VOPARG_OFFSETOF(struct vop_ioctl_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_select_desc = {
		0,
		"vop_select",
		0,
		{
			VOPARG_OFFSETOF(struct vop_select_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_select_args, a_cred),
		VOPARG_OFFSETOF(struct vop_select_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_poll_desc = {
		0,
		"vop_poll",
		0,
		{
				VOPARG_OFFSETOF(struct vop_poll_args, a_vp),
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_kqfilter_desc = {
		0,
		"vop_kqfilter",
		0,
		{
			VOPARG_OFFSETOF(struct vop_kqfilter_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_revoke_desc = {
		0,
		"vop_revoke",
		0,
		{
			VOPARG_OFFSETOF(struct vop_revoke_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_mmap_desc = {
		0,
		"vop_mmap",
		0,
		{
			VOPARG_OFFSETOF(struct vop_mmap_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_mmap_args, a_cred),
		VOPARG_OFFSETOF(struct vop_mmap_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_fsync_desc = {
		0,
		"vop_fsync",
		0,
		{
			VOPARG_OFFSETOF(struct vop_fsync_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_fsync_args, a_cred),
		VOPARG_OFFSETOF(struct vop_fsync_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_seek_desc = {
		0,
		"vop_seek",
		0,
		{
			VOPARG_OFFSETOF(struct vop_seek_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_seek_args, a_cred),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_remove_desc = {
		0,
		"vop_remove",
		0 | VDESC_VP0_WILLRELE | VDESC_VP1_WILLRELE,
		{
			VOPARG_OFFSETOF(struct vop_remove_args, a_dvp),
			VOPARG_OFFSETOF(struct vop_remove_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_remove_args, a_cnp),
};

struct vnodeop_desc vop_link_desc = {
		0,
		"vop_link",
		0 | VDESC_VP0_WILLRELE,
		{
			VOPARG_OFFSETOF(struct vop_link_args, a_vp),
			VOPARG_OFFSETOF(struct vop_link_args, a_tdvp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_link_args, a_cnp),
};

struct vnodeop_desc vop_rename_desc = {
		0,
		"vop_rename",
		0 | VDESC_VP0_WILLRELE | VDESC_VP1_WILLRELE | VDESC_VP2_WILLRELE | VDESC_VP3_WILLRELE,
		{
			VOPARG_OFFSETOF(struct vop_rename_args, a_fdvp),
			VOPARG_OFFSETOF(struct vop_rename_args, a_fvp),
			VOPARG_OFFSETOF(struct vop_rename_args, a_tdvp),
			VOPARG_OFFSETOF(struct vop_rename_args, a_tvp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_rename_args, a_fcnp),
};

struct vnodeop_desc vop_mkdir_desc = {
		0,
		"vop_mkdir",
		0 | VDESC_VP0_WILLRELE,
		{
			VOPARG_OFFSETOF(struct vop_mkdir_args, a_dvp),
			VDESC_NO_OFFSET
		},
		VOPARG_OFFSETOF(struct vop_mkdir_args, a_vpp),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_mkdir_args, a_cnp),
};

struct vnodeop_desc vop_rmdir_desc = {
		0,
		"vop_rmdir",
		0 | VDESC_VP0_WILLRELE | VDESC_VP1_WILLRELE,
		{
			VOPARG_OFFSETOF(struct vop_rmdir_args, a_dvp),
			VOPARG_OFFSETOF(struct vop_rmdir_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_rmdir_args, a_cnp),
};

struct vnodeop_desc vop_symlink_desc = {
		0,
		"vop_symlink",
		0 | VDESC_VP0_WILLRELE | VDESC_VPP_WILLRELE,
		{
			VOPARG_OFFSETOF(struct vop_symlink_args, a_dvp),
			VDESC_NO_OFFSET
		},
		VOPARG_OFFSETOF(struct vop_symlink_args, a_vpp),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_symlink_args, a_cnp),
};

struct vnodeop_desc vop_readdir_desc = {
		0,
		"vop_readdir",
		0,
		{
			VOPARG_OFFSETOF(struct vop_readdir_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_readdir_args, a_cred),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_readlink_desc = {
		0,
		"vop_readlink",
		0,
		{
			VOPARG_OFFSETOF(struct vop_readlink_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_readlink_args, a_cred),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_abortop_desc = {
		0,
		"vop_abortop",
		0,
		{
			VOPARG_OFFSETOF(struct vop_abortop_args, a_dvp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_abortop_args, a_cnp),
};

struct vnodeop_desc vop_inactive_desc = {
		0,
		"vop_inactive",
		0,
		{
			VOPARG_OFFSETOF(struct vop_inactive_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_inactive_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_reclaim_desc = {
		0,
		"vop_reclaim",
		0,
		{
			VOPARG_OFFSETOF(struct vop_reclaim_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_reclaim_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_lock_desc = {
		0,
		"vop_lock",
		0,
		{
			VOPARG_OFFSETOF(struct vop_lock_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_lock_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_unlock_desc = {
		0,
		"vop_unlock",
		0,
		{
			VOPARG_OFFSETOF(struct vop_unlock_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_unlock_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_bmap_desc = {
		0,
		"vop_bmap",
		0,
		{
			VOPARG_OFFSETOF(struct vop_bmap_args, a_vp),
			VDESC_NO_OFFSET
		},
		VOPARG_OFFSETOF(struct vop_bmap_args, a_vpp),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_print_desc = {
		0,
		"vop_print",
		0,
		{
			VOPARG_OFFSETOF(struct vop_print_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_islocked_desc = {
		0,
		"vop_islocked",
		0,
		{
			VOPARG_OFFSETOF(struct vop_islocked_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_pathconf_desc = {
		0,
		"vop_pathconf",
		0,
		{
			VOPARG_OFFSETOF(struct vop_pathconf_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_advlock_desc = {
		0,
		"vop_advlock",
		0,
		{
			VOPARG_OFFSETOF(struct vop_advlock_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_blkatoff_desc = {
		0,
		"vop_blkatoff",
		0,
		{
			VOPARG_OFFSETOF(struct vop_blkatoff_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_valloc_desc = {
		0,
		"vop_valloc",
		0,
		{
			VOPARG_OFFSETOF(struct vop_valloc_args, a_pvp),
			VDESC_NO_OFFSET
		},
		VOPARG_OFFSETOF(struct vop_valloc_args, a_vpp),
		VOPARG_OFFSETOF(struct vop_valloc_args, a_cred),
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_reallocblks_desc = {
		0,
		"vop_reallocblks",
		0,
		{
			VOPARG_OFFSETOF(struct vop_reallocblks_args, a_vp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_vfree_desc = {
		0,
		"vop_vfree",
		0,
		{
			VOPARG_OFFSETOF(struct vop_vfree_args, a_pvp),
			VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_truncate_desc = {
		0,
		"vop_truncate",
		0,
		{
				VOPARG_OFFSETOF(struct vop_truncate_args, a_vp),
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VOPARG_OFFSETOF(struct vop_truncate_args, a_cred),
		VOPARG_OFFSETOF(struct vop_truncate_args, a_p),
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_update_desc = {
		0,
		"vop_update",
		0,
		{
				VOPARG_OFFSETOF(struct vop_update_args, a_vp),
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

/* Special cases: */
struct vnodeop_desc vop_strategy_desc = {
		0,
		"vop_strategy",
		0,
		{
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

struct vnodeop_desc vop_bwrite_desc = {
		0,
		"vop_bwrite",
		0,
		{
				VDESC_NO_OFFSET
		},
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
		VDESC_NO_OFFSET,
};

/* End of special cases. */
#endif /* _SYS_VNODE_IF_H_ */
