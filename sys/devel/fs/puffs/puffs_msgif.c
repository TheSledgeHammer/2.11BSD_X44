/*	$NetBSD: puffs_msgif.c,v 1.8 2006/11/21 01:53:33 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_msgif.c,v 1.8 2006/11/21 01:53:33 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/lock.h>
#include <sys/poll.h>

#include <devel/fs/puffs/puffs_msgif.h>
#include <devel/fs/puffs/puffs_mount.h>

/*
 * kernel-user-kernel waitqueues
 */

static int touser(struct puffs_mount *, struct puffs_park *, uint64_t,
		  struct vnode *, struct vnode *);

uint64_t
puffs_getreqid(struct puffs_mount *pmp)
{
	unsigned int rv;

	simple_lock(&pmp->pmp_lock);
	rv = pmp->pmp_nextreq++;
	simple_unlock(&pmp->pmp_lock);

	return (rv);
}

/* vfs request */
int
puffs_vfstouser(struct puffs_mount *pmp, int optype, void *kbuf, size_t buflen)
{
	struct puffs_park park;

	memset(&park.park_preq, 0, sizeof(struct puffs_req));

	park.park_opclass = PUFFSOP_VFS;
	park.park_optype = optype;

	park.park_kernbuf = kbuf;
	park.park_buflen = buflen;
	park.park_copylen = buflen;
	park.park_flags = 0;

	return (touser(pmp, &park, puffs_getreqid(pmp), NULL, NULL));
}

/*
 * vnode level request
 */
int
puffs_vntouser(struct puffs_mount *pmp, int optype,
	void *kbuf, size_t buflen, void *cookie,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_park park;

	memset(&park.park_preq, 0, sizeof(struct puffs_req));

	park.park_opclass = PUFFSOP_VN;
	park.park_optype = optype;
	park.park_cookie = cookie;

	park.park_kernbuf = kbuf;
	park.park_buflen = buflen;
	park.park_copylen = buflen;
	park.park_flags = 0;

	return (touser(pmp, &park, puffs_getreqid(pmp), vp1, vp2));
}

/*
 * vnode level request, caller-controller req id
 */
int
puffs_vntouser_req(struct puffs_mount *pmp, int optype,
	void *kbuf, size_t buflen, void *cookie, uint64_t reqid,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_park park;

	memset(&park.park_preq, 0, sizeof(struct puffs_req));

	park.park_opclass = PUFFSOP_VN;
	park.park_optype = optype;
	park.park_cookie = cookie;

	park.park_kernbuf = kbuf;
	park.park_buflen = buflen;
	park.park_copylen = buflen;
	park.park_flags = 0;

	return (touser(pmp, &park, reqid, vp1, vp2));
}

/*
 * vnode level request, copy routines can adjust "kernbuf"
 */
int
puffs_vntouser_adjbuf(struct puffs_mount *pmp, int optype,
	void **kbuf, size_t *buflen, size_t copylen, void *cookie,
	struct vnode *vp1, struct vnode *vp2)
{
	struct puffs_park park;
	int error;

	memset(&park.park_preq, 0, sizeof(struct puffs_req));

	park.park_opclass = PUFFSOP_VN;
	park.park_optype = optype;
	park.park_cookie = cookie;

	park.park_kernbuf = *kbuf;
	park.park_buflen = *buflen;
	park.park_copylen = copylen;
	park.park_flags = PUFFS_REQFLAG_ADJBUF;

	error = touser(pmp, &park, puffs_getreqid(pmp), vp1, vp2);
	*kbuf = park.park_kernbuf;
	*buflen = park.park_buflen;

	return (error);
}

/*
 * Notice: kbuf will be free'd later.  I must be allocated from the
 * kernel heap and it's ownership is shifted to this function from
 * now on, i.e. the caller is not allowed to use it anymore!
 */
