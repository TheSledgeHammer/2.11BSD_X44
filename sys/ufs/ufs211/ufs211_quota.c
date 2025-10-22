/*
 * Copyright (c) 1982, 1986, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
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
 *	@(#)ufs_quota.c	8.5 (Berkeley) 5/20/95
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)quota_ufs.c	7.1.1 (2.11BSD GTE) 12/31/93
 */

/*
 * MELBOURNE QUOTAS
 *
 * Routines used in checking limits on file system usage.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/user.h>

#include <ufs/ufs211/ufs211_quota.h>
#include <ufs/ufs211/ufs211_inode.h>
#include <ufs/ufs211/ufs211_mount.h>
#include <ufs/ufs211/ufs211_extern.h>

static char *quotatypes[] = INITQFNAMES;

#define QHASH(qvp, id)	\
	(&ufs211_qhashtbl[((((int)(qvp)) >> 8) + id) & UFS211_NQHASH])
LIST_HEAD(ufs211_qhash, ufs211_quota) *ufs211_qhashtbl;
u_long ufs211_qhash;

#define	DQHASH(dqvp, id) \
	(&ufs211_dqhashtbl[((((int)(dqvp)) >> 8) + id) & UFS211_NDQHASH])
LIST_HEAD(ufs211_dqhash, ufs211_dquot) *ufs211_dqhashtbl;
u_long ufs211_dqhash;

#define	DQUOTINC	5	/* minimum free dquots desired */
#define	QUOTINC		5	/* minimum free quots desired */
TAILQ_HEAD(ufs211_qfreelist, ufs211_quota) 	ufs211_qfreelist;
TAILQ_HEAD(ufs211_dqfreelist, ufs211_dquot) ufs211_dqfreelist;
long ufs211_numdquot, ufs211_desiredquot = DQUOTINC;

void
ufs211_quotainit(void)
{
	//ufs211_qhashtbl = hashinit(desiredvnodes, M_DQUOT, &ufs211_qhash);
	//TAILQ_INIT(&ufs211_qfreelist);

	ufs211_dqhashtbl = hashinit(desiredvnodes, M_DQUOT, &ufs211_dqhash);
	TAILQ_INIT(&ufs211_dqfreelist);
}

int
ufs211_getinoquota(ip)
	register struct ufs211_inode *ip;
{
	struct ufs211_mount *ump;
	struct vnode *vp;
	int error;

	vp = UFS211_ITOV(ip);
	ump = VFSTOUFS211(vp->v_mount);

	/*
	 * Set up the user quota based on file uid.
	 * EINVAL means that quotas are not enabled.
	 */
	if (ip->i_dquot[USRQUOTA] == NODQUOT
			&& (error = ufs211_dqget(vp, ip->i_uid, ump, USRQUOTA,
					&ip->i_dquot[USRQUOTA])) && error != EINVAL)
		return (error);
	/*
	 * Set up the group quota based on file gid.
	 * EINVAL means that quotas are not enabled.
	 */
	if (ip->i_dquot[GRPQUOTA] == NODQUOT
			&& (error = ufs211_dqget(vp, ip->i_gid, ump, GRPQUOTA,
					&ip->i_dquot[GRPQUOTA])) && error != EINVAL)
		return (error);

	return (0);
}

int
ufs211_chkdq(ip, change, cred, flags)
	register struct ufs211_inode *ip;
	long change;
	struct ucred *cred;
	int flags;
{
	register struct ufs211_dquot *dq;
	register int i;
	int ncurblocks, error;

#ifdef DIAGNOSTIC
	if ((flags & CHOWN) == 0) {
		ufs211_chkdquot(ip);
	}
#endif
	if (change == 0) {
		return (0);
	}
	if (change < 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			if ((dq = ip->i_dquot[i]) == NODQUOT) {
				continue;
			}
			while (dq->dq_flags & DQ_LOCK) {
				dq->dq_flags |= DQ_WANT;
				sleep((caddr_t)dq, PINOD+1);
			}
			ncurblocks = dq->dq_curblocks + change;
			if (ncurblocks >= 0) {
				dq->dq_curblocks = ncurblocks;
			} else {
				dq->dq_curblocks = 0;
			}
			dq->dq_flags &= ~DQ_BLKS;
			dq->dq_flags |= DQ_MOD;
		}
		return (0);
	}
	if ((flags & FORCE) == 0 && cred->cr_uid != 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			if ((dq = ip->i_dquot[i]) == NODQUOT) {
				continue;
			}
			error = ufs211_chkdqchg(ip, change, cred, i);
			if (error) {
				return (error);
			}
		}
	}
	for (i = 0; i < MAXQUOTAS; i++) {
		if ((dq = ip->i_dquot[i]) == NODQUOT) {
			continue;
		}
		while (dq->dq_flags & DQ_LOCK) {
			dq->dq_flags |= DQ_WANT;
			sleep((caddr_t)dq, PINOD+1);
		}
		dq->dq_curblocks += change;
		dq->dq_flags |= DQ_MOD;
	}
	return (0);
}

