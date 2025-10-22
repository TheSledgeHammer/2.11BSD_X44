/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)ffs_vnops.c	8.15 (Berkeley) 5/14/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/lockf.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <vm/include/vm.h>
#include <miscfs/fifofs/fifo.h>
#include <miscfs/specfs/specdev.h>

/* Global vfs data structures for ufs/ffs. */
struct vnodeops ffs_vnodeops = {
		.vop_default = vop_default_error,/* default */
		.vop_lookup = ufs_lookup,		/* lookup */
		.vop_create = ufs_create,		/* create */
		.vop_mknod = ufs_mknod,			/* mknod */
		.vop_open = ufs_open,			/* open */
		.vop_close = ufs_close,			/* close */
		.vop_access = ufs_access,		/* access */
		.vop_getattr = ufs_getattr,		/* getattr */
		.vop_setattr = ufs_setattr,		/* setattr */
		.vop_read = ffs_read,			/* read */
		.vop_write = ffs_write,			/* write */
		.vop_lease = ufs_lease_check,	/* lease */
		.vop_ioctl = ufs_ioctl,			/* ioctl */
		.vop_select = ufs_select,		/* select */
		.vop_poll = ufs_poll,			/* poll */
		.vop_revoke = ufs_revoke,		/* revoke */
		.vop_mmap = ufs_mmap,			/* mmap */
		.vop_fsync = ffs_fsync,			/* fsync */
		.vop_seek = ufs_seek,			/* seek */
		.vop_remove = ufs_remove,		/* remove */
		.vop_link = ufs_link,			/* link */
		.vop_rename = ufs_rename,		/* rename */
		.vop_mkdir = ufs_mkdir,			/* mkdir */
		.vop_rmdir = ufs_rmdir,			/* rmdir */
		.vop_symlink = ufs_symlink,		/* symlink */
		.vop_readdir = ufs_readdir,		/* readdir */
		.vop_readlink = ufs_readlink,	/* readlink */
		.vop_abortop = ufs_abortop,		/* abortop */
		.vop_inactive = ufs_inactive,	/* inactive */
		.vop_reclaim = ffs_reclaim,		/* reclaim */
		.vop_lock = ufs_lock,			/* lock */
		.vop_unlock = ufs_unlock,		/* unlock */
		.vop_bmap = ufs_bmap,			/* bmap */
		.vop_strategy = ufs_strategy,	/* strategy */
		.vop_print = ufs_print,			/* print */
		.vop_islocked = ufs_islocked,	/* islocked */
		.vop_pathconf = ufs_pathconf,	/* pathconf */
		.vop_advlock = ufs_advlock,		/* advlock */
		.vop_blkatoff = ffs_blkatoff,	/* blkatoff */
		.vop_valloc = ffs_valloc,		/* valloc */
		.vop_reallocblks = ffs_reallocblks,/* reallocblks */
		.vop_vfree = ffs_vfree,			/* vfree */
		.vop_truncate = ffs_truncate,	/* truncate */
		.vop_update = ffs_update,		/* update */
		.vop_bwrite = vn_bwrite,		/* bwrite */
};

struct vnodeops ffs_specops = {
		.vop_default = vop_default_error,/* default */
		.vop_lookup = spec_lookup,		/* lookup */
		.vop_create = spec_create,		/* create */
		.vop_mknod = spec_mknod,		/* mknod */
		.vop_open = spec_open,			/* open */
		.vop_close = ufsspec_close,		/* close */
		.vop_access = ufs_access,		/* access */
		.vop_getattr = ufs_getattr,		/* getattr */
		.vop_setattr = ufs_setattr,		/* setattr */
		.vop_read = ufsspec_read,		/* read */
		.vop_write = ufsspec_write,		/* write */
		.vop_lease = spec_lease_check,	/* lease */
		.vop_ioctl = spec_ioctl,		/* ioctl */
		.vop_select = spec_select,		/* select */
		.vop_poll = spec_poll,			/* poll */
		.vop_revoke = spec_revoke,		/* revoke */
		.vop_mmap = spec_mmap,			/* mmap */
		.vop_fsync = ffs_fsync,			/* fsync */
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
		.vop_inactive = ufs_inactive,	/* inactive */
		.vop_reclaim = ffs_reclaim,		/* reclaim */
		.vop_lock = ufs_lock,			/* lock */
		.vop_unlock = ufs_unlock,		/* unlock */
		.vop_bmap = spec_bmap,			/* bmap */
		.vop_strategy = spec_strategy,	/* strategy */
		.vop_print = ufs_print,			/* print */
		.vop_islocked = ufs_islocked,	/* islocked */
		.vop_pathconf = spec_pathconf,	/* pathconf */
		.vop_advlock = spec_advlock,	/* advlock */
		.vop_blkatoff = spec_blkatoff,	/* blkatoff */
		.vop_valloc = spec_valloc,		/* valloc */
		.vop_reallocblks = spec_reallocblks,/* reallocblks */
		.vop_vfree = ffs_vfree,			/* vfree */
		.vop_truncate = spec_truncate,	/* truncate */
		.vop_update = ffs_update,		/* update */
		.vop_bwrite = vn_bwrite,		/* bwrite */
};

