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

#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/user.h>
#include <sys/param.h>

#include <sys/malloc.h>

#include <ufs/ufs211/ufs211_extern.h>
#include <ufs/ufs211/ufs211_fs.h>
#include <ufs/ufs211/ufs211_inode.h>
#include <ufs/ufs211/ufs211_mount.h>
#include <devel/ufs/ufs211/ufs211_quota.h>

#define QHASH(qvp, id)	\
	(&qhashtbl[((((int)(qvp)) >> 8) + id) & UFS211_NQHASH])
LIST_HEAD(ufs211_qhash, ufs211_quota) *ufs211_qhashtbl;
u_long ufs211_qhash;

#define	DQHASH(dqvp, id) \
	(&ufs211_dqhashtbl[((((int)(dqvp)) >> 8) + id) & UFS211_NDQHASH])
LIST_HEAD(ufs211_dqhash, ufs211_dquot) *ufs211_dqhashtbl;
u_long ufs211_dqhash;

#define	QUOTINC	5	/* minimum free quots desired */
TAILQ_HEAD(ufs211_qfreelist, ufs211_quota) 	ufs211_qfreelist;
TAILQ_HEAD(ufs211_dqfreelist, ufs211_dquot) ufs211_dqfreelist;
long numdquot, desiredquot = QUOTINC;

void
quotainit()
{
	ufs211_qhashtbl = hashinit(desiredvnodes, M_DQUOT, &ufs211_qhash);
	ufs211_dqhashtbl = hashinit(desiredvnodes, M_DQUOT, &ufs211_dqhash);
	TAILQ_INIT(&ufs211_qfreelist);
	TAILQ_INIT(&ufs211_dqfreelist);
}

int
getinoquota(ip)
	register struct ufs211_inode *ip;
{
	struct ufs211_mount *ump;
	struct vnode *vp;
	int error;

	vp = UFS211_ITOV(ip);
	ump = VFSTOUFS(vp->v_mount);



	return (0);
}
