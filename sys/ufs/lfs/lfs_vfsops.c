/*
 * Copyright (c) 1989, 1991, 1993, 1994
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
 *	@(#)lfs_vfsops.c	8.20 (Berkeley) 6/10/95
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/user.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>

#include <ufs/lfs/lfs.h>
#include <ufs/lfs/lfs_extern.h>
#include <miscfs/specfs/specdev.h>

int lfs_mountfs (struct vnode *, struct mount *, struct proc *);
int lfs1_vget_debug(struct inode *, struct lfs *, struct ufs1_dinode *, struct buf *, ino_t, int);
int lfs2_vget_debug(struct inode *, struct lfs *, struct ufs2_dinode *, struct buf *, ino_t, int);

#define	 lfs_sysctl ((int (*) (int *, u_int, void *, size_t *, void *, size_t, struct proc *))eopnotsupp)

struct vfsops lfs_vfsops = {
		.vfs_mount = lfs_mount,
		.vfs_start = ufs_start,
		.vfs_unmount = lfs_unmount,
		.vfs_root = ufs_root,
		.vfs_quotactl = ufs_quotactl,
		.vfs_statfs = lfs_statfs,
		.vfs_sync = lfs_sync,
		.vfs_vget = lfs_vget,
		.vfs_fhtovp = lfs_fhtovp,
		.vfs_vptofh = lfs_vptofh,
		.vfs_init = lfs_init,
		.vfs_sysctl = lfs_sysctl,
};

/*
 * Called by main() when ufs is going to be mounted as root.
 */
int
lfs_mountroot()
{
	extern struct vnode *rootvp;
	struct fs *fs;
	struct mount *mp;
	struct proc *p = curproc;	/* XXX */
	int error;
	
	/*
	 * Get vnodes for swapdev and rootdev.
	 */
	if ((error = bdevvp(swapdev, &swapdev_vp)) ||
	    (error = bdevvp(rootdev, &rootvp))) {
		printf("lfs_mountroot: can't setup bdevvp's");
		return (error);
	}
	if ((error = vfs_rootmountalloc(MOUNT_LFS, "root_device", &mp)))
		return (error);
	if ((error = lfs_mountfs(rootvp, mp, p))) {
		mp->mnt_vfc->vfc_refcount--;
		vfs_unbusy(mp, p);
		free(mp, M_MOUNT);
		return (error);
	}
	simple_lock(&mountlist_slock);
	CIRCLEQ_INSERT_TAIL(&mountlist, mp, mnt_list);
	simple_unlock(&mountlist_slock);
	(void)lfs_statfs(mp, &mp->mnt_stat, p);
	vfs_unbusy(mp, p);
	return (0);
}

/*
 * VFS Operations.
 *
 * mount system call
 */