int
ufs211_chkdqchg(ip, change, cred, type)
	struct ufs211_inode *ip;
	long change;
	struct ucred *cred;
	int type;
{
	register struct ufs211_dquot *dq;
	long ncurblocks;

	dq = ip->i_dquot[type];
	ncurblocks = dq->dq_curblocks + change;

	if (ncurblocks >= dq->dq_bhardlimit && dq->dq_bhardlimit) {
		if ((dq->dq_flags & DQ_BLKS) == 0 && ip->i_uid == cred->cr_uid) {
			uprintf("\n%s: write failed, %s disk limit reached\n", UFS211_ITOV(ip)->v_mount->mnt_stat.f_mntonname, quotatypes[type]);
			dq->dq_flags |= DQ_BLKS;
		}
		return (EDQUOT);
	}
	if (ncurblocks >= dq->dq_bsoftlimit && dq->dq_bsoftlimit) {
		if (dq->dq_curblocks < dq->dq_bsoftlimit) {
			dq->dq_bwarn = time.tv_sec
					+ VFSTOUFS211(UFS211_ITOV(ip)->v_mount)->m_bwarn[type];
			if (ip->i_uid == cred->cr_uid)
				uprintf("\n%s: warning, %s %s\n",
				UFS211_ITOV(ip)->v_mount->mnt_stat.f_mntonname,
						quotatypes[type], "disk quota exceeded");
			return (0);
		}
		if (time.tv_sec > dq->dq_bwarn) {
			if ((dq->dq_flags & DQ_BLKS) == 0 && ip->i_uid == cred->cr_uid) {
				uprintf("\n%s: write failed, %s %s\n",
				UFS211_ITOV(ip)->v_mount->mnt_stat.f_mntonname,
						quotatypes[type], "disk quota exceeded for too long");
				dq->dq_flags |= DQ_BLKS;
			}
			return (EDQUOT);
		}
	}
	return (0);
}

/*
 * Check the inode limit, applying corrective action.
 */
int
ufs211_chkiq(ip, change, cred, flags)
	register struct ufs211_inode *ip;
	long change;
	struct ucred *cred;
	int flags;
{
	register struct ufs211_dquot *dq;
	register int i;
	int ncurinodes, error;

#ifdef DIAGNOSTIC
	if ((flags & CHOWN) == 0)
		ufs211_chkdquot(ip);
#endif
	if (change == 0)
		return (0);
	if (change < 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			if ((dq = ip->i_dquot[i]) == NODQUOT)
				continue;
			while (dq->dq_flags & DQ_LOCK) {
				dq->dq_flags |= DQ_WANT;
				sleep((caddr_t)dq, PINOD+1);
			}
			ncurinodes = dq->dq_curinodes + change;
			if (ncurinodes >= 0)
				dq->dq_curinodes = ncurinodes;
			else
				dq->dq_curinodes = 0;
			dq->dq_flags &= ~DQ_INODS;
			dq->dq_flags |= DQ_MOD;
		}
		return (0);
	}
	if ((flags & FORCE) == 0 && cred->cr_uid != 0) {
		for (i = 0; i < MAXQUOTAS; i++) {
			if ((dq = ip->i_dquot[i]) == NODQUOT)
				continue;
			if ((error = ufs211_chkiqchg(ip, change, cred, i)))
				return (error);
		}
	}
	for (i = 0; i < MAXQUOTAS; i++) {
		if ((dq = ip->i_dquot[i]) == NODQUOT)
			continue;
		while (dq->dq_flags & DQ_LOCK) {
			dq->dq_flags |= DQ_WANT;
			sleep((caddr_t)dq, PINOD+1);
		}
		dq->dq_curinodes += change;
		dq->dq_flags |= DQ_MOD;
	}
    return (0);
}

