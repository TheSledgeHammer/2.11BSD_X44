/*
 * Copyright (c) 1989, 1993, 1995
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
 *	@(#)spec_vnops.c	8.14 (Berkeley) 5/21/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/lockf.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <miscfs/specfs/specdev.h>

/* symbolic sleep message strings for devices */
char	devopn[] = "devopn";
char	devio[] = "devio";
char	devwait[] = "devwait";
char	devin[] = "devin";
char	devout[] = "devout";
char	devioc[] = "devioc";
char	devcls[] = "devcls";

struct vnode	*speclisth[SPECHSZ];

struct vnodeops spec_vnodeops = {
		.vop_default = vop_default_error,/* default */
		.vop_lookup = spec_lookup,		/* lookup */
		.vop_create = spec_create,		/* create */
		.vop_mknod = spec_mknod,		/* mknod */
		.vop_open = spec_open,			/* open */
		.vop_close = spec_close,		/* close */
		.vop_access = spec_access,		/* access */
		.vop_getattr = spec_getattr,	/* getattr */
		.vop_setattr = spec_setattr,	/* setattr */
		.vop_read = spec_read,			/* read */
		.vop_write = spec_write,		/* write */
		.vop_lease = spec_lease_check,	/* lease */
		.vop_ioctl = spec_ioctl,		/* ioctl */
		.vop_select = spec_select,		/* select */
		.vop_poll = spec_poll,			/* poll */
		.vop_revoke = spec_revoke,		/* revoke */
		.vop_mmap = spec_mmap,			/* mmap */
		.vop_fsync = spec_fsync,		/* fsync */
		.vop_seek = spec_seek,			/* seek */
		.vop_remove = spec_remove,		/* remove */
		.vop_link = spec_link,			/* link */
		.vop_rename = spec_rename,		/* rename */
		.vop_mkdir = spec_mkdir,		/* mkdir */
		.vop_rmdir = spec_rmdir,		/* rmdir */
		.vop_symlink = spec_symlink,	/* symlink */
		.vop_readdir = spec_readdir,	/* readdir */
		.vop_readlink = spec_readlink,	/* readlink */
		.vop_abortop = spec_abortop,	/* abortop */
		.vop_inactive = spec_inactive,	/* inactive */
		.vop_reclaim = spec_reclaim,	/* reclaim */
		.vop_lock = spec_lock,			/* lock */
		.vop_unlock = spec_unlock,		/* unlock */
		.vop_bmap = spec_bmap,			/* bmap */
		.vop_strategy = spec_strategy,	/* strategy */
		.vop_print = spec_print,		/* print */
		.vop_islocked = spec_islocked,	/* islocked */
		.vop_pathconf = spec_pathconf,	/* pathconf */
		.vop_advlock = spec_advlock,	/* advlock */
		.vop_blkatoff = spec_blkatoff,	/* blkatoff */
		.vop_valloc = spec_valloc,		/* valloc */
		.vop_vfree = spec_vfree,		/* vfree */
		.vop_truncate = spec_truncate,	/* truncate */
		.vop_update = spec_update,		/* update */
		.vop_bwrite = spec_bwrite,		/* bwrite */
};

/*
 * Trivial lookup routine that always fails.
 */
int
spec_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
	} */ *ap;
{

	*ap->a_vpp = NULL;
	return (ENOTDIR);
}

/*
 * Open a special file.
 */
/* ARGSUSED */
int
spec_open(ap)
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct proc *p = ap->a_p;
	struct vnode *bvp, *vp = ap->a_vp;
	const struct cdevsw *cdev;
	const struct bdevsw *bdev;
	dev_t blkdev, dev = (dev_t)vp->v_rdev;
	int maj = major(dev);
	int error;

	/*
	 * Don't allow open if fs is mounted -nodev.
	 */
	if (vp->v_mount && (vp->v_mount->mnt_flag & MNT_NODEV))
		return (ENXIO);

	switch (vp->v_type) {

	case VCHR:
		cdev = cdevsw_lookup(dev);

		if ((u_int)maj >= nchrdev || cdev == NULL)
			return (ENXIO);
		if (ap->a_cred != FSCRED && (ap->a_mode & FWRITE)) {
			/*
			 * When running in very secure mode, do not allow
			 * opens for writing of any disk character devices.
			 */
			if (securelevel >= 2 && cdev->d_type == D_DISK)
				return (EPERM);
			/*
			 * When running in secure mode, do not allow opens
			 * for writing of /dev/mem, /dev/kmem, or character
			 * devices whose corresponding block devices are
			 * currently mounted.
			 */
			if (securelevel >= 1) {
				blkdev = chrtoblk(dev);
				if (blkdev != (dev_t)NODEV &&
				    vfinddev(blkdev, VBLK, &bvp) &&
				    bvp->v_usecount > 0 &&
				    (error = vfs_mountedon(bvp)))
					return (error);
				if (iskmemdev(dev))
					return (EPERM);
			}
		}
		if (cdev->d_type == D_TTY)
			vp->v_flag |= VISTTY;
		VOP_UNLOCK(vp, 0, p);
		error = (*cdev->d_open)(dev, ap->a_mode, S_IFCHR, p);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);

		return (error);

	case VBLK:
		bdev = bdevsw_lookup(dev);

		if ((u_int)maj >= nblkdev || bdev == NULL)
			return (ENXIO);
		/*
		 * When running in very secure mode, do not allow
		 * opens for writing of any disk block devices.
		 */
		if (securelevel >= 2 && ap->a_cred != FSCRED &&
		    (ap->a_mode & FWRITE) && bdev->d_type == D_DISK)
			return (EPERM);
		/*
		 * Do not allow opens of block devices that are
		 * currently mounted.
		 */
		if ((error = vfs_mountedon(vp)))
			return (error);
		return ((*bdev->d_open)(dev, ap->a_mode, S_IFBLK, p));
	}
	return (0);
}

