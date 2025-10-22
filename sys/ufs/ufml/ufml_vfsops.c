/*
 * Copyright (c) 1992, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)null_vfsops.c	8.7 (Berkeley) 5/14/95
 *
 * @(#)lofs_vfsops.c	1.2 (Berkeley) 6/18/92
 * $Id: lofs_vfsops.c,v 1.9 1992/05/30 10:26:24 jsp Exp jsp $
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>

#include <ufs/ufml/ufml.h>

/*
 * Mount ufml layer
 */
int
ufmlfs_mount(mp, path, data, ndp, p)
	struct mount *mp;
	char *path;
	caddr_t data;
	struct nameidata *ndp;
	struct proc *p;
{
	int error = 0;
	struct ufml_args args;
	struct vnode *lowerrootvp, *vp;
	struct vnode *ufmlm_rootvp;
	struct ufml_mount *xmp;
	u_int size;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufmlfs_mount(mp = %x)\n", mp);
#endif

	/*
	 * Update is a no-op
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		return (EOPNOTSUPP);
		/* return VFS_MOUNT(MOUNTTONULLMOUNT(mp)->nullm_vfs, path, data, ndp, p);*/
	}

	/*
	 * Get argument
	 */
	if ((error = copyin(data, (caddr_t)&args, sizeof(struct ufml_args))))
		return (error);

	/*
	 * Find lower node
	 */
	NDINIT(ndp, LOOKUP, FOLLOW|WANTPARENT|LOCKLEAF,
		UIO_USERSPACE, args.target, p);
	if ((error = namei(ndp)))
		return (error);

	/*
	 * Sanity check on lower vnode
	 */
	lowerrootvp = ndp->ni_vp;

	vrele(ndp->ni_dvp);
	ndp->ni_dvp = NULL;

	xmp = (struct ufml_mount *) malloc(sizeof(struct ufml_mount), M_UFSMNT, M_WAITOK);	/* XXX */

	/*
	 * Save reference to underlying FS
	 */
	xmp->ufmlm_vfs = lowerrootvp->v_mount;

	/*
	 * Save reference.  Each mount also holds
	 * a reference on the root vnode.
	 */
	error = ufml_node_create(mp, lowerrootvp, &vp);
	/*
	 * Unlock the node (either the lower or the alias)
	 */
	VOP_UNLOCK(vp, 0, p);
	/*
	 * Make sure the node alias worked
	 */
	if (error) {
		vrele(lowerrootvp);
		free(xmp, M_UFSMNT);	/* XXX */
		return (error);
	}

	/*
	 * Keep a held reference to the root vnode.
	 * It is vrele'd in nullfs_unmount.
	 */
	ufmlm_rootvp = vp;
	ufmlm_rootvp->v_flag |= VROOT;
	xmp->ufmlm_rootvp = ufmlm_rootvp;
	if (UFMLVPTOLOWERVP(ufmlm_rootvp)->v_mount->mnt_flag & MNT_LOCAL)
		mp->mnt_flag |= MNT_LOCAL;
	mp->mnt_data = (qaddr_t) xmp;
	vfs_getnewfsid(mp);

	(void) copyinstr(path, mp->mnt_stat.f_mntonname, MNAMELEN - 1, &size);
	bzero(mp->mnt_stat.f_mntonname + size, MNAMELEN - size);
	(void) copyinstr(args.target, mp->mnt_stat.f_mntfromname, MNAMELEN - 1, 
	    &size);
	bzero(mp->mnt_stat.f_mntfromname + size, MNAMELEN - size);
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufmlfs_mount: lower %s, alias at %s\n",
		mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname);
#endif
	return (0);
}

/*
 * VFS start.  Nothing needed here - the start routine
 * on the underlying filesystem will have been called
 * when that filesystem was mounted.
 */
int
ufmlfs_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{
	return (0);
	/* return VFS_START(MOUNTTOUFMLMOUNT(mp)->ufmlm_vfs, flags, p); */
}

/*
 * Free reference to null layer
 */
int
ufmlfs_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	struct vnode *ufmlm_rootvp = MOUNTTOUFMLMOUNT(mp)->ufmlm_rootvp;
	int error;
	int flags = 0;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufmlfs_unmount(mp = %x)\n", mp);
#endif

	if (mntflags & MNT_FORCE)
		flags |= FORCECLOSE;

	/*
	 * Clear out buffer cache.  I don't think we
	 * ever get anything cached at this level at the
	 * moment, but who knows...
	 */
#if 0
	mntflushbuf(mp, 0); 
	if (mntinvalbuf(mp, 1))
		return (EBUSY);
#endif
	if (ufmlm_rootvp->v_usecount > 1)
		return (EBUSY);
	if ((error = vflush(mp, ufmlm_rootvp, flags)))
		return (error);

