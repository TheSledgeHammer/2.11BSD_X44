/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)ufs211_vfsops.c	1.00
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/file.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <ufs/ufs211/ufs211_dir.h>

#include <ufs/ufs211/ufs211_quota.h>
#include <ufs/ufs211/ufs211_inode.h>
#include <ufs/ufs211/ufs211_fs.h>
#include <ufs/ufs211/ufs211_extern.h>
#include <ufs/ufs211/ufs211_mount.h>
#include <miscfs/specfs/specdev.h>

struct vfsops ufs211_vfsops = {
		.vfs_mount = ufs211_mount,
		.vfs_start = ufs211_start,
		.vfs_unmount = ufs211_unmount,
		.vfs_root = ufs211_root,
		.vfs_quotactl = ufs211_quotactl,
		.vfs_statfs = ufs211_statfs,
		.vfs_sync = ufs211_sync,
		.vfs_vget = ufs211_vget,
		.vfs_fhtovp = ufs211_fhtovp,
		.vfs_vptofh = ufs211_vptofh,
		.vfs_init = ufs211_init,
		.vfs_sysctl = ufs211_sysctl,
};

/*
 * VFS Operations.
 *
 * mount system call
 */
int
ufs211_mount(mp, path, data, ndp, p)
	register struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	struct vnode *devvp;
	struct ufs211_args args;
	struct ufs211_mount *ump;
	register struct ufs211_fs *fs;
	u_int size;
	int error, flags;
	mode_t accessmode;

	if ((error = copyin(data, (caddr_t)&args, sizeof (struct ufs211_args)))) {
		return (error);
	}
	if (mp->mnt_flag & MNT_UPDATE) {
		ump = VFSTOUFS211(mp);
		fs = ump->m_filsys;
		if (fs->fs_ronly == 0 && (mp->mnt_flag & MNT_RDONLY)) {
			flags = WRITECLOSE;
			if (mp->mnt_flag & MNT_FORCE)
				flags |= FORCECLOSE;
			if ((error = ufs211_flushfiles(mp, flags, p))) {
				return (error);
			}
//			fs->fs_clean = 1;
			fs->fs_ronly = 1;
		}
		if (fs->fs_ronly && (mp->mnt_flag & MNT_WANTRDWR)) {
			/*
			 * If upgrade to read-write by non-root, then verify
			 * that user has necessary permissions on the device.
			 */
			if (p->p_ucred->cr_uid != 0) {
				devvp = ump->m_devvp;
				vn_lock(devvp, LK_EXCLUSIVE | LK_RETRY, p);
				if ((error = VOP_ACCESS(devvp, VREAD | VWRITE, p->p_ucred, p))) {
					VOP_UNLOCK(devvp, 0, p);
					return (error);
				}
				VOP_UNLOCK(devvp, 0, p);
			}
			fs->fs_ronly = 0;
//			fs->fs_clean = 0;
		}
		if (args.fspec == 0) {
			/*
			 * Process export requests.
			 */
			return (vfs_export(mp, &ump->m_export, &args.export));
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
	if ((mp->mnt_flag & MNT_UPDATE) == 0) {
		error = ufs211_mountfs(devvp, mp, p);
	} else {
		if (devvp != ump->m_devvp) {
			error = EINVAL; /* needs translation */
		} else {
			vrele(devvp);
		}
	}
	if (error) {
		vrele(devvp);
		return (error);
	}
	ump = VFSTOUFS211(mp);
	fs = ump->m_filsys;
	(void) copyinstr(path, fs->fs_fsmnt, sizeof(fs->fs_fsmnt) - 1, &size);
	bzero(fs->fs_fsmnt + size, sizeof(fs->fs_fsmnt) - size);
	bcopy((caddr_t) fs->fs_fsmnt, (caddr_t) mp->mnt_stat.f_mntonname,
	MNAMELEN);
	(void) copyinstr(args.fspec, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
	(void) ufs211_statfs(mp, &mp->mnt_stat, p);
	return (0);
}

int
ufs211_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	register struct ufs211_mount *ump;
	register struct ufs211_fs *fs;
	int error, flags;

	ump = VFSTOUFS211(mp);
	flags = 0;
	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;
	if (error = ufs211_flushfiles(mp, flags, p))
		return (error);
	ump = VFSTOUFS211(mp);
	fs = ump->m_filsys;
	/*
	if (fs->fs_ronly == 0) {
		fs->fs_clean = 1;
		if (error = ffs_sbupdate(ump, MNT_WAIT)) {
			fs->fs_clean = 0;
			return (error);
		}
	}
	*/
	ump->m_devvp->v_specflags &= ~SI_MOUNTEDON;
	error = VOP_CLOSE(ump->m_devvp, fs->fs_ronly ? FREAD : FREAD|FWRITE, NOCRED, p);
	vrele(ump->m_devvp);
	free(fs, M_UFSMNT);
	free(ump, M_UFSMNT);
	mp->mnt_data = (qaddr_t)0;
	return (error);
}

void
mount_updname(mp, fsmnt, on, from, lenon, lenfrom)
	struct mount *mp;
	char *fsmnt, *on, *from;
	int	 lenon, lenfrom;
{
	register struct	xmount	*xmp;

	bzero(fsmnt, sizeof(fsmnt));
	bcopy(on, fsmnt, sizeof(fsmnt) - 1);
	xmp = (struct xmount *)mp->mnt_xmnt;
	bzero(xmp, sizeof (struct xmount));
	bcopy(on, xmp->xm_mnton, lenon);
	bcopy(from, xmp->xm_mntfrom, lenfrom);
}

/*
 * Common code for mount and mountroot
 */
int
ufs211_mountfs(devvp, mp, p)
	register struct vnode *devvp;
	struct mount *mp;
	struct proc *p;
{
	register struct ufs211_mount *ump;
	struct buf *bp;
	register struct ufs211_fs *fs;
	dev_t dev;
	struct partinfo dpart;
	int error, i, blks, size, ronly;
	struct ucred *cred;
	extern struct vnode *rootvp;

	dev = devvp->v_rdev;
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
	if ((error = VOP_OPEN(devvp, ronly ? FREAD : FREAD | FWRITE, FSCRED, p)))
		return (error);
	if (VOP_IOCTL(devvp, DIOCGPART, (caddr_t)&dpart, FREAD, cred, p) != 0)
		size = DEV_BSIZE;
	else
		size = dpart.disklab->d_secsize;

	bp = NULL;
	ump = NULL;
	if ((error = bread(devvp, (daddr_t) (UFS211_SBLOCK / size), UFS211_SBSIZE, cred, &bp))) {
		goto out;
	}
	fs = (struct ufs211_fs *) bp->b_data;
	if (fs->fs_magic != FS_UFS211_MAGIC ) {
		error = EINVAL; /* XXX needs translation */
		goto out;
	}
	ump = malloc(sizeof(*ump), M_UFS211, M_WAITOK);
	bzero((caddr_t) ump, sizeof(*ump));
	ump->m_filsys = malloc(sizeof(struct ufs211_fs), M_UFS211, M_WAITOK);
	bcopy(bp->b_data, ump->m_filsys, sizeof(struct ufs211_fs));
	brelse(bp);
	bp = NULL;
	fs = ump->m_filsys;
	fs->fs_ronly = (ronly != 0);
	if (ronly == 0) {
		fs->fs_fmod = 1;
	}
	fs->fs_ilock = 0;
	fs->fs_flock = 0;
	fs->fs_nbehind = 0;
	fs->fs_lasti = 1;
	//fs->fs_flags = flags;
	mp->mnt_data = (qaddr_t) ump;
	mp->mnt_stat.f_fsid.val[0] = (long) dev;
	mp->mnt_stat.f_fsid.val[1] = mp->mnt_vfc->vfc_typenum;
	//mp->mnt_maxsymlinklen = fs->fs_maxsymlinklen;
	ump->m_mountp = mp;
	ump->m_dev = dev;
	ump->m_devvp = devvp;
	for (i = 0; i < MAXQUOTAS; i++) {
		ump->m_quotas[i] = NULLVP;
	}
	devvp->v_specflags |= SI_MOUNTEDON;
    return (0);

out:
	if (bp)
		brelse(bp);
	(void) VOP_CLOSE(devvp, ronly ? FREAD : FREAD | FWRITE, cred, p);
	if (ump) {
		free(ump->m_filsys, M_UFS211);
		free(ump, M_UFS211);
		mp->mnt_data = (qaddr_t) 0;
	}
	return (error);
}

/*
 * Flush out all the files in a filesystem.
 */
int
ufs211_flushfiles(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	register struct ufs211_mount *ump;
	int i, error;

	ump = VFSTOUFS211(mp);
#ifdef QUOTA
	if (mp->mnt_flag & MNT_QUOTA) {
		if ((error = vflush(mp, NULLVP, SKIPSYSTEM | flags))) {
			return (error);
		}
		for (i = 0; i < MAXQUOTAS; i++) {
			if (ump->m_quotas[i] == NULLVP) {
				continue;
			}
			ufs211_quotaoff(p, mp, i);
		}
	}
#endif
	error = vflush(mp, NULLVP, flags);
	return (error);
}

/*
 * Make a filesystem operational.
 * Nothing to do at the moment.
 */
/* ARGSUSED */
int
ufs211_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	return (0);
}