int
ufs211_chkiqchg(ip, change, cred, type)
	struct ufs211_inode *ip;
	long change;
	struct ucred *cred;
	int type;
{
	register struct ufs211_dquot *dq;
	long ncurinodes;

	dq = ip->i_dquot[type];
	ncurinodes = dq->dq_curinodes + change;
	/*
	 * If user would exceed their hard limit, disallow inode allocation.
	 */
	if (ncurinodes >= dq->dq_ihardlimit && dq->dq_ihardlimit) {
		if ((dq->dq_flags & DQ_INODS) == 0 &&
		    ip->i_uid == cred->cr_uid) {
			uprintf("\n%s: write failed, %s inode limit reached\n",
					UFS211_ITOV(ip)->v_mount->mnt_stat.f_mntonname,
			    quotatypes[type]);
			dq->dq_flags |= DQ_INODS;
		}
		return (EDQUOT);
	}
	/*
	 * If user is over their soft limit for too long, disallow inode
	 * allocation. Reset time limit as they cross their soft limit.
	 */
	if (ncurinodes >= dq->dq_isoftlimit && dq->dq_isoftlimit) {
		if (dq->dq_curinodes < dq->dq_isoftlimit) {
			dq->dq_iwarn = time.tv_sec +
			VFSTOUFS211(UFS211_ITOV(ip)->v_mount)->m_iwarn[type];
			if (ip->i_uid == cred->cr_uid)
				uprintf("\n%s: warning, %s %s\n",
				UFS211_ITOV(ip)->v_mount->mnt_stat.f_mntonname,
						quotatypes[type], "inode quota exceeded");
			return (0);
		}
		if (time.tv_sec > dq->dq_iwarn) {
			if ((dq->dq_flags & DQ_INODS) == 0 && ip->i_uid == cred->cr_uid) {
				uprintf("\n%s: write failed, %s %s\n",
				UFS211_ITOV(ip)->v_mount->mnt_stat.f_mntonname,
						quotatypes[type], "inode quota exceeded for too long");
				dq->dq_flags |= DQ_INODS;
			}
			return (EDQUOT);
		}
	}
	return (0);
}

#ifdef DIAGNOSTIC
/*
 * On filesystems with quotas enabled, it is an error for a file to change
 * size and not to have a dquot structure associated with it.
 */
void
ufs211_chkdquot(ip)
	register struct ufs211_inode *ip;
{
	struct ufs211_mount *ump;
	register int i;

	ump = VFSTOUFS211(UFS211_ITOV(ip)->v_mount);
	for (i = 0; i < MAXQUOTAS; i++) {
		if (ump->m_quotas[i] == NULLVP
				|| (ump->m_qflags[i] & (QTF_OPENING | QTF_CLOSING))) {
			continue;
		}
		if (ip->i_dquot[i] == NODQUOT) {
			vprint("chkdquot: missing dquot", UFS211_ITOV(ip));
			panic("missing dquot");
		}
	}
}
#endif

/*
 * Code to process quotactl commands.
 */

/*
 * Q_QUOTAON - set up a quota file for a particular file system.
 */
int
ufs211_quotaon(p, mp, type, fname)
	struct proc *p;
	struct mount *mp;
	register int type;
	caddr_t fname;
{
	struct ufs211_mount *ump;
	struct vnode *vp, **vpp;
	struct vnode *nextvp;
	struct ufs211_dquot *dq;
	int error;
	struct nameidata nd;

	ump = VFSTOUFS211(mp);
	vpp = &ump->m_quotas[type];
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, fname, p);
	error = vn_open(&nd, FREAD|FWRITE, 0);
	if (error) {
		return (error);
	}
	vp = nd.ni_vp;
	VOP_UNLOCK(vp, 0, p);
	if (vp->v_type != VREG) {
		(void) vn_close(vp, FREAD|FWRITE, p->p_ucred, p);
		return (EACCES);
	}
	if (*vpp != vp) {
		ufs211_quotaoff(p, mp, type);
	}
	ump->m_qflags[type] |= QTF_OPENING;
	mp->mnt_flag |= MNT_QUOTA;
	vp->v_flag |= VSYSTEM;
	*vpp = vp;
	/*
	 * Save the credential of the process that turned on quotas.
	 * Set up the time limits for this quota.
	 */
	crhold(p->p_ucred);
	ump->m_cred[type] = p->p_ucred;
	ump->m_bwarn[type] = MAX_DQ_WARN;
	ump->m_iwarn[type] = MAX_IQ_WARN;
	if (ufs211_dqget(NULLVP, 0, ump, type, &dq) == 0) {
		if (dq->dq_bwarn > 0) {
			ump->m_bwarn[type] = dq->dq_bwarn;
		}
		if (dq->dq_iwarn > 0) {
			ump->m_iwarn[type] = dq->dq_iwarn;
		}
		ufs211_dqrele(NULLVP, dq);
	}
	/*
	 * Search vnodes associated with this mount point,
	 * adding references to quota file being opened.
	 * NB: only need to add dquot's for inodes being modified.
	 */