int
lfs_mount(mp, path, data, ndp, p)
	register struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *devvp;
	struct ufs_args args;
	struct ufsmount *ump;
	register struct lfs *fs;				/* LFS */
	u_int size;
	int error;
	mode_t accessmode;

	if ((error = copyin(data, (caddr_t)&args, sizeof (struct ufs_args))))
		return (error);

	/* Until LFS can do NFS right.		XXX */
	if (args.export.ex_flags & MNT_EXPORTED)
		return (EINVAL);

	/*
	 * If updating, check whether changing from read-only to
	 * read/write; if there is no device name, that's all we do.
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		ump = VFSTOUFS(mp);
		if (fs->lfs_ronly && (mp->mnt_flag & MNT_WANTRDWR)) {
			/*
			 * If upgrade to read-write by non-root, then verify
			 * that user has necessary permissions on the device.
			 */
			if (p->p_ucred->cr_uid != 0) {
				vn_lock(ump->um_devvp, LK_EXCLUSIVE | LK_RETRY,
				    p);
				if ((error = VOP_ACCESS(ump->um_devvp, VREAD | VWRITE, p->p_ucred, p))) {
					VOP_UNLOCK(ump->um_devvp, 0, p);
					return (error);
				}
				VOP_UNLOCK(ump->um_devvp, 0, p);
			}
			fs->lfs_ronly = 0;
		}
		if (args.fspec == 0) {
			/*
			 * Process export requests.
			 */
			return (vfs_export(mp, &ump->um_export, &args.export));
		}
	}
	/*
	 * Not an update, or updating the name: look up the name
	 * and verify that it refers to a sensible block device.
	 */
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, args.fspec, p);
	if ((error = namei(ndp)))
		return (error);
	devvp = ndp->ni_vp;
	if (devvp->v_type != VBLK) {
		vrele(devvp);
		return (ENOTBLK);
	}
	if (major(devvp->v_rdev) >= nblkdev) {
		vrele(devvp);
		return (ENXIO);
	}
	/*
	 * If mount by non-root, then verify that user has necessary
	 * permissions on the device.
	 */
	if (p->p_ucred->cr_uid != 0) {
		accessmode = VREAD;
		if ((mp->mnt_flag & MNT_RDONLY) == 0)
			accessmode |= VWRITE;
		vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, p);
		if ((error = VOP_ACCESS(devvp, accessmode, p->p_ucred, p))) {
			vput(devvp);
			return (error);
		}
		VOP_UNLOCK(devvp, 0, p);
	}
	if ((mp->mnt_flag & MNT_UPDATE) == 0)
		error = lfs_mountfs(devvp, mp, p);		/* LFS */
	else {
		if (devvp != ump->um_devvp)
			error = EINVAL;	/* needs translation */
		else
			vrele(devvp);
	}
	if (error) {
		vrele(devvp);
		return (error);
	}
	ump = VFSTOUFS(mp);
	fs = ump->um_lfs;					/* LFS */
#ifdef NOTLFS							/* LFS */
	(void) copyinstr(path, fs->fs_fsmnt, sizeof(fs->fs_fsmnt) - 1, &size);
	bzero(fs->fs_fsmnt + size, sizeof(fs->fs_fsmnt) - size);
	bcopy((caddr_t)fs->fs_fsmnt, (caddr_t)mp->mnt_stat.f_mntonname,
	    MNAMELEN);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1,
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void) ufs_statfs(mp, &mp->mnt_stat, p);
#else
	(void)copyinstr(path, fs->lfs_fsmnt, sizeof(fs->lfs_fsmnt) - 1, &size);
	bzero(fs->lfs_fsmnt + size, sizeof(fs->lfs_fsmnt) - size);
	bcopy((caddr_t)fs->lfs_fsmnt, (caddr_t)mp->mnt_stat.f_mntonname,
	    MNAMELEN);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1,
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void) lfs_statfs(mp, &mp->mnt_stat, p);
#endif
	return (0);
}

/*
 * Common code for mount and mountroot
 * LFS specific
 */
int
lfs_mountfs(devvp, mp, p)
	register struct vnode *devvp;
	struct mount *mp;
	struct proc *p;
{
	extern struct vnode *rootvp;
	register struct lfs *fs;
	register struct ufsmount *ump;
	struct vnode *vp;
	struct buf *bp;
	struct partinfo dpart;
	dev_t dev;
	int error, i, ronly, size;
	struct ucred *cred;

	cred = p ? p->p_ucred : NOCRED;
	/*
	 * Disallow multiple mounts of the same device.
	 * Disallow mounting of a device that is currently in use
	 * (except for root, which might share swap device for miniroot).
	 * Flush out any old buffers remaining from a previous use.
	 */
	if ((error = vfs_mountedon(devvp)))
		return (error);
	if (vcount(devvp) > 1 && devvp != rootvp)
		return (EBUSY);
	if ((error = vinvalbuf(devvp, V_SAVE, cred, p, 0, 0)))
		return (error);

	ronly = (mp->mnt_flag & MNT_RDONLY) != 0;
	if ((error = VOP_OPEN(devvp, ronly ? FREAD : FREAD|FWRITE, FSCRED, p)))
		return (error);

