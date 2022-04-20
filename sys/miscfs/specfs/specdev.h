/*
 * Copyright (c) 1990, 1993
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
 *	@(#)specdev.h	8.6 (Berkeley) 5/21/95
 */

/*
 * This structure defines the information maintained about
 * special devices. It is allocated in checkalias and freed
 * in vgone.
 */
struct specinfo {
	struct	vnode 	**si_hashchain;
	struct	vnode 	*si_specnext;
	long			si_flags;
	dev_t			si_rdev;
	struct lockf	*si_lockf;
};
/*
 * Exported shorthand
 */
#define v_rdev 		v_specinfo->si_rdev
#define v_hashchain v_specinfo->si_hashchain
#define v_specnext 	v_specinfo->si_specnext
#define v_specflags v_specinfo->si_flags
#define v_speclockf	v_specinfo->si_lockf

/*
 * Flags for specinfo
 */
#define	SI_MOUNTEDON	0x0001	/* block special device is mounted on */

/*
 * Special device management
 */
#define	SPECHSZ	64
#if	((SPECHSZ&(SPECHSZ-1)) == 0)
#define	SPECHASH(rdev)	(((rdev>>5)+(rdev))&(SPECHSZ-1))
#else
#define	SPECHASH(rdev)	(((unsigned)((rdev>>5)+(rdev)))%SPECHSZ)
#endif

struct vnode *speclisth[SPECHSZ];

/*
 * Prototypes for special file operations on vnodes.
 */
extern struct vnodeops spec_vnodeops;
struct	nameidata;
struct	componentname;
struct	ucred;
struct	flock;
struct	buf;
struct	uio;

int	spec_badop(void *);
int spec_ebadf(void);
int	spec_lookup(struct vop_lookup_args *);
int	spec_open(struct vop_open_args *);
int	spec_close(struct vop_close_args *);
int	spec_read(struct vop_read_args *);
int	spec_write(struct vop_write_args *);
int	spec_ioctl(struct vop_ioctl_args *);
int	spec_select(struct vop_select_args *);
int	spec_poll(struct vop_poll_args *);
int	spec_fsync(struct vop_fsync_args *);
int	spec_inactive(struct vop_inactive_args *);
int	spec_bmap(struct vop_bmap_args *);
int	spec_strategy(struct vop_strategy_args *);
int	spec_print(struct vop_print_args *);
int	spec_pathconf(struct vop_pathconf_args *);
int	spec_advlock (struct vop_advlock_args *);
#define spec_create 	 ((int (*) (struct  vop_create_args *))spec_badop)
#define spec_mknod 		 ((int (*) (struct  vop_mknod_args *))spec_badop)
#define spec_access 	 ((int (*) (struct  vop_access_args *))spec_ebadf)
#define spec_getattr	 ((int (*) (struct  vop_getattr_args *))spec_ebadf)
#define spec_setattr 	 ((int (*) (struct  vop_setattr_args *))spec_ebadf)
#define	spec_lease_check ((int (*) (struct  vop_lease_args *))nullop)
#define	spec_revoke 	 ((int (*) (struct  vop_revoke_args *))vop_norevoke)
#define spec_mmap 		 ((int (*) (struct  vop_mmap_args *))spec_badop)
#define spec_seek 		 ((int (*) (struct  vop_seek_args *))spec_badop)
#define spec_remove 	 ((int (*) (struct  vop_remove_args *))spec_badop)
#define spec_link 		 ((int (*) (struct  vop_link_args *))spec_badop)
#define spec_rename 	 ((int (*) (struct  vop_rename_args *))spec_badop)
#define spec_mkdir 		 ((int (*) (struct  vop_mkdir_args *))spec_badop)
#define spec_rmdir 		 ((int (*) (struct  vop_rmdir_args *))spec_badop)
#define spec_symlink 	 ((int (*) (struct  vop_symlink_args *))spec_badop)
#define spec_readdir 	 ((int (*) (struct  vop_readdir_args *))spec_badop)
#define spec_readlink	 ((int (*) (struct  vop_readlink_args *))spec_badop)
#define spec_abortop 	 ((int (*) (struct  vop_abortop_args *))spec_badop)
#define spec_reclaim 	 ((int (*) (struct  vop_reclaim_args *))nullop)
#define spec_lock 		 ((int (*) (struct  vop_lock_args *))vop_nolock)
#define spec_unlock 	 ((int (*) (struct  vop_unlock_args *))vop_nounlock)
#define spec_islocked 	 ((int (*) (struct  vop_islocked_args *))vop_noislocked)
#define spec_blkatoff 	 ((int (*) (struct  vop_blkatoff_args *))spec_badop)
#define spec_valloc 	 ((int (*) (struct  vop_valloc_args *))spec_badop)
#define spec_reallocblks ((int (*) (struct  vop_reallocblks_args *))spec_badop)
#define spec_vfree 		 ((int (*) (struct  vop_vfree_args *))spec_badop)
#define spec_truncate 	 ((int (*) (struct  vop_truncate_args *))nullop)
#define spec_update 	 ((int (*) (struct  vop_update_args *))nullop)
#define spec_bwrite 	 ((int (*) (struct  vop_bwrite_args *))nullop)