#ifdef UFMLFS_DIAGNOSTIC
	vprint("alias root of lower", ufmlm_rootvp);
#endif	 
	/*
	 * Release reference on underlying root vnode
	 */
	vrele(ufmlm_rootvp);
	/*
	 * And blow it away for future re-use
	 */
	vgone(ufmlm_rootvp);
	/*
	 * Finally, throw away the null_mount structure
	 */
	free(mp->mnt_data, M_UFSMNT);	/* XXX */
	mp->mnt_data = 0;
	return 0;
}

int
ufmlfs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct proc *p = curproc;	/* XXX */
	struct vnode *vp;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufmlfs_root(mp = %x, vp = %x->%x)\n", mp,
			MOUNTTOUFMLMOUNT(mp)->ufmlm_rootvp,
			UFMLVPTOLOWERVP(MOUNTTOUFMLMOUNT(mp)->ufmlm_rootvp)
			);
#endif

	/*
	 * Return locked reference to root.
	 */
	vp = MOUNTTOUFMLMOUNT(mp)->ufmlm_rootvp;
	VREF(vp);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	*vpp = vp;
	return 0;
}

int
ufmlfs_quotactl(mp, cmd, uid, arg, p)
	struct mount *mp;
	int cmd;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
	return VFS_QUOTACTL(MOUNTTOUFMLMOUNT(mp)->ufmlm_vfs, cmd, uid, arg, p);
}

int
ufmlfs_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{
	int error;
	struct statfs mstat;

#ifdef UFMLFS_DIAGNOSTIC
	printf("nullfs_statfs(mp = %x, vp = %x->%x)\n", mp,
			MOUNTTOUFMLMOUNT(mp)->ufmlm_rootvp,
			NULLVPTOLOWERVP(MOUNTTOUFMLMOUNT(mp)->ufmlm_rootvp)
			);
#endif

	bzero(&mstat, sizeof(mstat));

	error = VFS_STATFS(MOUNTTOUFMLMOUNT(mp)->ufmlm_vfs, &mstat, p);
	if (error)
		return (error);

	/* now copy across the "interesting" information and fake the rest */
	sbp->f_type = mstat.f_type;
	sbp->f_flags = mstat.f_flags;
	sbp->f_bsize = mstat.f_bsize;
	sbp->f_iosize = mstat.f_iosize;
	sbp->f_blocks = mstat.f_blocks;
	sbp->f_bfree = mstat.f_bfree;
	sbp->f_bavail = mstat.f_bavail;
	sbp->f_files = mstat.f_files;
	sbp->f_ffree = mstat.f_ffree;
	if (sbp != &mp->mnt_stat) {
		bcopy(&mp->mnt_stat.f_fsid, &sbp->f_fsid, sizeof(sbp->f_fsid));
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	}
	return (0);
}

int
ufmlfs_sync(mp, waitfor, cred, p)
	struct mount *mp;
	int waitfor;
	struct ucred *cred;
	struct proc *p;
{
	/*
	 * XXX - Assumes no data cached at null layer.
	 */
	return (0);
}

int
ufmlfs_vget(mp, ino, vpp)
	struct mount *mp;
	ino_t ino;
	struct vnode **vpp;
{
	
	return VFS_VGET(MOUNTTOUFMLMOUNT(mp)->ufmlm_vfs, ino, vpp);
}

int
ufmlfs_fhtovp(mp, fidp, nam, vpp, exflagsp, credanonp)
	struct mount *mp;
	struct fid *fidp;
	struct mbuf *nam;
	struct vnode **vpp;
	int *exflagsp;
	struct ucred**credanonp;
{

	return VFS_FHTOVP(MOUNTTOUFMLMOUNT(mp)->ufmlm_vfs, fidp, nam, vpp, exflagsp, credanonp);
}

int
ufmlfs_vptofh(vp, fhp)
	struct vnode *vp;
	struct fid *fhp;
{
	return VFS_VPTOFH(UFMLVPTOLOWERVP(vp), fhp);
}

int ufmlfs_init(struct vfsconf *);

#define ufmlfs_sysctl ((int (*) (int *, u_int, void *, size_t *, void *, size_t, struct proc *))eopnotsupp)

struct vfsops ufml_vfsops = {
		.vfs_mount = ufmlfs_mount,
		.vfs_start = ufmlfs_start,
		.vfs_unmount = ufmlfs_unmount,
		.vfs_root = ufmlfs_root,
		.vfs_quotactl = ufmlfs_quotactl,
		.vfs_statfs = ufmlfs_statfs,
		.vfs_sync = ufmlfs_sync,
		.vfs_vget = ufmlfs_vget,
		.vfs_fhtovp = ufmlfs_fhtovp,
		.vfs_vptofh = ufmlfs_vptofh,
		.vfs_init = ufmlfs_init,
		.vfs_sysctl = ufmlfs_sysctl,
};