/*
 * Return the root of a filesystem.
 */
int
ufs211_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode *nvp;
	int error;

	if ((error = VFS_VGET(mp, (ino_t)UFS211_ROOTINO, &nvp)))
		return (error);
	*vpp = nvp;
	return (0);
}

/*
 * Do operations associated with quotas
 */
int
ufs211_quotactl(mp, cmds, uid, arg, p)
	struct mount *mp;
	int cmds;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
	struct ufs211_mount *ump;
	int cmd, type, error;

	ump = VFSTOUFS211(mp);
#ifndef QUOTA
	return (EOPNOTSUPP);
#else
	if (uid == -1)
		uid = p->p_cred->p_ruid;
	cmd = cmds >> SUBCMDSHIFT;

	switch (cmd) {
	case Q_SYNC:
		break;
	case Q_GETDLIM:
		if (uid == p->p_cred->p_ruid)
			break;

		break;
		/* fall through */
	default:
		if (error = suser())
			return (error);
	}

	type = cmds & SUBCMDMASK;
	if ((u_int)type >= MAXQUOTAS)
		return (EINVAL);
	if (vfs_busy(mp, LK_NOWAIT, 0, p))
		return (0);

	switch (cmd) {

	case Q_QUOTAON:
		error = ufs211_quotaon(p, mp, type, arg);
		break;

	case Q_QUOTAOFF:
		error = ufs211_quotaoff(p, mp, type);
		break;

	case Q_SETQUOTA:
		error = ufs211_setquota(mp, uid, type, arg);
		break;

	case Q_SETUSE:
		error = ufs211_setuse(mp, uid, type, arg);
		break;

	case Q_GETQUOTA:
		error = ufs211_getquota(mp, uid, type, arg);
		break;

	case Q_SETDLIM:
		error = ufs211_setquota(mp, uid, type, arg);
		break;

	case Q_SETDUSE:
		error = ufs211_setduse(mp, uid, type, arg);
		break;

	case Q_GETDLIM:
		error = ufs211_getquota(mp, uid, type, arg);
		break;

	case Q_SETWARN:
		error = ufs211_setwarn(mp, uid, type, arg);
		break;

	case Q_DOWARN:
		error = ufs211_dowarn(mp, uid, type);
		break;

	case Q_SYNC:
		error = ufs211_qsync(mp);
		break;

	default:
		error = EINVAL;
		break;
	}
	vfs_unbusy(mp, p);
	return (error);