/*
 * Vnode op for read
 */
/* ARGSUSED */
int
spec_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct uio *uio = ap->a_uio;
 	struct proc *p = uio->uio_procp;
	struct buf *bp;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	daddr_t bn, nextbn;
	long bsize, bscale;
	struct partinfo dpart;
	int n, on, majordev, (*ioctl)();
	int error = 0;
	dev_t dev;

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_READ)
		panic("spec_read mode");
	if (uio->uio_segflg == UIO_USERSPACE && uio->uio_procp != curproc)
		panic("spec_read proc");
#endif
	if (uio->uio_resid == 0)
		return (0);

	switch (vp->v_type) {

	case VCHR:
		VOP_UNLOCK(vp, 0, p);
		cdev = cdevsw_lookup(vp->v_rdev);
		error = (*cdev->d_read)(vp->v_rdev, uio, ap->a_ioflag);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		return (error);

	case VBLK:
		if (uio->uio_offset < 0)
			return (EINVAL);
		bsize = BLKDEV_IOSIZE;
		bdev = bdevsw_lookup(vp->v_rdev);
		dev = vp->v_rdev;
		if ((majordev = major(dev)) < nblkdev &&
				 ((*bdev->d_ioctl)(vp->v_rdev, DIOCGPART, (caddr_t)&dpart, FREAD, p) == 0)) {
			if (dpart.part->p_fstype == FS_BSDFFS &&
				    dpart.part->p_frag != 0 && dpart.part->p_fsize != 0) {
				bsize = dpart.part->p_frag * dpart.part->p_fsize;
			}
		}
		bscale = bsize / DEV_BSIZE;
		do {
			bn = (uio->uio_offset / DEV_BSIZE) &~ (bscale - 1);
			on = uio->uio_offset % bsize;
			n = min((unsigned)(bsize - on), uio->uio_resid);
			if (vp->v_lastr + bscale == bn) {
				nextbn = bn + bscale;
				error = breadn(vp, bn, (int)bsize, &nextbn,
					(int *)&bsize, 1, NOCRED, &bp);
			} else
				error = bread(vp, bn, (int)bsize, NOCRED, &bp);
			vp->v_lastr = bn;
			n = min(n, bsize - bp->b_resid);
			if (error) {
				brelse(bp);
				return (error);
			}
			error = uiomove((char *)bp->b_data + on, n, uio);
			if (n + on == bsize)
				bp->b_flags |= B_AGE;
			brelse(bp);
		} while (error == 0 && uio->uio_resid > 0 && n != 0);
		return (error);

	default:
		panic("spec_read type");
	}
	/* NOTREACHED */
	return (0);
}

/*
 * Vnode op for write
 */
/* ARGSUSED */
int
spec_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct uio *uio = ap->a_uio;
	struct proc *p = uio->uio_procp;
	struct buf *bp;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	daddr_t bn;
	int bsize, blkmask;
	struct partinfo dpart;
	register int n, on;
	int error = 0;

#ifdef DIAGNOSTIC
	if (uio->uio_rw != UIO_WRITE)
		panic("spec_write mode");
	if (uio->uio_segflg == UIO_USERSPACE && uio->uio_procp != curproc)
		panic("spec_write proc");
