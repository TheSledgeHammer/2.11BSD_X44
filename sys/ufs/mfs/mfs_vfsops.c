/*
 * Copyright (c) 1989, 1990, 1993, 1994
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
 *	@(#)mfs_vfsops.c	8.11 (Berkeley) 6/19/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <ufs/mfs/mfsnode.h>
#include <ufs/mfs/mfs_extern.h>

caddr_t	mfs_rootbase;	/* address of mini-root in kernel virtual memory */
u_long	mfs_rootsize;	/* size of mini-root in bytes */

static	int mfs_minor;	/* used for building internal dev_t */

/*
 * mfs vfs operations.
 */
struct vfsops mfs_vfsops = {
		.vfs_mount = mfs_mount,
		.vfs_start = mfs_start,
		.vfs_unmount = ffs_unmount,
		.vfs_root = ufs_root,
		.vfs_quotactl = ufs_quotactl,
		.vfs_statfs = mfs_statfs,
		.vfs_sync = ffs_sync,
		.vfs_vget = ffs_vget,
		.vfs_fhtovp = ffs_fhtovp,
		.vfs_vptofh = ffs_vptofh,
		.vfs_init = mfs_init,
		.vfs_sysctl = ffs_sysctl,
};

/*
 * Called by main() when mfs is going to be mounted as root.
 */
int
mfs_mountroot()
{
	extern struct vnode *rootvp;
	struct fs *fs;
	struct mount *mp;
	struct proc *p = curproc;	/* XXX */
	struct ufsmount *ump;
	struct mfsnode *mfsp;
	int error;

	/*
	 * Get vnodes for swapdev and rootdev.
	 */
	if ((error = bdevvp(swapdev, &swapdev_vp)) ||
	    (error = bdevvp(rootdev, &rootvp))) {
		printf("mfs_mountroot: can't setup bdevvp's");
		return (error);
	}
	if ((error = vfs_rootmountalloc(MOUNT_MFS, "mfs_root", &mp)))
		return (error);
	mfsp = malloc(sizeof *mfsp, M_MFSNODE, M_WAITOK);
	rootvp->v_data = mfsp;
	rootvp->v_op = &mfs_vnodeops;
	rootvp->v_tag = VT_MFS;
	mfsp->mfs_baseoff = mfs_rootbase;
	mfsp->mfs_size = mfs_rootsize;
	mfsp->mfs_vnode = rootvp;
	mfsp->mfs_pid = p->p_pid;
	mfsp->mfs_shutdown = 0;
	bufq_alloc(&mfsp->mfs_buflist, BUFQ_FCFS);
	if ((error = ffs_mountfs(rootvp, mp, p))) {
		mp->mnt_vfc->vfc_refcount--;
		vfs_unbusy(mp, p);
		bufq_free(&mfsp->mfs_buflist);
		free(mp, M_MOUNT);
		free(mfsp, M_MFSNODE);
		return (error);
	}
	simple_lock(&mountlist_slock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	simple_unlock(&mountlist_slock);
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	(void) copystr(mp->mnt_stat.f_mntonname, fs->fs_fsmnt, MNAMELEN - 1, 0);
	(void)ffs_statfs(mp, &mp->mnt_stat, p);
	vfs_unbusy(mp, p);
	inittodr((time_t)0);
	return (0);
}

/*
 * This is called early in boot to set the base address and size
 * of the mini-root.
 */
int
mfs_initminiroot(base)
	caddr_t base;
{
	struct fs *fs = (struct fs *)(base + SBOFF);
	extern int (*mountroot)();

	/* check for valid super block */
	if (fs->fs_magic != FS_MAGIC || fs->fs_bsize > MAXBSIZE ||
	    fs->fs_bsize < sizeof(struct fs))
		return (0);
	mountroot = mfs_mountroot;
	mfs_rootbase = base;
	mfs_rootsize = fs->fs_fsize * fs->fs_size;
	rootdev = makedev(255, mfs_minor++);
	return (mfs_rootsize);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
/* ARGSUSED */
int
mfs_mount(mp, path, data, ndp, p)
	register struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *devvp;
	struct mfs_args args;
	struct ufsmount *ump;
	register struct fs *fs;
	register struct mfsnode *mfsp;
	u_int size;
	int flags, error;

	if ((error = copyin(data, (caddr_t)&args, sizeof (struct mfs_args))))
		return (error);

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		ump = VFSTOUFS(mp);
		fs = ump->um_fs;
		if (fs->fs_ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			if ((error = ffs_flushfiles(mp, flags, p)))
				return (error);
		}
		if (fs->fs_ronly && (mp->mnt_flag & MNT_WANTRDWR))
			fs->fs_ronly = 0;
#ifdef EXPORTMFS
		if (args.fspec == 0)
			return (vfs_export(mp, &ump->um_export, &args.export));
#endif
		return (0);
	}
	error = getnewvnode(VT_MFS, (struct mount *)0, &mfs_vnodeops, &devvp);
	if (error)
		return (error);
	devvp->v_type = VBLK;
	if (checkalias(devvp, makedev(255, mfs_minor++), (struct mount *)0))
		panic("mfs_mount: dup dev");
	mfsp = (struct mfsnode *)malloc(sizeof *mfsp, M_MFSNODE, M_WAITOK);
	devvp->v_data = mfsp;
	mfsp->mfs_baseoff = args.base;
	mfsp->mfs_size = args.size;
	mfsp->mfs_vnode = devvp;
	mfsp->mfs_pid = p->p_pid;
	mfsp->mfs_shutdown = 0;
	bufq_alloc(&mfsp->mfs_buflist, BUFQ_FCFS);
	if ((error = ffs_mountfs(devvp, mp, p))) {
		mfsp->mfs_shutdown = 1;
		vrele(devvp);
		return (error);
	}
	ump = VFSTOUFS(mp);
	fs = ump->um_fs;
	(void) copyinstr(path, fs->fs_fsmnt, sizeof(fs->fs_fsmnt) - 1, &size);
	bzero(fs->fs_fsmnt + size, sizeof(fs->fs_fsmnt) - size);
	bcopy((caddr_t)fs->fs_fsmnt, (caddr_t)mp->mnt_stat.f_mntonname, MNAMELEN);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void) mfs_statfs(mp, &mp->mnt_stat, p);
	return (0);
}