#ifdef FIFO
struct vnodeops ffs_fifoops = {
		.vop_default = vop_default_error,/* default */
		.vop_lookup = fifo_lookup,		/* lookup */
		.vop_create = fifo_create,		/* create */
		.vop_mknod = fifo_mknod,		/* mknod */
		.vop_open = fifo_open,			/* open */
		.vop_close = ufsfifo_close,		/* close */
		.vop_access = ufs_access,		/* access */
		.vop_getattr = ufs_getattr,		/* getattr */
		.vop_setattr = ufs_setattr,		/* setattr */
		.vop_read = ufsfifo_read,		/* read */
		.vop_write = ufsfifo_write,		/* write */
		.vop_lease = fifo_lease_check,	/* lease */
		.vop_ioctl = fifo_ioctl,		/* ioctl */
		.vop_select = fifo_select,		/* select */
		.vop_poll = fifo_poll,			/* poll */
		.vop_revoke = fifo_revoke,		/* revoke */
		.vop_mmap = fifo_mmap,			/* mmap */
		.vop_fsync = fifo_fsync,		/* fsync */
		.vop_seek = fifo_seek,			/* seek */
		.vop_remove = fifo_remove,		/* remove */
		.vop_link = fifo_link,			/* link */
		.vop_rename = fifo_rename,		/* rename */
		.vop_mkdir = fifo_mkdir,		/* mkdir */
		.vop_rmdir = fifo_rmdir,		/* rmdir */
		.vop_symlink = fifo_symlink,	/* symlink */
		.vop_readdir = fifo_readdir,	/* readdir */
		.vop_readlink = fifo_readlink,	/* readlink */
		.vop_abortop = fifo_abortop,	/* abortop */
		.vop_inactive = ufs_inactive,	/* inactive */
		.vop_reclaim = ffs_reclaim,		/* reclaim */
		.vop_lock = ufs_lock,			/* lock */
		.vop_unlock = ufs_unlock,		/* unlock */
		.vop_bmap = fifo_bmap,			/* bmap */
		.vop_strategy = fifo_strategy,	/* strategy */
		.vop_print = ufs_print,			/* print */
		.vop_islocked = ufs_islocked,	/* islocked */
		.vop_pathconf = fifo_pathconf,	/* pathconf */
		.vop_advlock = fifo_advlock,	/* advlock */
		.vop_blkatoff = fifo_blkatoff,	/* blkatoff */
		.vop_valloc = fifo_valloc,		/* valloc */
		.vop_reallocblks = fifo_reallocblks,/* reallocblks */
		.vop_vfree = ffs_vfree,			/* vfree */
		.vop_truncate = fifo_truncate,	/* truncate */
		.vop_update = ffs_update,		/* update */
		.vop_bwrite = vn_bwrite,		/* bwrite */
};
#endif /* FIFO */

/*
 * Enabling cluster read/write operations.
 */
int doclusterread = 1;
int doclusterwrite = 1;

#include <ufs/ufs/ufs_readwrite.c>

/*
 * Synch an open file.
 */
/* ARGSUSED */
int
ffs_fsync(ap)
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_waitfor;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	register struct buf *bp;
	struct timeval tv;
	struct buf *nbp;
	int s;

	/*
	 * Flush all dirty buffers associated with a vnode.
	 */
loop:
	s = splbio();
	for (bp = LIST_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
		nbp = LIST_NEXT(bp, b_vnbufs);
		if ((bp->b_flags & B_BUSY))
			continue;
		if ((bp->b_flags & B_DELWRI) == 0)
			panic("ffs_fsync: not dirty");
		bremfree(bp);
		bp->b_flags |= B_BUSY;
		splx(s);
		/*
		 * Wait for I/O associated with indirect blocks to complete,
		 * since there is no way to quickly wait for them below.
		 */
		if (bp->b_vp == vp || ap->a_waitfor == MNT_NOWAIT)
			(void) bawrite(bp);
		else
			(void) bwrite(bp);
		goto loop;
	}
	if (ap->a_waitfor == MNT_WAIT) {
		while (vp->v_numoutput) {
			vp->v_flag |= VBWAIT;
			sleep((caddr_t)&vp->v_numoutput, PRIBIO + 1);
		}
#ifdef DIAGNOSTIC
		if (LIST_FIRST(&vp->v_dirtyblkhd)) {
			vprint("ffs_fsync: dirty", vp);
			goto loop;
		}
#endif
	}
	splx(s);
	tv = time;
	return (VOP_UPDATE(ap->a_vp, &tv, &tv, ap->a_waitfor == MNT_WAIT));
}

/*
 * Reclaim an inode so that it can be used for other purposes.
 */
int
ffs_reclaim(ap)
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	int error;

	if ((error = ufs_reclaim(vp, ap->a_p)))
		return (error);
	FREE(vp->v_data, VFSTOUFS(vp->v_mount)->um_devvp->v_tag == VT_MFS ? M_MFSNODE : M_FFSNODE);
	vp->v_data = NULL;
	return (0);
}
