/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)fifo.h	8.6 (Berkeley) 5/21/95
 */

//#ifdef FIFO
/*
 * Prototypes for fifo operations on vnodes.
 */
int	fifo_badop(void *);
int 	fifo_ebadf(void);
int	fifo_printinfo(struct vnode *);
int	fifo_lookup(struct vop_lookup_args *);
int	fifo_open(struct vop_open_args *);
int	fifo_close(struct vop_close_args *);
int	fifo_read(struct vop_read_args *);
int	fifo_write(struct vop_write_args *);
int	fifo_ioctl(struct vop_ioctl_args *);
int	fifo_select(struct vop_select_args *);
int	fifo_poll(struct vop_poll_args *);
int	fifo_inactive(struct  vop_inactive_args *);
int	fifo_bmap(struct vop_bmap_args *);
int	fifo_print(struct vop_print_args *);
int	fifo_pathconf(struct vop_pathconf_args *);
int	fifo_advlock(struct vop_advlock_args *);

#define fifo_create 		((int (*) (struct  vop_create_args *))fifo_badop)
#define fifo_mknod 			((int (*) (struct  vop_mknod_args *))fifo_badop)
#define fifo_access 		((int (*) (struct  vop_access_args *))fifo_ebadf)
#define fifo_getattr 		((int (*) (struct  vop_getattr_args *))fifo_ebadf)
#define fifo_setattr 		((int (*) (struct  vop_setattr_args *))fifo_ebadf)
#define fifo_lease_check 	((int (*) (struct  vop_lease_args *))nullop)
#define	fifo_revoke 		((int (*) (struct  vop_revoke_args *))vop_norevoke)
#define fifo_mmap 			((int (*) (struct  vop_mmap_args *))fifo_badop)
#define fifo_fsync 			((int (*) (struct  vop_fsync_args *))nullop)
#define fifo_seek 			((int (*) (struct  vop_seek_args *))fifo_badop)
#define fifo_remove 		((int (*) (struct  vop_remove_args *))fifo_badop)
#define fifo_link 			((int (*) (struct  vop_link_args *))fifo_badop)
#define fifo_rename 		((int (*) (struct  vop_rename_args *))fifo_badop)
#define fifo_mkdir 			((int (*) (struct  vop_mkdir_args *))fifo_badop)
#define fifo_rmdir 			((int (*) (struct  vop_rmdir_args *))fifo_badop)
#define fifo_symlink 		((int (*) (struct  vop_symlink_args *))fifo_badop)
#define fifo_readdir 		((int (*) (struct  vop_readdir_args *))fifo_badop)
#define fifo_readlink 		((int (*) (struct  vop_readlink_args *))fifo_badop)
#define fifo_abortop 		((int (*) (struct  vop_abortop_args *))fifo_badop)
#define fifo_reclaim 		((int (*) (struct  vop_reclaim_args *))nullop)
#define fifo_lock 			((int (*) (struct  vop_lock_args *))vop_nolock)
#define fifo_unlock 		((int (*) (struct  vop_unlock_args *))vop_nounlock)
#define fifo_strategy 		((int (*) (struct  vop_strategy_args *))fifo_badop)
#define fifo_islocked 		((int (*) (struct  vop_islocked_args *))vop_noislocked)
#define fifo_blkatoff 		((int (*) (struct  vop_blkatoff_args *))fifo_badop)
#define fifo_valloc 		((int (*) (struct  vop_valloc_args *))fifo_badop)
#define fifo_reallocblks	((int (*) (struct  vop_reallocblks_args *))fifo_badop)
#define fifo_vfree 			((int (*) (struct  vop_vfree_args *))fifo_badop)
#define fifo_truncate 		((int (*) (struct  vop_truncate_args *))nullop)
#define fifo_update 		((int (*) (struct  vop_update_args *))nullop)
#define fifo_bwrite 		((int (*) (struct  vop_bwrite_args *))nullop)
//#endif /* FIFO */