#endif

	switch (vp->v_type) {

	case VCHR:
		VOP_UNLOCK(vp, 0, p);
		cdev = cdevsw_lookup(vp->v_rdev);
		error = (*cdev->d_write)(vp->v_rdev, uio, ap->a_ioflag);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		return (error);

	case VBLK:
		if (uio->uio_resid == 0)
			return (0);
		if (uio->uio_offset < 0)
			return (EINVAL);
		bsize = BLKDEV_IOSIZE;
		bdev = bdevsw_lookup(vp->v_rdev);
		if ((*bdev->d_ioctl)(vp->v_rdev, DIOCGPART,
		    (caddr_t)&dpart, FREAD, p) == 0) {
			if (dpart.part->p_fstype == FS_BSDFFS &&
			    dpart.part->p_frag != 0 && dpart.part->p_fsize != 0)
				bsize = dpart.part->p_frag *
				    dpart.part->p_fsize;
		}
		blkmask = (bsize / DEV_BSIZE) - 1;
		do {
			bn = (uio->uio_offset / DEV_BSIZE) &~ blkmask;
			on = uio->uio_offset % bsize;
			n = min((unsigned)(bsize - on), uio->uio_resid);
			if (n == bsize)
				bp = getblk(vp, bn, bsize, 0, 0);
			else
				error = bread(vp, bn, bsize, NOCRED, &bp);
			n = min(n, bsize - bp->b_resid);
			if (error) {
				brelse(bp);
				return (error);
			}
			error = uiomove((char *)bp->b_data + on, n, uio);
			if (n + on == bsize) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else
				bdwrite(bp);
		} while (error == 0 && uio->uio_resid > 0 && n != 0);
		return (error);

	default:
		panic("spec_write type");
	}
	/* NOTREACHED */
	return (0);
}

/*
 * Device ioctl operation.
 */
/* ARGSUSED */
int
spec_ioctl(ap)
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		int  a_command;
		caddr_t  a_data;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	dev_t dev;

	dev = ap->a_vp->v_rdev;
	switch (ap->a_vp->v_type) {
	case VCHR:
		cdev = cdevsw_lookup(dev);
		if (cdev == NULL)
			return (ENXIO);
		return ((*cdev->d_ioctl)(dev, ap->a_command, ap->a_data, ap->a_fflag,
				ap->a_p));

	case VBLK:
		bdev = bdevsw_lookup(dev);
		if (bdev == NULL)
			return (ENXIO);
		if (ap->a_command == 0 && (int)ap->a_data == B_TAPE)
			if (bdev->d_type == D_TAPE)
				return (0);
			else
				return (1);
		return ((*bdev->d_ioctl)(dev, ap->a_command, ap->a_data, ap->a_fflag,
				ap->a_p));

	default:
		panic("spec_ioctl");
		/* NOTREACHED */
	}
	return (0);
}

/* ARGSUSED */
int
spec_select(ap)
	struct vop_select_args /* {
		struct vnode *a_vp;
		int  a_which;
		int  a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	const struct cdevsw *cdev;
	register dev_t dev;

	switch (ap->a_vp->v_type) {
	case VCHR:
		dev = ap->a_vp->v_rdev;
		cdev = cdevsw_lookup(dev);
		return (*cdev->d_select)(dev, ap->a_which, ap->a_p);

	default:
		return (1);		/* XXX */
	}
}

int
spec_poll(ap)
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
		struct proc *a_p;
	} */ *ap;
{
	const struct cdevsw *cdev;
	register dev_t dev;

	switch (ap->a_vp->v_type) {
	case VCHR:
		dev = ap->a_vp->v_rdev;
		cdev = cdevsw_lookup(dev);
		return (*cdev->d_poll)(dev, ap->a_events, ap->a_p);

	default:
		return (1);		/* XXX */
	}
}

/*
 * Synch buffers associated with a block device
 */
/* ARGSUSED */
int
spec_fsync(ap)
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int  a_waitfor;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct buf *bp;
	struct buf *nbp;
	int s;

	if (vp->v_type == VCHR)
		return (0);
	/*
	 * Flush all dirty buffers associated with a block device.
	 */
loop:
	s = splbio();
	for (bp = vp->v_dirtyblkhd.lh_first; bp; bp = nbp) {
		nbp = bp->b_vnbufs.le_next;
		if ((bp->b_flags & B_BUSY))
			continue;
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("spec_fsync: not dirty");
		bremfree(bp);
		bp->b_flags |= B_BUSY;
		splx(s);
		bawrite(bp);
		goto loop;
	}
	if (ap->a_waitfor == MNT_WAIT) {
		while (vp->v_numoutput) {
			vp->v_flag |= VBWAIT;
			sleep((caddr_t)&vp->v_numoutput, PRIBIO + 1);
		}
#ifdef DIAGNOSTIC
		if (vp->v_dirtyblkhd.lh_first) {
			vprint("spec_fsync: dirty", vp);
			goto loop;
		}
#endif
	}
	splx(s);
	return (0);
}

int
spec_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap;
{

	VOP_UNLOCK(ap->a_vp, 0, ap->a_p);
	return (0);
}

/*
 * Just call the device strategy routine
 */