#endif
}

int
ufs211_statfs(mp, sbp, p)
	struct mount *mp;
	register struct statfs *sbp;
	struct proc *p;
{
	register struct ufs211_mount *ump;
	register struct ufs211_fs *fs;
	register struct ufs211_fblk *fbp;
	struct buf *bp;

	ump = VFSTOUFS211(mp);
	fs = ump->m_filsys;
	if (fs->fs_magic != FS_UFS211_MAGIC) {
		panic("ufs211_statfs");
	}
	ufs211_mapin(fbp);
	fbp = (struct ufs211_fblk *)bp;
	sbp->f_bsize = fs->fs_fsize;
	sbp->f_iosize = MAXBSIZE;
	sbp->f_blocks = fs->fs_fsize - fs->fs_isize;
	sbp->f_bfree = fs->fs_tfree;
	sbp->f_bavail = fs->fs_tfree;
	sbp->f_files = (fs->fs_isize - 2) * INOPB;
	sbp->f_ffree = fs->fs_tinode;
	*((struct ufs211_fblk *) &fs->fs_nfree) = *fbp;
	ufs211_mapout(bp);
	if (sbp != &mp->mnt_stat) {
		sbp->f_type = mp->mnt_vfc->vfc_typenum;
		bcopy((caddr_t) mp->mnt_stat.f_mntonname,
				(caddr_t) & sbp->f_mntonname[0], MNAMELEN);
		bcopy((caddr_t) mp->mnt_stat.f_mntfromname,
				(caddr_t) & sbp->f_mntfromname[0], MNAMELEN);
	}
	return (0);
}