	if (VOP_IOCTL(devvp, DIOCGPART, (caddr_t)&dpart, FREAD, cred, p) != 0)
		size = DEV_BSIZE;
	else {
		size = dpart.disklab->d_secsize;
#ifdef NEVER_USED
		dpart.part->p_fstype = FS_LFS;
		dpart.part->p_fsize = fs->lfs_fsize;	/* frag size */
		dpart.part->p_frag = fs->lfs_frag;	/* frags per block */
		dpart.part->p_cpg = fs->lfs_segshift;	/* segment shift */
#endif
	}

	/* Don't free random space on error. */
	bp = NULL;
	ump = NULL;

	/* Read in the superblock. */
	if ((error = bread(devvp, LFS_LABELPAD / size, LFS_SBPAD, cred, &bp)))
		goto out;
	fs = (struct lfs *)bp->b_data;

	/* Check the basics. */
	if (fs->lfs_magic != LFS_MAGIC || fs->lfs_bsize > MAXBSIZE ||
	    fs->lfs_bsize < sizeof(struct lfs)) {
		error = EINVAL;		/* XXX needs translation */
		goto out;
	}

	/* Allocate the mount structure, copy the superblock into it. */
	ump = (struct ufsmount *)malloc(sizeof *ump, M_UFSMNT, M_WAITOK);
	fs = ump->um_lfs = malloc(sizeof(struct lfs), M_UFSMNT, M_WAITOK);
	bcopy(bp->b_data, fs, sizeof(struct lfs));
	if (sizeof(struct lfs) < LFS_SBPAD)			/* XXX why? */
		bp->b_flags |= B_INVAL;
	brelse(bp);
	bp = NULL;

	/* Set up the I/O information */
	fs->lfs_iocount = 0;

	/* Set up the ifile and lock aflags */
	fs->lfs_doifile = 0;
	fs->lfs_writer = 0;
	fs->lfs_dirops = 0;
	fs->lfs_seglock = 0;

	/* Set the file system readonly/modify bits. */
	fs->lfs_ronly = ronly;
	if (ronly == 0)
		fs->lfs_fmod = 1;

	/* Initialize the mount structure. */
	dev = devvp->v_rdev;
	mp->mnt_data = (qaddr_t)ump;
	mp->mnt_stat.f_fsid.val[0] = (long)dev;
	mp->mnt_stat.f_fsid.val[1] = lfs_mount_type;
	mp->mnt_maxsymlinklen = fs->lfs_maxsymlinklen;
	mp->mnt_flag |= MNT_LOCAL;
	ump->um_mountp = mp;
	ump->um_dev = dev;
	ump->um_devvp = devvp;
	ump->um_bptrtodb = 0;
	ump->um_seqinc = 1 << fs->lfs_fsbtodb;
	ump->um_nindir = fs->lfs_nindir;
	for (i = 0; i < MAXQUOTAS; i++)
		ump->um_quotas[i] = NULLVP;
	devvp->v_specflags |= SI_MOUNTEDON;

	/*
	 * We use the ifile vnode for almost every operation.  Instead of
	 * retrieving it from the hash table each time we retrieve it here,
	 * artificially increment the reference count and keep a pointer
	 * to it in the incore copy of the superblock.
	 */
	if ((error = VFS_VGET(mp, LFS_IFILE_INUM, &vp)))
		goto out;
	fs->lfs_ivnode = vp;
	VREF(vp);
	vput(vp);

	return (0);
out:
	if (bp)
		brelse(bp);
	(void)VOP_CLOSE(devvp, ronly ? FREAD : FREAD|FWRITE, cred, p);
	if (ump) {
		free(ump->um_lfs, M_UFSMNT);
		free(ump, M_UFSMNT);
		mp->mnt_data = (qaddr_t)0;
	}
	return (error);
}

/*
 * unmount system call
 */
int
lfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	extern int doforce;
	register struct ufsmount *ump;
	register struct lfs *fs;
	int i, error, flags, ronly;

	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	ump = VFSTOUFS(mp);
	fs = ump->um_lfs;
