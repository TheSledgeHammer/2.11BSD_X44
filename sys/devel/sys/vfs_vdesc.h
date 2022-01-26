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
 *	@(#)vnode.h	7.39 (Berkeley) 6/27/91
 */

#ifndef _SYS_VFS_VDESC_H_
#define _SYS_VFS_VDESC_H_

#include <sys/vnode.h>

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

union vnodeopv_entry_desc {
	struct vnodeops				**opve_vops;			/* vnode operations */
	struct vnodeop_desc 		**opve_op;  			/* which operation this is */

	int (*opve_impl)();
};

struct vnodeopv_desc_list;
LIST_HEAD(vnodeopv_desc_list, vnodeopv_desc);
struct vnodeopv_desc {
	LIST_ENTRY(vnodeopv_desc)	opv_entry;
    const char                  *opv_fsname;
    int                         opv_voptype;
    union vnodeopv_entry_desc 	opv_desc_ops;   		/* null terminated list */

    int (***opv_desc_vector_p)();
};

/* vnodeops voptype */
#define D_VNODEOPS  0   /* vops vnodeops */
#define D_SPECOPS   1   /* vops specops */
#define D_FIFOOPS   2   /* vops fifoops */

void 					vnodeopv_desc_create(struct vnodeopv_desc *, const char *, int, struct vnodeops *, struct vnodeop_desc *);
struct vnodeopv_desc 	*vnodeopv_desc_lookup(const char *, int);
struct vnodeops 		*vnodeopv_desc_get_vnodeops(const char *, int);
struct vnodeop_desc 	*vnodeopv_desc_get_vnodeop_desc(const char *, int);

#define VNODEOPV_DESC_NAME(name, voptype)         name##_##voptype##_opv_desc
#define VNODEOPV_DESC_STRUCT(name, voptype) \
		struct vnodeopv_desc VNODEOPV_DESC_NAME(name, voptype);

extern struct vnodeopv_desc_list vfs_opv_descs;


/*
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
*/

#define VDESCNAME(name)	 (vop_##name_desc)
#define VNODEOP_DESC_INIT(name)	 						\
	struct vnodeop_desc VDESCNAME(name) = 				\
	{ __offsetof(struct vnodeops, vop_##name), #name }


/* 4.4BSD-Lite2 */
/*
 * VDESC_NO_OFFSET is used to identify the end of the offset list
 * and in places where no such field exists.
 */
#define VDESC_NO_OFFSET -1

#define VOPARG_OFFSET(p_type,field) \
        ((int) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))
#define VOPARG_OFFSETOF(s_type,field) \
	VOPARG_OFFSET(s_type*,field)
#define VOPARG_OFFSETTO(S_TYPE,S_OFFSET,STRUCT_P) \
	((S_TYPE)(((char*)(STRUCT_P))+(S_OFFSET)))
#define VOCALL(OPSV,OFF,AP) ((*((OPSV)[(OFF)]))(AP))
#define VCALL(VP,OFF,AP) 	VOCALL((VP)->v_op,(OFF),(AP))
#define VDESC(OP) 			(& __CONCAT(OP, _desc))
#define VOFFSET(OP) 		(VDESC(OP)->vdesc_offset)

#define VCALL2(vp, ap) 		VOCALL((vp)->v_op, (ap)->a_head->a_desc->vdesc_offset, (ap))
/* DragonflyBSD */
typedef int (*vocall_func_t)(struct vop_generic_args *);
#define VOCALL1(vops, ap)	(*(vocall_func_t *)((char *)(vops)+((ap)->a_desc->vdesc_offset)))(ap)
#define VCALL1(vp, ap) 		VOCALL1((VP)->v_op, (AP))

#define VOCALL(OPSV,OFF,AP) ((*((OPSV)[(OFF)]))(AP))
#define VCALL(VP,OFF,AP) 	VOCALL((VP)->v_op,(OFF),(AP))
#define VDESC(OP) 			(& __CONCAT(OP, _desc))
#define VOFFSET(OP) 		(VDESC(OP)->vdesc_offset)

#endif /* _SYS_VFS_VDESC_H_ */