/*
 * 'ufs_sync' is the routine which syncs a single filesystem.  This was
 * created to replace 'update' which 'unmount' called.  It seemed silly to
 * sync _every_ filesystem when unmounting just one filesystem.
*/
int
ufs211_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{
	struct vnode *nvp, *vp;
	struct ufs211_inode *ip;
	struct ufs211_mount *ump;
	struct ufs211_fs *fs;
	struct buf *bp;
	int error, allerror = 0;

	ump = VFSTOUFS211(mp);

	fs = ump->m_filsys;
	if (fs->fs_fmod != 0 && fs->fs_ronly != 0) {
		printf("fs = %s\n", fs->fs_fsmnt);
		panic("sync: rofs");
	}

	/*
	 * Write back each (modified) inode.
	 */
	simple_lock(&mntvnode_slock);
loop:
	for (vp = LIST_FIRST(&mp->mnt_vnodelist); vp != NULL; vp = nvp) {
		/*
		 * If the vnode that we are about to sync is no longer
		 * associated with this mount point, start over.
		 */
		if (vp->v_mount != mp)
			goto loop;
		simple_lock(&vp->v_interlock);
		nvp = LIST_NEXT(vp, v_mntvnodes);
		ip = UFS211_VTOI(vp);
		if ((ip->i_flag & (UFS211_IACC | UFS211_ICHG | UFS211_IMOD | UFS211_IUPD))
				== 0 && LIST_FIRST(&vp->v_dirtyblkhd) == NULL) {
			simple_unlock(&vp->v_interlock);
			continue;
		}
		simple_unlock(&mntvnode_slock);
		error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK, p);
		if (error) {
			simple_lock(&mntvnode_slock);
			if (error == ENOENT)
				goto loop;
			continue;
		}
		if ((error = VOP_FSYNC(vp, cred, waitfor, 0, p)))
			allerror = error;
		VOP_UNLOCK(vp, 0, p);
		vrele(vp);
		simple_lock(&mntvnode_slock);
	}
	simple_unlock(&mntvnode_slock);
	/*
	 * Force stale file system control information to be flushed.
	 */
	if ((error = VOP_FSYNC(ump->m_devvp, cred, waitfor, 0, p))) {
		allerror = error;
	}

	bp = getblk(ump->m_devvp, UFS211_SUPERB, fs->fs_fsize, 0, 0);
	if(bp) {
		bflush(ump->m_devvp, bp->b_blkno, ump->m_dev); 		/* flush dirty data blocks */
	}
#ifdef	QUOTA
	ufs211_qsync(mp);		/* sync the quotas */
#endif

	/*
	 * And lastly the superblock, if the filesystem was modified.
	 * Write back modified superblocks. Consistency check that the superblock
	 * of each file system is still in the buffer cache.
	 */
	if (fs->fs_fmod) {
		fs->fs_fmod = 0;
		fs->fs_time = time.tv_sec;
		bcopy(fs, bp, sizeof(struct ufs211_fs));
		ufs211_mapout(bp);
		bwrite(bp);
		if(error == geterror(bp)) {
			allerror = error;
		}
	}
	return (allerror);
}