void
puffs_vntouser_faf(struct puffs_mount *pmp, int optype,
	void *kbuf, size_t buflen, void *cookie)
{
	struct puffs_park *ppark;

	/* XXX: is it allowable to sleep here? */
	ppark = malloc(sizeof(struct puffs_park), M_PUFFS, M_NOWAIT | M_ZERO);
	if (ppark == NULL) {
		return; /* 2bad */
	}

	ppark->park_opclass = PUFFSOP_VN | PUFFSOPFLAG_FAF;
	ppark->park_optype = optype;
	ppark->park_cookie = cookie;

	ppark->park_kernbuf = kbuf;
	ppark->park_buflen = buflen;
	ppark->park_copylen = buflen;

	(void)touser(pmp, ppark, 0, NULL, NULL);
}

/*
 * Wait for the userspace ping-pong game in calling process context.
 *
 * This unlocks vnodes if they are supplied.  vp1 is the vnode
 * before in the locking order, i.e. the one which must be locked
 * before accessing vp2.  This is done here so that operations are
 * already ordered in the queue when vnodes are unlocked (I'm not
 * sure if that's really necessary, but it can't hurt).  Okok, maybe
 * there's a slight ugly-factor also, but let's not worry about that.
 */
static int
touser(struct puffs_mount *pmp, struct puffs_park *ppark, uint64_t reqid,
	struct vnode *vp1, struct vnode *vp2)
{

	simple_lock(&pmp->pmp_lock);
	if (pmp->pmp_status != PUFFSTAT_RUNNING
	    && pmp->pmp_status != PUFFSTAT_MOUNTING) {
		simple_unlock(&pmp->pmp_lock);
		return (ENXIO);
	}

	ppark->park_id = reqid;

	TAILQ_INSERT_TAIL(&pmp->pmp_req_touser, ppark, park_entries);
	pmp->pmp_req_touser_waiters++;

	/*
	 * Don't do unlock-relock dance yet.  There are a couple of
	 * unsolved issues with it.  If we don't unlock, we can have
	 * processes wanting vn_lock in case userspace hangs.  But
	 * that can be "solved" by killing the userspace process.  It
	 * would of course be nicer to have antilocking in the userspace
	 * interface protocol itself.. your patience will be rewarded.
	 */
#if 0
	/* unlock */
	if (vp2)
		VOP_UNLOCK(vp2, 0);
	if (vp1)
		VOP_UNLOCK(vp1, 0);
#endif

	/*
	 * XXX: does releasing the lock here cause trouble?  Can't hold
	 * it, because otherwise the below would cause locking against
	 * oneself-problems in the kqueue stuff.  yes, it is a
	 * theoretical race, so it must be solved
	 */
	simple_unlock(&pmp->pmp_lock);

	wakeup(&pmp->pmp_req_touser);
	selnotify(pmp->pmp_sel, 0);

	if (PUFFSOP_WANTREPLY(ppark->park_opclass)) {
		ltsleep(ppark, PUSER, "puffs1", 0, NULL);
	}

#if 0
	/* relock */
	if (vp1)
		KASSERT(vn_lock(vp1, LK_EXCLUSIVE | LK_RETRY) == 0);
	if (vp2)
		KASSERT(vn_lock(vp2, LK_EXCLUSIVE | LK_RETRY) == 0);
#endif

	return (ppark->park_rv);
}

/*
 * We're dead, kaput, RIP, slightly more than merely pining for the
 * fjords, belly-up, fallen, lifeless, finished, expired, gone to meet
 * our maker, ceased to be, etcetc.  YASD.  It's a dead FS!
 */
void
puffs_userdead(struct puffs_mount *pmp)
{
	struct puffs_park *park;

	simple_lock(&pmp->pmp_lock);

	/*
	 * Mark filesystem status as dying so that operations don't
	 * attempt to march to userspace any longer.
	 */
	pmp->pmp_status = PUFFSTAT_DYING;

	/* and wakeup processes waiting for a reply from userspace */
	TAILQ_FOREACH(park, &pmp->pmp_req_replywait, park_entries) {
		park->park_rv = ENXIO;
		TAILQ_REMOVE(&pmp->pmp_req_replywait, park, park_entries);
		wakeup(park);
	}

	/* wakeup waiters for completion of vfs/vnode requests */
	TAILQ_FOREACH(park, &pmp->pmp_req_touser, park_entries) {
		park->park_rv = ENXIO;
		TAILQ_REMOVE(&pmp->pmp_req_touser, park, park_entries);
		wakeup(park);
	}

	simple_unlock(&pmp->pmp_lock);
}