again:
	for (vp = LIST_FIRST(&mp->mnt_vnodelist); vp != NULL; vp = nextvp) {
		nextvp = LIST_NEXT(vp, v_mntvnodes);
		if (vp->v_writecount == 0)
			continue;
		if (vget(vp, LK_EXCLUSIVE, p))
			goto again;
		if ((error = ufs211_getinoquota(UFS211_VTOI(vp)))) {
			vput(vp);
			break;
		}
		vput(vp);
		if (LIST_NEXT(vp, v_mntvnodes) != nextvp || vp->v_mount != mp) {
			goto again;
		}
	}
	ump->m_qflags[type] &= ~QTF_OPENING;
	if (error) {
		ufs211_quotaoff(p, mp, type);
	}
	return (error);
}

/*
 * Q_QUOTAOFF - turn off disk quotas for a filesystem.
 */
int
ufs211_quotaoff(p, mp, type)
	struct proc *p;
	struct mount *mp;
	register int type;
{
	struct vnode *vp;
	struct vnode *qvp, *nextvp;
	struct ufs211_mount *ump;
	struct ufs211_dquot *dq;
	struct ufs211_inode *ip;
	int error;

	ump = VFSTOUFS211(mp);
	if ((qvp = ump->m_quotas[type]) == NULLVP) {
		return (0);
	}
	ump->m_qflags[type] |= QTF_CLOSING;
	/*
	 * Search vnodes associated with this mount point,
	 * deleting any references to quota file being closed.
	 */
again:
	for (vp = LIST_FIRST(&mp->mnt_vnodelist); vp != NULL; vp = nextvp) {
		nextvp = LIST_NEXT(vp, v_mntvnodes);
		if (vget(vp, LK_EXCLUSIVE, p)) {
			goto again;
		}
		ip = UFS211_VTOI(vp);
		dq = ip->i_dquot[type];
		ip->i_dquot[type] = NODQUOT;
		ufs211_dqrele(vp, dq);
		vput(vp);
		if (LIST_NEXT(vp, v_mntvnodes) != nextvp || vp->v_mount != mp) {
			goto again;
		}
	}
	ufs211_dqflush(qvp);
	qvp->v_flag &= ~VSYSTEM;
	error = vn_close(qvp, FREAD | FWRITE, p->p_ucred, p);
	ump->m_quotas[type] = NULLVP;
	crfree(ump->m_cred[type]);
	ump->m_cred[type] = NOCRED;
	ump->m_qflags[type] &= ~QTF_CLOSING;
	for (type = 0; type < MAXQUOTAS; type++) {
		if (ump->m_quotas[type] != NULLVP) {
			break;
		}
	}
	if (type == MAXQUOTAS) {
		mp->mnt_flag &= ~MNT_QUOTA;
	}
	return (error);
}

/*
 * Q_GETQUOTA - return current values in a dqblk structure.
 */
int
ufs211_getquota(mp, id, type, addr)
	struct mount *mp;
	u_long id;
	int type;
	caddr_t addr;
{
	struct ufs211_dquot *dq;
	int error;

	if (error = ufs211_dqget(NULLVP, id, VFSTOUFS211(mp), type, &dq))
		return (error);
	error = copyout((caddr_t)&dq->dq_dqb, addr, sizeof (struct ufs211_dqblk));
	ufs211_dqrele(NULLVP, dq);
	return (error);
}

/*
 * Q_SETQUOTA - assign an entire dqblk structure.
 */
int
ufs211_setquota(mp, id, type, addr)
	struct mount *mp;
	u_long id;
	int type;
	caddr_t addr;
{
	register struct ufs211_dquot *dq;
	struct ufs211_dquot *ndq;
	struct ufs211_mount *ump = VFSTOUFS211(mp);
	struct ufs211_dqblk newlim;
	int error;

	if (error = copyin(addr, (caddr_t)&newlim, sizeof (struct ufs211_dqblk)))
		return (error);
	if (error = ufs211_dqget(NULLVP, id, ump, type, &ndq))
		return (error);
	dq = ndq;
	while (dq->dq_flags & DQ_LOCK) {
		dq->dq_flags |= DQ_WANT;
		sleep((caddr_t)dq, PINOD+1);
	}
	/*
	 * Copy all but the current values.
	 * Reset time limit if previously had no soft limit or were
	 * under it, but now have a soft limit and are over it.
	 */
	newlim.dqb_curblocks = dq->dq_curblocks;
	newlim.dqb_curinodes = dq->dq_curinodes;
	if (dq->dq_id != 0) {
		newlim.dqb_bwarn = dq->dq_bwarn;
		newlim.dqb_iwarn = dq->dq_iwarn;
	}
	if (newlim.dqb_bsoftlimit &&
	    dq->dq_curblocks >= newlim.dqb_bsoftlimit &&
	    (dq->dq_bsoftlimit == 0 || dq->dq_curblocks < dq->dq_bsoftlimit)) {
		newlim.dqb_bwarn = time.tv_sec + ump->m_bwarn[type];
	}
	if (newlim.dqb_isoftlimit &&
	    dq->dq_curinodes >= newlim.dqb_isoftlimit &&
	    (dq->dq_isoftlimit == 0 || dq->dq_curinodes < dq->dq_isoftlimit)) {
		newlim.dqb_iwarn = time.tv_sec + ump->m_iwarn[type];
	}
	dq->dq_dqb = newlim;
	if (dq->dq_curblocks < dq->dq_bsoftlimit) {
		dq->dq_flags &= ~DQ_BLKS;
	}
	if (dq->dq_curinodes < dq->dq_isoftlimit) {
		dq->dq_flags &= ~DQ_INODS;
	}
	if (dq->dq_isoftlimit == 0 && dq->dq_bsoftlimit == 0 &&
	    dq->dq_ihardlimit == 0 && dq->dq_bhardlimit == 0) {
		dq->dq_flags |= DQ_FAKE;
	} else {
		dq->dq_flags &= ~DQ_FAKE;
	}
	dq->dq_flags |= DQ_MOD;
	ufs211_dqrele(NULLVP, dq);
	return (0);
}