#ifdef QUOTA
	if (mp->mnt_flag & MNT_QUOTA) {
		if (error = vflush(mp, fs->lfs_ivnode, SKIPSYSTEM|flags))
			return (error);
		for (i = 0; i < MAXQUOTAS; i++) {
			if (ump->um_quotas[i] == NULLVP)
				continue;
			quotaoff(p, mp, i);
		}
		/*
		 * Here we fall through to vflush again to ensure
		 * that we have gotten rid of all the system vnodes.
		 */
	}
#endif
	if ((error = vflush(mp, fs->lfs_ivnode, flags)))
		return (error);
	fs->lfs_clean = 1;
	if ((error = VFS_SYNC(mp, 1, p->p_ucred, p)))
		return (error);
	if (LIST_FIRST(&fs->lfs_ivnode->v_dirtyblkhd))
		panic("lfs_unmount: still dirty blocks on ifile vnode\n");
	vrele(fs->lfs_ivnode);
	vgone(fs->lfs_ivnode);

	ronly = !fs->lfs_ronly;
	ump->um_devvp->v_specflags &= ~SI_MOUNTEDON;
	error = VOP_CLOSE(ump->um_devvp,
	    ronly ? FREAD : FREAD|FWRITE, NOCRED, p);
	vrele(ump->um_devvp);
	free(fs, M_UFSMNT);
	free(ump, M_UFSMNT);
	mp->mnt_data = (qaddr_t)0;
	mp->mnt_flag &= ~MNT_LOCAL;
	return (error);
}

/*
 * Get file system statistics.
 */
int
lfs_statfs(mp, sbp, p)
	struct mount *mp;
	register struct statfs *sbp;
	struct proc *p;
{
	register struct lfs *fs;
	register struct ufsmount *ump;

	ump = VFSTOUFS(mp);
	fs = ump->um_lfs;
	if (fs->lfs_magic != LFS_MAGIC)
		panic("lfs_statfs: magic");
	sbp->f_bsize = fs->lfs_fsize;
	sbp->f_iosize = fs->lfs_bsize;
	sbp->f_blocks = dbtofrags(fs,fs->lfs_dsize);
	sbp->f_bfree = dbtofrags(fs, fs->lfs_bfree);
	/*
	 * To compute the available space.  Subtract the minimum free
	 * from the total number of blocks in the file system.  Set avail
	 * to the smaller of this number and fs->lfs_bfree.
	 */
	sbp->f_bavail = fs->lfs_dsize * (100 - fs->lfs_minfree) / 100;
	sbp->f_bavail =
	    sbp->f_bavail > fs->lfs_bfree ? fs->lfs_bfree : sbp->f_bavail;
	sbp->f_bavail = dbtofrags(fs, sbp->f_bavail);
	sbp->f_files = fs->lfs_nfiles;
	sbp->f_ffree = sbp->f_bfree * INOPB(fs);
	if (sbp != &mp->mnt_stat) {
		sbp->f_type = mp->mnt_vfc->vfc_typenum;
		bcopy((caddr_t)mp->mnt_stat.f_mntonname,
			(caddr_t)&sbp->f_mntonname[0], MNAMELEN);
		bcopy((caddr_t)mp->mnt_stat.f_mntfromname,
			(caddr_t)&sbp->f_mntfromname[0], MNAMELEN);
	}
	return (0);
}

/*
 * Go through the disk queues to initiate sandbagged IO;
 * go through the inodes to write those that have been modified;
 * initiate the writing of the super block if it has been modified.
 *
 * Note: we are always called with the filesystem marked `MPBUSY'.
 */
int
lfs_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{
	int error;

	/* All syncs must be checkpoints until roll-forward is implemented. */
	error = lfs_segwrite(mp, SEGM_CKP | (waitfor ? SEGM_SYNC : 0));
#ifdef QUOTA
	qsync(mp);
#endif
	return (error);
}

/*
 * Look up an LFS dinode number to find its incore vnode.  If not already
 * in core, read it in from the specified device.  Return the inode locked.
 * Detection and handling of mount points must be done by the calling routine.
 */
