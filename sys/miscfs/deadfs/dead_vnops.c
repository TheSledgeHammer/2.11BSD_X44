/*
 * Copyright (c) 1989, 1993
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
 *	@(#)dead_vnops.c	8.3 (Berkeley) 5/14/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/namei.h>
#include <sys/buf.h>
#include <sys/null.h>
#include <sys/poll.h>

/*
 * Prototypes for dead operations on vnodes.
 */
int dead_default_error(struct vop_generic_args *);
int	dead_badop(void *);
int dead_ebadf(void);
int	dead_lookup(struct vop_lookup_args *);
int	dead_open(struct vop_open_args *);
int	dead_read(struct vop_read_args *);
int	dead_write(struct vop_write_args *);
int	dead_ioctl(struct vop_ioctl_args *);
int	dead_select(struct vop_select_args *);
int	dead_poll(struct vop_poll_args *);
int	dead_lock(struct vop_lock_args *);
int	dead_bmap(struct vop_bmap_args *);
int	dead_strategy(struct vop_strategy_args *);
int	dead_print(struct vop_print_args *);
int	chkvnlock(struct vnode *);

#define dead_create 		((int (*) (struct  vop_create_args *))dead_badop)
#define dead_mknod 			((int (*) (struct  vop_mknod_args *))dead_badop)
#define dead_close 			((int (*) (struct  vop_close_args *))nullop)
#define dead_access 		((int (*) (struct  vop_access_args *))dead_ebadf)
#define dead_getattr 		((int (*) (struct  vop_getattr_args *))dead_ebadf)
#define dead_setattr 		((int (*) (struct  vop_setattr_args *))dead_ebadf)
#define dead_mmap 			((int (*) (struct  vop_mmap_args *))dead_badop)
#define	dead_lease_check 	((int (*) (struct  vop_lease_args *))nullop)
#define	dead_revoke 		((int (*) (struct  vop_revoke_args *))vop_norevoke)
#define dead_fsync 			((int (*) (struct  vop_fsync_args *))nullop)
#define dead_seek 			((int (*) (struct  vop_seek_args *))nullop)
#define dead_remove 		((int (*) (struct  vop_remove_args *))dead_badop)
#define dead_link 			((int (*) (struct  vop_link_args *))dead_badop)
#define dead_rename 		((int (*) (struct  vop_rename_args *))dead_badop)
#define dead_mkdir 			((int (*) (struct  vop_mkdir_args *))dead_badop)
#define dead_rmdir 			((int (*) (struct  vop_rmdir_args *))dead_badop)
#define dead_symlink 		((int (*) (struct  vop_symlink_args *))dead_badop)
#define dead_readdir 		((int (*) (struct  vop_readdir_args *))dead_ebadf)
#define dead_readlink 		((int (*) (struct  vop_readlink_args *))dead_ebadf)
#define dead_abortop 		((int (*) (struct  vop_abortop_args *))dead_badop)
#define dead_inactive 		((int (*) (struct  vop_inactive_args *))nullop)
#define dead_reclaim 		((int (*) (struct  vop_reclaim_args *))nullop)
#define dead_unlock 		((int (*) (struct  vop_unlock_args *))vop_nounlock)
#define dead_islocked 		((int (*) (struct  vop_islocked_args *))vop_noislocked)
#define dead_pathconf 		((int (*) (struct  vop_pathconf_args *))dead_ebadf)
#define dead_advlock 		((int (*) (struct  vop_advlock_args *))dead_ebadf)
#define dead_blkatoff 		((int (*) (struct  vop_blkatoff_args *))dead_badop)
#define dead_valloc 		((int (*) (struct  vop_valloc_args *))dead_badop)
#define dead_vfree 			((int (*) (struct  vop_vfree_args *))dead_badop)
#define dead_truncate 		((int (*) (struct  vop_truncate_args *))nullop)
#define dead_update 		((int (*) (struct  vop_update_args *))nullop)
#define dead_bwrite 		((int (*) (struct  vop_bwrite_args *))nullop)

/*
 * Global vfs data structures for dead
 */