int
ufs211_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	struct proc *p = curproc; /* XXX */
	struct ufs211_fs *fs;
	struct ufs211_inode *ip;
	struct ufs211_mount *ump;
	struct buf *bp;
	struct vnode *vp;
    struct vnodeops *fifoops;
	dev_t dev;
	int i, type, error;
#ifdef FIFO
	fifoops = &ufs211_fifoops;
#else
	fifoops = NULL;
#endif

	ump = VFSTOUFS211(mp);
	dev = ump->m_dev;
	if ((*vpp = ufs211_ihashget(dev, ino)) != NULL) {
		return (0);
	}

	/* Allocate a new vnode/inode. */
	if ((error = getnewvnode(VT_UFS211, mp, &ufs211_vnodeops, &vp))) {
		*vpp = NULL;
		return (error);
	}
	type = ump->m_devvp->v_tag == VT_UFS211; /* XXX */
	MALLOC(ip, struct ufs211_inode *, sizeof(struct ufs211_inode), M_UFS211, M_WAITOK);
	bzero((caddr_t)ip, sizeof(struct ufs211_inode));
	lockinit(&ip->i_lock, PINOD, "ufs211 inode", 0, 0);
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_fs = fs = ump->m_filsys;
	ip->i_dev = dev;
	ip->i_number = ino;

	ufs211_ihashins(ip);

	if ((error = bread(ump->m_devvp, fsbtodb(itoo(ino)), (int)fs->fs_fsize, NOCRED, &bp))) {
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
	brelse(bp);

	/*
	 * Initialize the vnode from the inode, check for aliases.
	 * Note that the underlying vnode may have changed.
	 */
	if ((error = ufs211_vinit(mp, &ufs211_specops, fifoops, &vp))) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->m_devvp;
	VREF(ip->i_devvp);

	if (fs->fs_magic == FS_UFS211_MAGIC) {
		ip->i_uid = ip->i_din->di_uid;
		ip->i_gid = ip->i_din->di_gid;
		//ip->i_din  = *((struct ufs211_dinode *)bp->b_data + itod(ino));
	}
	*vpp = vp;
	return (0);
}

int
ufs211_fhtovp(mp, fhp, nam, vpp, exflagsp, credanonp)
	register struct mount *mp;
	struct fid *fhp;
	struct mbuf *nam;
	struct vnode **vpp;
	int *exflagsp;
	struct ucred **credanonp;
{
	struct ufs211_inode *ip;
    register struct ufid *ufhp;
    struct ufs211_fs *fs;
	struct vnode *nvp;
	int error;

    ufhp = (struct ufid *)fhp;

	error = VFS_VGET(mp, ufhp->ufid_ino, &nvp);
	if (error) {
		*vpp = NULLVP;
		return (error);
	}
	ip = UFS211_VTOI(nvp);
	if (ip->i_mode == 0 || ufhp->ufid_ino < UFS211_ROOTINO) {
		vput(nvp);
		*vpp = NULLVP;
		return (ESTALE);
	}
	*vpp = nvp;
	return (0);
}

int
ufs211_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	register struct ufs211_inode *ip;
	register struct ufid *ufhp;

	ip = UFS211_VTOI(vp);
	ufhp = (struct ufid *)fhp;
	ufhp->ufid_len = sizeof(struct ufid);
	ufhp->ufid_ino = ip->i_number;
	ufhp->ufid_gen = ip->i_din->di_gen;
	return (0);
}

/*
 * Initial UFS211 filesystems, done only once.
 */
int
ufs211_init(vfsp)
	struct vfsconf *vfsp;
{
	static int done;

	if (done)
		return (0);
	done = 1;

	ufs211_bufmap_init();
	ufs211_ihinit();
#ifdef QUOTA
	ufs211_quotainit();
#endif
	return (0);
}

/*
 * ufs211 filesystem related variables.
 */
int
ufs211_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR); /* overloaded */
	return (EOPNOTSUPP);
}