int
lfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	register struct lfs *fs;
	struct ufs1_dinode *dip1;
	struct ufs2_dinode *dip2;
	register struct inode *ip;
	struct buf *bp;
	struct ifile *ifp;
	struct vnode *vp;
	struct ufsmount *ump;
	struct vnodeops *fifoops;
	ufs1_daddr_t daddr;
	dev_t dev;
	int error, retries;

#ifdef FIFO
	fifoops = &lfs_fifoops
#else
	fifoops = NULL;
#endif

	ump = VFSTOUFS(mp);
	dev = ump->um_dev;
	if ((*vpp = ufs_ihashget(dev, ino)) != NULL)
		return (0);

	/* Translate the inode number to a disk address. */
	fs = ump->um_lfs;
	if (ino == LFS_IFILE_INUM)
		daddr = fs->lfs_idaddr;
	else {
		LFS_IENTRY(ifp, fs, ino, bp);
		daddr = ifp->if_daddr;
		brelse(bp);
		if (daddr == LFS_UNUSED_DADDR)
			return (ENOENT);
	}

	/* Allocate new vnode/inode. */
	if ((error = lfs_vcreate(mp, ino, &vp))) {
		*vpp = NULL;
		return (error);
	}

	/*
	 * Put it onto its hash chain and lock it so that other requests for
	 * this inode will block if they arrive while we are sleeping waiting
	 * for old data structures to be purged or for the contents of the
	 * disk portion of this inode to be read.
	 */
	ip = VTOI(vp);
	ufs_ihashins(ip);

	/*
	 * XXX
	 * This may not need to be here, logically it should go down with
	 * the i_devvp initialization.
	 * Ask Kirk.
	 */
	ip->i_lfs = ump->um_lfs;

	/* Read in the disk contents for the inode, copy into the inode. */
	retries = 0;
again:
	if ((error = bread(ump->um_devvp, daddr, (int)fs->lfs_bsize, NOCRED, &bp))) {
		/*
		 * The inode does not contain anything useful, so it would
		 * be misleading to leave it on its hash chain. With mode
		 * still zero, it will be unlinked and returned to the free
		 * list by vput().
		 */
		vput(vp);
		brelse(bp);
		*vpp = NULL;
		return (error);
	}

	if (UFS1) {
		dip1 = lfs1_ifind(fs, ino, bp);
		if (dip1 != NULL) {
			*ip->i_din.ffs1_din = *dip1;
			goto end;
		} else {
			error = lfs1_vget_debug(ip, fs, dip1, bp, ino, retries);
			if (error != 0) {
				goto again;
			}
		}
	} else {
		dip2 = lfs2_ifind(fs, ino, bp);
		if (dip2 != NULL) {
			*ip->i_din.ffs2_din = *dip2;
			goto end;
		} else {
			error = lfs2_vget_debug(ip, fs, dip2, bp, ino, retries);
			if (error != 0) {
				goto again;
			}
		}
	}

end:
	brelse(bp);

	/*
	 * Initialize the vnode from the inode, check for aliases.  In all
	 * cases re-init ip, the underlying vnode/inode may have changed.
	 */
	if ((error = ufs_vinit(mp, &lfs_specops, fifoops, &vp))) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->um_devvp;
	VREF(ip->i_devvp);
	*vpp = vp;
	return (0);
}

/*
 * File handle to vnode
 *
 * Have to be really careful about stale file handles:
 * - check that the inode number is valid
 * - call lfs_vget() to get the locked inode
 * - check for an unallocated inode (i_mode == 0)
 * - check that the given client host has export rights and return
 *   those rights via. exflagsp and credanonp
 *
 * XXX
 * use ifile to see if inode is allocated instead of reading off disk
 * what is the relationship between my generational number and the NFS
 * generational number.
 */
int
lfs_fhtovp(mp, fhp, nam, vpp, exflagsp, credanonp)
	register struct mount *mp;
	struct fid *fhp;
	struct mbuf *nam;
	struct vnode **vpp;
	int *exflagsp;
	struct ucred **credanonp;
{
	register struct ufid *ufhp;

