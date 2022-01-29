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
 *	@(#)vfs_init.c	8.5 (Berkeley) 5/11/95
 */

/*
 * TODO: change following to work with list
 * - config/mkioconf.c: specifies vfs_list_initial[]
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <devel/sys/vnode_if.h>

struct vnodeop_desc *vfs_op_descs[] = {
		//&vop_default_desc,	/* MUST BE FIRST */
		&vop_strategy_desc,		/* XXX: SPECIAL CASE */
		&vop_bwrite_desc,		/* XXX: SPECIAL CASE */

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
};

struct vnodeopv_desc_list vfs_opv_descs;
struct vattr va_null;

int vfs_opv_numops = sizeof(vfs_op_descs) / sizeof(vfs_op_descs[0]);

void
vfs_opv_init()
{
	struct vnodeopv_desc    	*opv;
	struct vnodeop_desc 		*opv_desc;
	union vnodeopv_entry_desc 	*opve_descp;

	LIST_FOREACH(opv, &vfs_opv_descs, opv_entry) {
		opv_desc = vnodeopv_entry_desc_get_vnodeop_desc(opv, NULL, D_NOOPS);
		if(opv_desc == NULL) {
			MALLOC(*opv_desc, struct vnodeop_desc *, vfs_opv_numops * sizeof(struct vnodeop_desc *), M_VNODE, M_WAITOK);
			bzero(*opv_desc, vfs_opv_numops * sizeof(struct vnodeop_desc *));
		}

		opve_descp = vnodeopv_entry_desc(opv, NULL, D_NOOPS);

		if (opve_descp->opve_op->vdesc_offset == 0 && opve_descp->opve_op->vdesc_offset != VOFFSET(vop_default)) {
			printf("operation %s not listed in %s.\n", opve_descp->opve_op->vdesc_name, "vfs_op_descs");
			panic ("vfs_opv_init: bad operation");
		}
	}
}

void
vfs_op_init()
{
	int i;
	LIST_FOREACH(opv, &vfs_opv_descs, opv_entry) {

	}
	for (vfs_opv_numops = 0, i = 0; vfs_op_descs[i]; i++) {
		vfs_op_descs[i].vdesc_offset = vfs_opv_numops;
		vfs_opv_numops++;
	}
}

/*
 * Initialize the vnode structures and initialize each file system type.
 */
void
vfsinit()
{
	struct vfsconf *vfsp;
	int maxtypenum;

	/*
	 * Initialize the vnode table
	 */
	vntblinit();

	/*
	 * Initialize the vnode name cache
	 */
	nchinit();

	/*
	 * Initialize filesystem table
	 */
	vfsconf_fs_init();

	/*
	 * Initialize each file system type.
	 */
	vattr_null(&va_null);
	maxtypenum = 0;
	LIST_FOREACH(vfsp, &vfsconflist, vfc_next) {
		if (maxtypenum <= vfsp->vfc_typenum) {
			maxtypenum = vfsp->vfc_typenum + 1;
		}
		(*vfsp->vfc_vfsops->vfs_init)(vfsp);
	}
	/* next vfc_typenum to be used */
	maxvfsconf = maxtypenum;

	/*
	 * Initialize the vnode advisory lock vfs_lockf.c
	 */
	lf_init();
}