/*
 * Q_SETUSE - set current inode and block usage.
 */
int
ufs211_setuse(mp, id, type, addr)
	struct mount *mp;
	u_long id;
	int type;
	caddr_t addr;
{
	register struct ufs211_dquot *dq;
	struct ufs211_mount *ump = VFSTOUFS211(mp);
	struct ufs211_dquot *ndq;
	struct ufs211_dqblk usage;
	int error;

	if (error = copyin(addr, (caddr_t)&usage, sizeof (struct ufs211_dqblk)))
		return (error);
	if (error = ufs211_dqget(NULLVP, id, ump, type, &ndq))
		return (error);
	dq = ndq;
	while (dq->dq_flags & DQ_LOCK) {
		dq->dq_flags |= DQ_WANT;
		sleep((caddr_t)dq, PINOD+1);
	}
	/*
	 * Reset time limit if have a soft limit and were
	 * previously under it, but are now over it.
	 */
	if (dq->dq_bsoftlimit && dq->dq_curblocks < dq->dq_bsoftlimit &&
	    usage.dqb_curblocks >= dq->dq_bsoftlimit) {
		dq->dq_bwarn = time.tv_sec + ump->m_bwarn[type];
	}
	if (dq->dq_isoftlimit && dq->dq_curinodes < dq->dq_isoftlimit &&
	    usage.dqb_curinodes >= dq->dq_isoftlimit) {
		dq->dq_iwarn = time.tv_sec + ump->m_iwarn[type];
	}
	dq->dq_curblocks = usage.dqb_curblocks;
	dq->dq_curinodes = usage.dqb_curinodes;
	if (dq->dq_curblocks < dq->dq_bsoftlimit) {
		dq->dq_flags &= ~DQ_BLKS;
	}
	if (dq->dq_curinodes < dq->dq_isoftlimit) {
		dq->dq_flags &= ~DQ_INODS;
	}
	dq->dq_flags |= DQ_MOD;
	ufs211_dqrele(NULLVP, dq);
	return (0);
}

/*
 * Q_SYNC - sync quota files to disk.
 */
int
ufs211_qsync(mp)
	struct mount *mp;
{
	struct ufs211_mount *ump;
	struct proc *p = curproc;		/* XXX */
	struct vnode *vp, *nextvp;
	struct ufs211_dquot *dq;
	int i, error;

	ump = VFSTOUFS211(mp);
	/*
	 * Check if the mount point has any quotas.
	 * If not, simply return.
	 */
	for (i = 0; i < MAXQUOTAS; i++)
		if (ump->m_quotas[i] != NULLVP)
			break;
	if (i == MAXQUOTAS)
		return (0);
	/*
	 * Search vnodes associated with this mount point,
	 * synchronizing any modified dquot structures.
	 */
	simple_lock(&mntvnode_slock);
again:
	for (vp = LIST_FIRST(&mp->mnt_vnodelist); vp != NULL; vp = nextvp) {
		if (vp->v_mount != mp)
			goto again;
		nextvp = LIST_NEXT(vp, v_mntvnodes);
		simple_lock(&vp->v_interlock);
		simple_unlock(&mntvnode_slock);
		error = vget(vp, LK_EXCLUSIVE | LK_NOWAIT | LK_INTERLOCK, p);
		if (error) {
			simple_lock(&mntvnode_slock);
			if (error == ENOENT)
				goto again;
			continue;
		}
		for (i = 0; i < MAXQUOTAS; i++) {
			dq = UFS211_VTOI(vp)->i_dquot[i];
			if (dq != NODQUOT && (dq->dq_flags & DQ_MOD))
				ufs211_dqsync(vp, dq);
		}
		vput(vp);
		simple_lock(&mntvnode_slock);
		if (LIST_NEXT(vp, v_mntvnodes) != nextvp)
			goto again;
	}
	simple_unlock(&mntvnode_slock);
	return (0);
}

