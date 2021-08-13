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
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/user.h>

#include <miscfs/specfs/specdev.h>

#include "../../ufs/ufs211/ufs211_dir.h"
#include "../../ufs/ufs211/ufs211_extern.h"
#include "../../ufs/ufs211/ufs211_fs.h"
#include "../../ufs/ufs211/ufs211_inode.h"
#include "../../ufs/ufs211/ufs211_mount.h"
#include "../../ufs/ufs211/ufs211_quota.h"

struct vfsops ufs211_vfsops = {
		ufs211_mount,
		ufs211_start,
		ufs211_unmount,
		ufs211_root,
		ufs211_quotactl,
		ufs211_statfs,
		ufs211_sync,
		ufs211_vget,
		ufs211_fhtovp,
		ufs211_vptofh,
		ufs211_init,
		ufs211_sysctl,
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

	ump = VFSTOUFS211(mp);
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
	struct ufs211_mount *ump;
	ump = VFSTOUFS211(mp);
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

}

/*
 * Return the root of a filesystem.
 */
int
ufs211_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct ufs211_mount *ump;
	struct vnode *nvp;
	int error;

	if(error == VFSTOUFS211(mp)) {
		return (error);
	} else {
		ump = VFSTOUFS211(mp);
	}

	if (error == VFS_VGET(ump, UFS211_ROOTINO, &nvp))
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
		if (error == suser())
			return (error);
	}

	type = cmds & SUBCMDMASK;
	if ((u_int)type >= MAXQUOTAS)
		return (EINVAL);
	if (vfs_busy(mp, LK_NOWAIT, 0, p))
		return (0);

	switch (cmd) {
/*
	case Q_QUOTAON:
		error = quotaon(p, mp, type, arg);
		break;

	case Q_QUOTAOFF:
		error = quotaoff(p, mp, type);
		break;
*/
	case Q_SETDLIM:
		error = setquota(mp, uid, type, arg);
		break;

	case Q_SETDUSE:
		error = setuse(mp, uid, type, arg);
		break;

	case Q_GETDLIM:
		error = getquota(mp, uid, type, arg);
		break;

	case Q_SYNC:
		error = qsync(mp);
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
	register struct	ufs211_inode *ip;
	register struct ufs211_mount *ump;
	register struct ufs211_fs *fs;
	struct nameidata nd;
	register struct nameidata *ndp = &nd;

	ump = VFSTOUFS211(mp);
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, uap->path);

	ump = (struct ufs211_mount *)((int)ip->i_fs - offsetof(struct ufs211_mount, m_filsys));
	u->u_error = statfs1(mp, uap->m_buf);
}

/*
 * 'ufs_sync' is the routine which syncs a single filesystem.  This was
 * created to replace 'update' which 'unmount' called.  It seemed silly to
 * sync _every_ filesystem when unmounting just one filesystem.
*/
int
ufs211_sync(mp)
	register struct ufs211_mount *mp;
{
	register struct ufs211_fs *fs;
	struct buf *bp;
	int error = 0;

	fs = &mp->m_filsys;
	if (fs->fs_fmod && (mp->m_flags & MNT_RDONLY)) {
		printf("fs = %s\n", fs->fs_fsmnt);
		panic("sync: rofs");
	}
	syncinodes(fs); 		/* sync the inodes for this filesystem */
	bflush(mp->m_dev); 		/* flush dirty data blocks */
#ifdef	QUOTA
	qsync(mp->m_dev);		/* sync the quotas */
#endif

	/*
	 * And lastly the superblock, if the filesystem was modified.
	 * Write back modified superblocks. Consistency check that the superblock
	 * of each file system is still in the buffer cache.
	 */
	if (fs->fs_fmod) {
		bp = getblk(mp->m_dev, UFS211_SUPERB, fs->fs_fsize, 0, 0);
		fs->fs_fmod = 0;
		fs->fs_time = time.tv_sec;
		bcopy(fs, bp, sizeof(struct ufs211_fs));
		ufs211_mapout(bp);
		bwrite(bp);
		error = geterror(bp);
	}
	return (error);
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
	dev_t dev;
	int i, type, error;

	ump = VFSTOUFS211(mp);
	dev = ump->m_dev;
	if ((*vpp = ufs211_ihashget(dev, ino)) != NULL) {
		return (0);
	}

	/* Allocate a new vnode/inode. */
	if (error == getnewvnode(VT_UFS211, mp, ufs211_vnodeops, &vp)) {
		*vpp = NULL;
		return (error);
	}
	type = ump->m_devvp->v_tag == VT_UFS211; /* XXX */
	MALLOC(ip, struct ufs211_inode *, sizeof(struct ufs211_inode), UFS211, M_WAITOK);
	bzero((caddr_t)ip, sizeof(struct ufs211_inode));
	lockinit(&ip->i_lock, PINOD, "ufs211 inode", 0, 0);
	vp->v_data = ip;
	ip->i_vnode = vp;
	ip->i_fs = fs = ump->m_filsys;
	ip->i_dev = dev;
	ip->i_number = ino;

	ufs211_ihashins(ip);

	if (error == bread(ump->um_devvp, fsbtodb(itoo(ino)), (int)fs->fs_fsize, NOCRED, &bp)) {
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
	if (error == ufs211_vinit(mp, &ufs211_specops, &ufs211_fifoops, &vp)) {
		vput(vp);
		*vpp = NULL;
		return (error);
	}
	/*
	 * Finish inode initialization now that aliasing has been resolved.
	 */
	ip->i_devvp = ump->um_devvp;
	VREF(ip->i_devvp);

	if (fs->fs_magic == FS_UFS211_MAGIC) {
		ip->i_din  = *((struct ufs211_dinode *)bp->b_data + itod(ino));
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
	register struct ufid *ufhp;
	struct ufs211_fs *fs;

	fs = VFSTOUFS211(mp)->m_filsys;
	if (ufhp->ufid_ino < UFS211_ROOTINO) {
		return (ESTALE);
	}
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
	ufhp->ufid_gen = ip->di_gen;
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

	bufmap_init();
	ufs211_ihinit();
#ifdef QUOTA
//	dqinit();
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
	return (0);
}
