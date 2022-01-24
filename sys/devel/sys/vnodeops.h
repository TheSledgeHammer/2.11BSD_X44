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

#ifndef SYS_DEVEL_SYS_VNODEOPS_H_
#define SYS_DEVEL_SYS_VNODEOPS_H_

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

#endif /* SYS_DEVEL_SYS_VNODEOPS_H_ */