	ufhp = (struct ufid *)fhp;
	if (ufhp->ufid_ino < ROOTINO)
		return (ESTALE);
	return (ufs_check_export(mp, ufhp, nam, vpp, exflagsp, credanonp));
}

/*
 * Vnode pointer to File handle
 */
/* ARGSUSED */
int
lfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	register struct inode *ip;
	register struct ufid *ufhp;

	ip = VTOI(vp);
	ufhp = (struct ufid *)fhp;
	ufhp->ufid_len = sizeof(struct ufid);
	ufhp->ufid_ino = ip->i_number;
	ufhp->ufid_gen = ip->i_gen;
	return (0);
}

/*
 * Initialize the filesystem, most work done by ufs_init.
 */
int lfs_mount_type;

int
lfs_init(vfsp)
	struct vfsconf *vfsp;
{
	lfs_mount_type = vfsp->vfc_typenum;
	return (ufs_init(vfsp));
}

int
lfs1_vget_debug(ip, fs, dip, bp, ino, retries)
	struct inode *ip;
	struct lfs *fs;
	struct ufs1_dinode *dip;
	struct buf *bp;
	ino_t ino;
	int retries;
{
	if (dip == NULL) {
		/* Assume write has not completed yet; try again */
		bp->b_flags |= B_INVAL;
		brelse(bp);
		++retries;
		if (retries > LFS_IFIND_RETRIES) {
#ifdef DEBUG
			/* If the seglock is held look at the bpp to see
			 what is there anyway */
			if (fs->lfs_seglock > 0) {
				struct buf **bpp;
				struct ufs1_dinode *dp;
				int i;

				for (bpp = fs->lfs_sp->bpp; bpp != fs->lfs_sp->cbpp; ++bpp) {
					if ((*bpp)->b_vp == fs->lfs_ivnode && bpp != fs->lfs_sp->bpp) {
						/* Inode block */
						printf("block 0x%" PRIx64 ": ", (*bpp)->b_blkno);
						dp = (struct ufs1_dinode*) (*bpp)->b_data;
						for (i = 0; i < INOPB(fs); i++) {
							if (dp[i].di_u.inumber) {
								printf("%d ", dp[i].di_u.inumber);
							}
						}
						printf("\n");

					}
				}
			}
#endif
			panic("lfs_vget: dinode not found");
		}
		printf("lfs_vget: dinode %d not found, retrying...\n", ino);
		(void) tsleep(&fs->lfs_iocount, PRIBIO + 1, "lfs1 ifind", 1);
		return (1);
	}
	return (0);
}

int
lfs2_vget_debug(ip, fs, dip, bp, ino, retries)
	struct inode *ip;
	struct lfs *fs;
	struct ufs2_dinode *dip;
	struct buf *bp;
	ino_t ino;
	int retries;
{
	if (dip == NULL) {
		/* Assume write has not completed yet; try again */
		bp->b_flags |= B_INVAL;
		brelse(bp);
		++retries;
		if (retries > LFS_IFIND_RETRIES) {
#ifdef DEBUG
			/* If the seglock is held look at the bpp to see
			 what is there anyway */
			if (fs->lfs_seglock > 0) {
				struct buf **bpp;
				struct ufs2_dinode *dp;
				int i;

				for (bpp = fs->lfs_sp->bpp; bpp != fs->lfs_sp->cbpp; ++bpp) {
					if ((*bpp)->b_vp == fs->lfs_ivnode && bpp != fs->lfs_sp->bpp) {
						/* Inode block */
						printf("block 0x%" PRIx64 ": ", (*bpp)->b_blkno);
						dp = (struct ufs2_dinode*) (*bpp)->b_data;
						for (i = 0; i < INOPB(fs); i++) {
							if (dp[i].di_u.inumber) {
								printf("%d ", dp[i].di_u.inumber);
							}
						}
						printf("\n");

					}
				}
			}
#endif
			panic("lfs_vget: dinode not found");
		}
		printf("lfs_vget: dinode %d not found, retrying...\n", ino);
		(void) tsleep(&fs->lfs_iocount, PRIBIO + 1, "lfs2 ifind", 1);
		return (1);
	}
	return (0);
}