/*
 * Obtain a dquot structure for the specified identifier and quota file
 * reading the information from the file if necessary.
 */
int
ufs211_dqget(vp, id, ump, type, dqp)
	struct vnode *vp;
	u_long id;
	register struct ufs211_mount *ump;
	register int type;
	struct ufs211_dquot **dqp;
{
	struct proc *p = curproc;		/* XXX */
	struct ufs211_dquot *dq;
	struct ufs211_dqhash *dqh;
	struct vnode *dqvp;
	struct iovec aiov;
	struct uio auio;
	int error;

	dqvp = ump->m_quotas[type];
	if (dqvp == NULLVP || (ump->m_qflags[type] & QTF_CLOSING)) {
		*dqp = NODQUOT;
		return (EINVAL);
	}
	/*
	 * Check the cache first.
	 */
	dqh = DQHASH(dqvp, id);
	for (dq = LIST_FIRST(dqh); dq; dq = LIST_NEXT(dq, dq_hash)) {
		if (dq->dq_id != id ||
		    dq->dq_ump->m_quotas[dq->dq_type] != dqvp)
			continue;
		/*
		 * Cache hit with no references.  Take
		 * the structure off the free list.
		 */
		if (dq->dq_cnt == 0)
			TAILQ_REMOVE(&ufs211_dqfreelist, dq, dq_freelist);
		DQREF(dq);
		*dqp = dq;
		return (0);
	}
	/*
	 * Not in cache, allocate a new one.
	 */
	if (TAILQ_FIRST(&ufs211_dqfreelist) == NODQUOT &&
	    ufs211_numdquot < MAXQUOTAS * desiredvnodes)
		ufs211_desiredquot += DQUOTINC;
	if (ufs211_numdquot < ufs211_desiredquot) {
		dq = (struct ufs211_dquot *)malloc(sizeof *dq, M_DQUOT, M_WAITOK);
		bzero((char *)dq, sizeof *dq);
		ufs211_numdquot++;
	} else {
		if ((dq = TAILQ_FIRST(&ufs211_dqfreelist)) == NULL) {
			tablefull("dquot");
			*dqp = NODQUOT;
			return (EUSERS);
		}
		if (dq->dq_cnt || (dq->dq_flags & DQ_MOD))
			panic("free dquot isn't");
		TAILQ_REMOVE(&ufs211_dqfreelist, dq, dq_freelist);
		LIST_REMOVE(dq, dq_hash);
	}
	/*
	 * Initialize the contents of the dquot structure.
	 */
	if (vp != dqvp)
		vn_lock(dqvp, LK_EXCLUSIVE | LK_RETRY, p);
	LIST_INSERT_HEAD(dqh, dq, dq_hash);
	DQREF(dq);
	dq->dq_flags = DQ_LOCK;
	dq->dq_id = id;
	dq->dq_ump = ump;
	dq->dq_type = type;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = (caddr_t)&dq->dq_dqb;
	aiov.iov_len = sizeof (struct ufs211_dqblk);
	auio.uio_resid = sizeof (struct ufs211_dqblk);
	auio.uio_offset = (off_t)(id * sizeof (struct ufs211_dqblk));
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = (struct proc *)0;
	error = VOP_READ(dqvp, &auio, 0, ump->m_cred[type]);
	if (auio.uio_resid == sizeof(struct ufs211_dqblk) && error == 0)
		bzero((caddr_t)&dq->dq_dqb, sizeof(struct ufs211_dqblk));
	if (vp != dqvp)
		VOP_UNLOCK(dqvp, 0, p);
	if (dq->dq_flags & DQ_WANT)
		wakeup((caddr_t)dq);
	dq->dq_flags = 0;
	/*
	 * I/O error in reading quota file, release
	 * quota structure and reflect problem to caller.
	 */
	if (error) {
		LIST_REMOVE(dq, dq_hash);
		ufs211_dqrele(vp, dq);
		*dqp = NODQUOT;
		return (error);
	}
	/*
	 * Check for no limit to enforce.
	 * Initialize time values if necessary.
	 */
	if (dq->dq_isoftlimit == 0 && dq->dq_bsoftlimit == 0 &&
	    dq->dq_ihardlimit == 0 && dq->dq_bhardlimit == 0)
		dq->dq_flags |= DQ_FAKE;
	if (dq->dq_id != 0) {
		if (dq->dq_bwarn == 0)
			dq->dq_bwarn = time.tv_sec + ump->m_bwarn[type];
		if (dq->dq_iwarn == 0)
			dq->dq_iwarn = time.tv_sec + ump->m_iwarn[type];
	}
	*dqp = dq;
	return (0);
}