int	mfs_pri = PWAIT | PCATCH;		/* XXX prob. temp */

/*
 * Used to grab the process and keep it in the kernel to service
 * memory filesystem I/O requests.
 *
 * Loop servicing I/O requests.
 * Copy the requested data into or out of the memory filesystem
 * address space.
 */
/* ARGSUSED */
int
mfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	register struct vnode *vp = VFSTOUFS(mp)->um_devvp;
	register struct mfsnode *mfsp = VTOMFS(vp);
	register struct buf *bp;
	register caddr_t base;

	base = mfsp->mfs_baseoff;
	while (mfsp->mfs_shutdown != 1) {
		while ((bp = BUFQ_GET(&mfsp->mfs_buflist)) != NULL) {
			mfs_doio(bp, base);
			wakeup((caddr_t)bp);
		}
		/*
		 * If a non-ignored signal is received, try to unmount.
		 * If that fails, clear the signal (it has been "processed"),
		 * otherwise we will loop here, as tsleep will always return
		 * EINTR/ERESTART.
		 */
		if (tsleep((caddr_t) vp, mfs_pri, "mfsidl", 0)
				&& dounmount(mp, 0, p) != 0) {
			CLRSIG(p, CURSIG(p));
		}
	}
	KASSERT(BUFQ_PEEK(&mfsp->mfs_buflist) == NULL);
	bufq_free(&mfsp->mfs_buflist);
	return (0);
}

/*
 * Get file system statistics.
 */
int
mfs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{
	int error;

	error = ffs_statfs(mp, sbp, p);
	sbp->f_type = mp->mnt_vfc->vfc_typenum;
	return (error);
}
