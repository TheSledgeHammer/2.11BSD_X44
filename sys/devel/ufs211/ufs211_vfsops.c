/*
 * ufs211_vfsops.c
 *
 *  Created on: 18 Apr 2020
 *      Author: marti
 */


#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/malloc.h>

#include <ufs211/ufs211_quota.h>
#include <ufs211/ufs211_inode.h>
#include <ufs211/ufs211_dir.h>
#include <ufs211/ufs211_extern.h>
#include <ufs211/ufs211_fs.h>
#include <miscfs/specfs/specdev.h>


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

	if (error == VFS_VGET(mp, (ino_t)ROOTINO, &nvp))
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
	case Q_GETDLIM:
		if (uid == p->p_cred->p_ruid)
			break;

		break;
		/* fall through */
	default:
		if (error == suser(p->p_ucred, &p->p_acflag))
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
#ifdef QUOTA
//	dqinit();
#endif
	return (0);
}

int
ufs211_sync(mp, vpp, ufs211)
	struct mount *mp;
	struct vnode **vpp;
	struct ufs211 *ufs211;
{

}
