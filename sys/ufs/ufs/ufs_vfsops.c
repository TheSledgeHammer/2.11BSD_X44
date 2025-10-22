/*
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ufs_vfsops.c	8.8 (Berkeley) 5/20/95
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/ufs_extern.h>
#include <miscfs/specfs/specdev.h>

/*
 * Make a filesystem operational.
 * Nothing to do at the moment.
 */
/* ARGSUSED */
int
ufs_start(mp, flags, p)
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
ufs_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct vnode *nvp;
	int error;

	if ((error = VFS_VGET(mp, (ino_t)ROOTINO, &nvp)))
		return (error);
	*vpp = nvp;
	return (0);
}

/*
 * Do operations associated with quotas
 */
int
ufs_quotactl(mp, cmds, uid, arg, p)
	struct mount *mp;
	int cmds;
	uid_t uid;
	caddr_t arg;
	struct proc *p;
{
	int cmd, type, error;

#ifndef QUOTA
	return (EOPNOTSUPP);
#else
	if (uid == -1)
		uid = p->p_cred->p_ruid;
	cmd = cmds >> SUBCMDSHIFT;

	switch (cmd) {
	case Q_SYNC:
		break;
	case Q_GETQUOTA:
		if (uid == p->p_cred->p_ruid)
			break;

		break;
		/* fall through */
	default:
		if ((error = suser1(p->p_ucred, &p->p_acflag)))
			return (error);
	}
	type = cmds & SUBCMDMASK;

	if ((u_int)type >= MAXQUOTAS)
		return (EINVAL);

	if (vfs_busy(mp, LK_NOWAIT, 0, p))
		return (0);

	switch (cmd) {

	case Q_QUOTAON:
		error = quotaon(p, mp, type, arg);
		break;

	case Q_QUOTAOFF:
		error = quotaoff(p, mp, type);
		break;

	case Q_SETQUOTA:
		error = setquota(mp, uid, type, arg);
		break;

	case Q_SETUSE:
		error = setuse(mp, uid, type, arg);
		break;

	case Q_GETQUOTA:
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

/*
 * Initial UFS filesystems, done only once.
 */
int
ufs_init(vfsp)
	struct vfsconf *vfsp;
{
	static int done;

	if (done)
		return (0);
	done = 1;
	ufs_ihashinit();
#ifdef QUOTA
	dqinit();
#endif
	return (0);
}

/*
 * This is the generic part of fhtovp called after the underlying
 * filesystem has validated the file handle.
 *
 * Verify that a host should have access to a filesystem, and if so
 * return a vnode for the presented file handle.
 */
int
ufs_check_export(mp, ufhp, nam, vpp, exflagsp, credanonp)
	register struct mount *mp;
	struct ufid *ufhp;
	struct mbuf *nam;
	struct vnode **vpp;
	int *exflagsp;
	struct ucred **credanonp;
{
	register struct inode *ip;
	register struct netcred *np;
	register struct ufsmount *ump = VFSTOUFS(mp);
	struct vnode *nvp;
	int error;

	/*
	 * Get the export permission structure for this <mp, client> tuple.
	 */
	np = vfs_export_lookup(mp, &ump->um_export, nam);
	if (np == NULL)
		return (EACCES);

	if ((error = VFS_VGET(mp, ufhp->ufid_ino, &nvp))) {
		*vpp = NULLVP;
		return (error);
	}
	ip = VTOI(nvp);
	if (ip->i_mode == 0 || ip->i_gen != ufhp->ufid_gen) {
		vput(nvp);
		*vpp = NULLVP;
		return (ESTALE);
	}
	*vpp = nvp;
	*exflagsp = np->netc_exflags;
	*credanonp = &np->netc_anon;
	return (0);
}

/*
 * This is the generic part of fhtovp called after the underlying
 * filesystem has validated the file handle.
 *
 * Call the VFS_CHECKEXP beforehand to verify access.
 */
int
ufs_fhtovp(mp, ufhp, vpp)
	struct mount *mp;
	struct ufid *ufhp;
	struct vnode **vpp;
{
	struct inode *ip;
	struct vnode *nvp;
	int error;

	error = VFS_VGET(mp, ufhp->ufid_ino, &nvp);
	if (error) {
		*vpp = NULLVP;
		return (error);
	}
	ip = VTOI(nvp);
	if (ip->i_mode == 0 || ip->i_gen != ufhp->ufid_gen || ip->i_effnlink <= 0) {
		vput(nvp);
		*vpp = NULLVP;
		return (ESTALE);
	}
	*vpp = nvp;
	return (0);
}