/*
 * Obtain a reference to a dquot.
 */
void
ufs211_dqref(dq)
	struct ufs211_dquot *dq;
{
	dq->dq_cnt++;
}

/*
 * Release a reference to a dquot.
 */
void
ufs211_dqrele(vp, dq)
	struct vnode *vp;
	register struct ufs211_dquot *dq;
{

	if (dq == NODQUOT)
		return;
	if (dq->dq_cnt > 1) {
		dq->dq_cnt--;
		return;
	}
	if (dq->dq_flags & DQ_MOD)
		(void) ufs211_dqsync(vp, dq);
	if (--dq->dq_cnt > 0)
		return;
	TAILQ_INSERT_TAIL(&ufs211_dqfreelist, dq, dq_freelist);
}

/*
 * Update the disk quota in the quota file.
 */
int
ufs211_dqsync(vp, dq)
	struct vnode *vp;
	struct ufs211_dquot *dq;
{
	struct proc *p = curproc;		/* XXX */
	struct vnode *dqvp;
	struct iovec aiov;
	struct uio auio;
	int error;

	if (dq == NODQUOT)
		panic("dqsync: dquot");
	if ((dq->dq_flags & DQ_MOD) == 0)
		return (0);
	if ((dqvp = dq->dq_ump->m_quotas[dq->dq_type]) == NULLVP)
		panic("dqsync: file");
	if (vp != dqvp)
		vn_lock(dqvp, LK_EXCLUSIVE | LK_RETRY, p);
	while (dq->dq_flags & DQ_LOCK) {
		dq->dq_flags |= DQ_WANT;
		sleep((caddr_t) dq, PINOD + 2);
		if ((dq->dq_flags & DQ_MOD) == 0) {
			if (vp != dqvp)
				VOP_UNLOCK(dqvp, 0, p);
			return (0);
		}
	}
	dq->dq_flags |= DQ_LOCK;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = (caddr_t)&dq->dq_dqb;
	aiov.iov_len = sizeof (struct ufs211_dqblk);
	auio.uio_resid = sizeof (struct ufs211_dqblk);
	auio.uio_offset = (off_t)(dq->dq_id * sizeof (struct ufs211_dqblk));
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_WRITE;
	auio.uio_procp = (struct proc *)0;
	error = VOP_WRITE(dqvp, &auio, 0, dq->dq_ump->m_cred[dq->dq_type]);
	if (auio.uio_resid && error == 0)
		error = EIO;
	if (dq->dq_flags & DQ_WANT)
		wakeup((caddr_t) dq);
	dq->dq_flags &= ~(DQ_MOD | DQ_LOCK | DQ_WANT);
	if (vp != dqvp)
		VOP_UNLOCK(dqvp, 0, p);
	return (error);
}

/*
 * Flush all entries from the cache for a particular vnode.
 */
void
ufs211_dqflush(vp)
	register struct vnode *vp;
{
	register struct ufs211_dquot *dq, *nextdq;
	struct ufs211_dqhash *dqh;

	/*
	 * Move all dquot's that used to refer to this quota
	 * file off their hash chains (they will eventually
	 * fall off the head of the free list and be re-used).
	 */
	for (dqh = &ufs211_dqhashtbl[ufs211_dqhash]; dqh >= ufs211_dqhashtbl; dqh--) {
		for (dq = LIST_FIRST(dqh); dq; dq = nextdq) {
			nextdq = LIST_NEXT(dq, dq_hash);
			if (dq->dq_ump->m_quotas[dq->dq_type] != vp)
				continue;
			if (dq->dq_cnt) {
				panic("dqflush: stray dquot");
			}
			LIST_REMOVE(dq, dq_hash);
			dq->dq_ump = (struct ufs211_mount *)0;
		}
	}
}