struct vnodeops dead_vnodeops = {
		.vop_default = dead_default_error,/* default */
		.vop_lookup = dead_lookup,		/* lookup */
		.vop_create = dead_create,		/* create */
		.vop_mknod = dead_mknod,		/* mknod */
		.vop_open = dead_open,			/* open */
		.vop_close = dead_close,		/* close */
		.vop_access = dead_access,		/* access */
		.vop_getattr = dead_getattr,	/* getattr */
		.vop_setattr = dead_setattr,	/* setattr */
		.vop_read = dead_read,			/* read */
		.vop_write = dead_write,		/* write */
		.vop_lease = dead_lease_check,	/* lease */
		.vop_ioctl = dead_ioctl,		/* ioctl */
		.vop_select = dead_select,		/* select */
		.vop_poll = dead_poll,			/* poll */
		.vop_revoke = dead_revoke,		/* revoke */
		.vop_mmap = dead_mmap,			/* mmap */
		.vop_fsync = dead_fsync,		/* fsync */
		.vop_seek = dead_seek,			/* seek */
		.vop_remove = dead_remove,		/* remove */
		.vop_link = dead_link,			/* link */
		.vop_rename = dead_rename,		/* rename */
		.vop_mkdir = dead_mkdir,		/* mkdir */
		.vop_rmdir = dead_rmdir,		/* rmdir */
		.vop_symlink = dead_symlink,	/* symlink */
		.vop_readdir = dead_readdir,	/* readdir */
		.vop_readlink = dead_readlink,	/* readlink */
		.vop_abortop = dead_abortop,	/* abortop */
		.vop_inactive = dead_inactive,	/* inactive */
		.vop_reclaim = dead_reclaim,	/* reclaim */
		.vop_lock = dead_lock,			/* lock */
		.vop_unlock = dead_unlock,		/* unlock */
		.vop_bmap = dead_bmap,			/* bmap */
		.vop_strategy = dead_strategy,	/* strategy */
		.vop_print = dead_print,		/* print */
		.vop_islocked = dead_islocked,	/* islocked */
		.vop_pathconf = dead_pathconf,	/* pathconf */
		.vop_advlock = dead_advlock,	/* advlock */
		.vop_blkatoff = dead_blkatoff,	/* blkatoff */
		.vop_valloc = dead_valloc,		/* valloc */
		.vop_vfree = dead_vfree,		/* vfree */
		.vop_truncate = dead_truncate,	/* truncate */
		.vop_update = dead_update,		/* update */
		.vop_bwrite = dead_bwrite,		/* bwrite */
};

/*
 * Trivial lookup routine that always fails.
 */
/* ARGSUSED */
int
dead_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap;
{

	*ap->a_vpp = NULL;
	return (ENOTDIR);
}

/*
 * Open always fails as if device did not exist.
 */
/* ARGSUSED */
int
dead_open(ap)
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

	return (ENXIO);
}

/*
 * Vnode op for read
 */
/* ARGSUSED */
int
dead_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{

	if (chkvnlock(ap->a_vp))
		panic("dead_read: lock");
	/*
	 * Return EOF for tty devices, EIO for others
	 */
	if ((ap->a_vp->v_flag & VISTTY) == 0)
		return (EIO);
	return (0);
}

/*
 * Vnode op for write
 */
/* ARGSUSED */
int
dead_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{

	if (chkvnlock(ap->a_vp))
		panic("dead_write: lock");
	return (EIO);
}

/*
 * Device ioctl operation.
 */
/* ARGSUSED */
int
dead_ioctl(ap)
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		int  a_command;
		caddr_t  a_data;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	if (!chkvnlock(ap->a_vp))
		return (EBADF);
	return (VCALL(ap->a_vp, &ap->a_head));
}

/* ARGSUSED */
int
dead_select(ap)
	struct vop_select_args /* {
		struct vnode *a_vp;
		int  a_which;
		int  a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

	/*
	 * Let the user find out that the descriptor is gone.
	 */
	return (1);
}

int
dead_poll(ap)
	struct vop_poll_args *ap;
{

	if (ap->a_events & ~POLLSTANDARD)
		return (POLLNVAL);

	/*
	 * Let the user find out that the descriptor is gone.
	 */
	return (POLLHUP | ((POLLIN | POLLRDNORM) & ap->a_events));

}

/*
 * Just call the device strategy routine
 */
int
dead_strategy(ap)
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	if (ap->a_bp->b_vp == NULL || !chkvnlock(ap->a_bp->b_vp)) {
		ap->a_bp->b_flags |= B_ERROR;
		biodone(ap->a_bp);
		return (EIO);
	}
	return (VOP_STRATEGY(ap->a_bp));
}

/*
 * Wait until the vnode has finished changing state.
 */
int
dead_lock(ap)
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	/*
	 * Since we are not using the lock manager, we must clear
	 * the interlock here.
	 */
	if (ap->a_flags & LK_INTERLOCK) {
		simple_unlock(&vp->v_interlock);
		ap->a_flags &= ~LK_INTERLOCK;
	}
	if (!chkvnlock(vp))
		return (0);
	return (VCALL(vp, &ap->a_head));
}

/*
 * Wait until the vnode has finished changing state.
 */
int
dead_bmap(ap)
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap;
{
	if (!chkvnlock(ap->a_vp))
		return (EIO);
	return (VOP_BMAP(ap->a_vp, ap->a_bn, ap->a_vpp, ap->a_bnp, ap->a_runp));
}

/*
 * Print out the contents of a dead vnode.
 */
/* ARGSUSED */
int
dead_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	printf("tag VT_NON, dead vnode\n");
}

int
dead_default_error(ap)
	struct vop_generic_args *ap;
{
	return (EBADF);
}

/*
 * Empty vnode failed operation
 */
int
dead_ebadf(void)
{

	return (EBADF);
}

/*
 * Empty vnode bad operation
 */
int
dead_badop(v)
	void *v;
{

	panic("dead_badop called");
	/* NOTREACHED */
}

/*
 * We have to wait during times when the vnode is
 * in a state of change.
 */
int
chkvnlock(vp)
	register struct vnode *vp;
{
	int locked = 0;

	while (vp->v_flag & VXLOCK) {
		vp->v_flag |= VXWANT;
		sleep((caddr_t)vp, PINOD);
		locked = 1;
	}
	return (locked);
}
