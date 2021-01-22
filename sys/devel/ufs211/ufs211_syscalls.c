/*
 * ufs211_syscalls.c
 *
 *  Created on: 22 Jan 2021
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/user.h>
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
#include <sys/dirent.h>
#include <sys/unistd.h>

#include <vm/include/vm.h>
#include <miscfs/specfs/specdev.h>

#include "ufs211/ufs211_dir.h"
#include "ufs211/ufs211_extern.h"
#include "ufs211/ufs211_fs.h"
#include "ufs211/ufs211_inode.h"
#include "ufs211/ufs211_mount.h"
#include "ufs211/ufs211_quota.h"

/*
 * Change mode of a file given path name.
 */
void
ufs211_chmod()
{
	register struct ufs211_inode *ip;
	register struct a {
		char	*fname;
		int		fmode;
	} *uap = (struct a *)u->u_ap;

	struct	vattr				vattr;
	struct	nameidata 			nd;
	register struct nameidata *ndp = &nd;

	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, uap->fname);
	ip = namei(ndp);
	if (!ip)
		return;
	VATTR_NULL(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u->u_error = ufs_setattr(ip, &vattr);
	vput(ip);
}

int
ufs211_chmod1(vp, mode)
	register struct vnode *vp;
	register int mode;
{
	register struct ufs211_inode *ip = UFS211_VTOI(vp);

	if (u->u_uid != ip->i_uid && !suser())
		return (u->u_error);
	if (u->u_uid) {
		if ((ip->i_mode & UFS211_IFMT) != UFS211_IFDIR && (mode & UFS211_ISVTX)) {
			return (EFTYPE);
		}
		if (!groupmember(ip->i_gid) && (mode & UFS211_ISGID)) {
			return (EPERM);
		}
	}
	ip->i_mode &= ~07777; /* why? */
	ip->i_mode |= mode & 07777;
	ip->i_flag |= UFS211_ICHG;

	if ((vp->v_flag & VTEXT) && (ip->i_mode & S_ISTXT) == 0) {
		(void) vnode_pager_uncache(vp);
	}

	return (0);
}

/*
 * Set ownership given a path name.
 */
void
ufs211_chown()
{
	struct vnode *vp;
	register struct ufs211_inode *ip;
	register struct a {
		char	*fname;
		int		uid;
		int		gid;
	} *uap = (struct a *)u->u_ap;

	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;
	struct	vattr	vattr;

	NDINIT(ndp, LOOKUP, NOFOLLOW, UIO_USERSPACE, uap->fname);
	ip = namei(ndp);
	if (ip == NULL)
		return;
	VATTR_NULL(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u->u_error = ufs_setattr(ip, &vattr);
	vput(ip);
}


/*
 * Perform chown operation on inode ip.  This routine called from ufs_setattr.
 * inode must be locked prior to call.
 */
int
ufs211_chown1(ip, uid, gid)
	register struct ufs211_inode *ip;
	register int uid, gid;
{
	int ouid, ogid;
#ifdef QUOTA
	struct	dquot	**xdq;
	long change;
#endif
	if (uid == -1)
		uid = ip->i_uid;
	if (gid == -1)
		gid = ip->i_gid;
	/*
	 * If we don't own the file, are trying to change the owner
	 * of the file, or are not a member of the target group,
	 * the caller must be superuser or the call fails.
	 */
	if ((u->u_uid != ip->i_uid || uid != ip->i_uid || !groupmember((gid_t) gid)) && !suser()) {
		return (u->u_error);
	}
	ouid = ip->i_uid;
	ogid = ip->i_gid;
#ifdef QUOTA
	QUOTAMAP();
	if (ip->i_uid == uid)
		change = 0;
	else
		change = ip->i_size;
	(void) chkdq(ip, -change, 1);
	(void) chkiq(ip->i_dev, ip, ip->i_uid, 1);
	xdq = &ix_dquot[ip - inode];
	dqrele(*xdq);
#endif
	ip->i_uid = uid;
	ip->i_gid = gid;
#ifdef QUOTA
	*xdq = inoquota(ip);
	(void) chkdq(ip, change, 1);
	(void) chkiq(ip->i_dev, (struct inode *)NULL, (uid_t)uid, 1);
	QUOTAUNMAP();
#endif
	if (ouid != uid || ogid != gid)
		ip->i_flag |= UFS211_ICHG;
	if (ouid != uid && u->u_uid != 0)
		ip->i_mode &= ~UFS211_ISUID;
	if (ogid != gid && u->u_uid != 0)
		ip->i_mode &= ~UFS211_ISGID;
	return (0);
}