int
spec_strategy(ap)
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	const struct bdevsw *bdev;
	struct buf *bp = ap->a_bp;

    bdev = bdevsw_lookup(bp->b_dev);
	(*bdev->d_strategy)(bp);
	return (0);
}

/*
 * This is a noop, simply returning what one has been given.
 */
int
spec_bmap(ap)
	struct vop_bmap_args /* {
		struct vnode *a_vp;
		daddr_t  a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap;
{

	if (ap->a_vpp != NULL)
		*ap->a_vpp = ap->a_vp;
	if (ap->a_bnp != NULL)
		*ap->a_bnp = ap->a_bn;
	if (ap->a_runp != NULL)
		*ap->a_runp = 0;
	return (0);
}

/*
 * Device close routine
 */
/* ARGSUSED */
int
spec_close(ap)
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	const struct bdevsw *bdev;
	const struct cdevsw *cdev;
	dev_t dev = vp->v_rdev;
	int (*devclose) (dev_t, int, int, struct proc *);
	int mode, error;

	switch (vp->v_type) {

	case VCHR:
		/*
		 * Hack: a tty device that is a controlling terminal
		 * has a reference from the session structure.
		 * We cannot easily tell that a character device is
		 * a controlling terminal, unless it is the closing
		 * process' controlling terminal.  In that case,
		 * if the reference count is 2 (this last descriptor
		 * plus the session), release the reference from the session.
		 */
		if (vcount(vp) == 2 && ap->a_p &&
		    vp == ap->a_p->p_session->s_ttyvp) {
			vrele(vp);
			ap->a_p->p_session->s_ttyvp = NULL;
		}
		/*
		 * If the vnode is locked, then we are in the midst
		 * of forcably closing the device, otherwise we only
		 * close on last reference.
		 */
		if (vcount(vp) > 1 && (vp->v_flag & VXLOCK) == 0)
			return (0);
		cdev = cdevsw_lookup(dev);
		devclose = cdev->d_close;
		mode = S_IFCHR;
		break;

	case VBLK:
		/*
		 * On last close of a block device (that isn't mounted)
		 * we must invalidate any in core blocks, so that
		 * we can, for instance, change floppy disks.
		 */
		if ((error = vinvalbuf(vp, V_SAVE, ap->a_cred, ap->a_p, 0, 0)))
			return (error);
		/*
		 * We do not want to really close the device if it
		 * is still in use unless we are trying to close it
		 * forcibly. Since every use (buffer, vnode, swap, cmap)
		 * holds a reference to the vnode, and because we mark
		 * any other vnodes that alias this device, when the
		 * sum of the reference counts on all the aliased
		 * vnodes descends to one, we are on last close.
		 */
		if (vcount(vp) > 1 && (vp->v_flag & VXLOCK) == 0)
			return (0);
		bdev = bdevsw_lookup(dev);
		devclose = bdev->d_close;
		mode = S_IFBLK;
		break;

	default:
		panic("spec_close: not special");
	}

	return ((*devclose)(dev, ap->a_fflag, mode, ap->a_p));
}

/*
 * Print out the contents of a special device vnode.
 */
int
spec_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	printf("tag VT_NON, dev %d, %d\n", major(ap->a_vp->v_rdev),
		minor(ap->a_vp->v_rdev));
}

/*
 * Return POSIX pathconf information applicable to special devices.
 */
int
spec_pathconf(ap)
	struct vop_pathconf_args /* {
		struct vnode *a_vp;
		int a_name;
		int *a_retval;
	} */ *ap;
{

	switch (ap->a_name) {
	case _PC_LINK_MAX:
		*ap->a_retval = LINK_MAX;
		return (0);
	case _PC_MAX_CANON:
		*ap->a_retval = MAX_CANON;
		return (0);
	case _PC_MAX_INPUT:
		*ap->a_retval = MAX_INPUT;
		return (0);
	case _PC_PIPE_BUF:
		*ap->a_retval = PIPE_BUF;
		return (0);
	case _PC_CHOWN_RESTRICTED:
		*ap->a_retval = 1;
		return (0);
	case _PC_VDISABLE:
		*ap->a_retval = _POSIX_VDISABLE;
		return (0);
	default:
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * Special device advisory byte-level locks.
 */
/* ARGSUSED */
int
spec_advlock(ap)
	struct vop_advlock_args /* {
		struct vnode *a_vp;
		caddr_t  a_id;
		int  a_op;
		struct flock *a_fl;
		int  a_flags;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;

	return lf_advlock(ap, &vp->v_speclockf, (off_t)0);
}

/*
 * Special device failed operation
 */
int
spec_ebadf(void)
{

	return (EBADF);
}

/*
 * Special device bad operation
 */
int
spec_badop(v)
	void *v;
{

	panic("spec_badop called");
	/* NOTREACHED */
}
