/*
 * vfs_vdesc.h
 *
 *  Created on: 18 Jan 2022
 *      Author: marti
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
	int							vdesc_flags;		/* VDESC_* flags */
};

struct vnodeopv_entry_desc {
	struct vnodeop_desc 		*opve_op;  			/* which operation this is */
};

struct vnodeopv_desc {
	struct vnodeopv_entry_desc 	*opv_desc_ops;   	/* null terminated list */
};

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

#define VDESCNAME(name)	 (vop_##name_desc)
#define VNODEOP_DESC_INIT(name)	 						\
	struct vnodeop_desc VDESCNAME(name) = 				\
	{ __offsetof(struct vnodeops, vop_##name), #name }

#endif /* _SYS_VFS_VDESC_H_ */