int
ufs211_setwarn(mp, id, type, addr)
	struct mount *mp;
	u_long id;
	int type;
	caddr_t addr;
{
	register struct ufs211_dquot *dq;
	struct ufs211_mount *ump;
	struct ufs211_dquot *ndq;
	struct ufs211_dqwarn warn;
	int error;

	ump = VFSTOUFS211(mp);
	if ((error = ufs211_dqget(NULLVP, id, ump, type, &ndq))) {
		return (error);
	}
	dq = ndq;
	if (dq == NODQUOT) {
		return (ESRCH);
	}
	while (dq->dq_flags & DQ_LOCK) {
		dq->dq_flags |= DQ_WANT;
		sleep((caddr_t)dq, PINOD+1);
	}
	if ((error = copyin(addr, (caddr_t)&warn, sizeof(struct ufs211_dqwarn)))) {
		dq->dq_iwarn = warn.dw_iwarn;
		dq->dq_bwarn = warn.dw_bwarn;
		dq->dq_flags &= ~(DQ_INODS | DQ_BLKS);
		dq->dq_flags |= DQ_MOD;
	}
	ufs211_dqrele(NULLVP, dq);
	return (0);
}

void
ufs211_qwarn(ump, dq)
	register struct ufs211_mount *ump;
	register struct ufs211_dquot *dq;
{
	register struct ufs211_fs *fs;

	fs = ump->m_filsys;
	if (dq->dq_isoftlimit && dq->dq_curinodes >= dq->dq_isoftlimit) {
		dq->dq_flags |= DQ_MOD;
		if (dq->dq_iwarn && --dq->dq_iwarn) {
			/* uprintf(
			    "Warning: too many files on %s, %d warning%s left\n"
			    , fs->fs_fsmnt
			    , dq->dq_iwarn
			    , dq->dq_iwarn > 1 ? "s" : ""
			); */
		} else {
			/* uprintf(
			    "WARNING: too many files on %s, NO MORE!!\n"
			    , fs->fs_fsmnt
			); */
		}
	} else {
		dq->dq_iwarn = MAX_IQ_WARN;
	}
	if (dq->dq_bsoftlimit && dq->dq_curblocks >= dq->dq_bsoftlimit) {
		dq->dq_flags |= DQ_MOD;
		if (dq->dq_bwarn && --dq->dq_bwarn) {
			/* uprintf(
				"Warning: too much disc space on %s, %d warning%s left\n"
				, fs->fs_fsmnt
				, dq->dq_bwarn
				, dq->dq_bwarn > 1 ? "s" : ""
			); */
		} else {
			/* uprintf(
				"WARNING: too much disc space on %s, NO MORE!!\n"
				, fs->fs_fsmnt
			); */
		}
	} else {
		dq->dq_bwarn = MAX_DQ_WARN;
	}
}

int
ufs211_dowarn(mp, id, type)
	struct mount *mp;
	u_long id;
	int type;
{
	struct ufs211_mount *ump;
	struct ufs211_dquot *dq;
	struct ufs211_dqhash *dqh;
	struct vnode *dqvp;

	ump = VFSTOUFS211(mp);
	dqvp = ump->m_quotas[type];
	if (dqvp == NULLVP || (ump->m_qflags[type] & QTF_CLOSING)) {
		//*dq = NODQUOT;
		return (EINVAL);
	}

	dqh = DQHASH(dqvp, id);
	for (dq = LIST_FIRST(dqh); dq; dq = LIST_NEXT(dq, dq_hash)) {
		if (dq != NODQUOT && dq != LOSTDQUOT) {
			ufs211_qwarn(ump, dq);
			ufs211_dqrele(NULLVP, dq);
		}
	}
	return (0);
}

int
ufs211_setduse(mp, id, type, addr)
	struct mount *mp;
	u_long id;
	int type;
	caddr_t addr;
{
	register struct ufs211_dquot *dq;
	struct ufs211_mount *ump;
	struct ufs211_dquot *ndq;
	struct ufs211_dqusage usage;
	int error;

	ump = VFSTOUFS211(mp);
	if (error = ufs211_dqget(NULLVP, id, ump, type, &ndq)) {
		return (error);
	}
	dq = ndq;
	if (dq == NODQUOT) {
		return (ESRCH);
	}
	while (dq->dq_flags & DQ_LOCK) {
		dq->dq_flags |= DQ_WANT;
		sleep((caddr_t)dq, PINOD+1);
	}
	if (error = copyin(addr, (caddr_t)&usage, sizeof(struct ufs211_dqusage))) {
		dq->dq_curinodes = usage.du_curinodes;
		dq->dq_curblocks = usage.du_curblocks;
		if (dq->dq_curinodes < dq->dq_isoftlimit) {
			dq->dq_iwarn = MAX_IQ_WARN;
		}
		if (dq->dq_curblocks < dq->dq_bsoftlimit) {
			dq->dq_bwarn = MAX_DQ_WARN;
		}
		dq->dq_flags &= ~(DQ_INODS | DQ_BLKS);
		dq->dq_flags |= DQ_MOD;
	}
	ufs211_dqrele(NULLVP, dq);
	return (error);
}
