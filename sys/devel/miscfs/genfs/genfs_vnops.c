/*	$NetBSD: genfs_vnops.c,v 1.63.2.3 2003/08/26 06:46:25 tron Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: genfs_vnops.c,v 1.63.2.3 2003/08/26 06:46:25 tron Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/user.h>

#include <miscfs/genfs/genfs.h>
#include <miscfs/genfs/genfs_node.h>
#include <miscfs/specfs/specdev.h>

#include <vm/include/vm.h>
#include <vm/include/vm_pager.h>


static __inline void genfs_rel_pages(struct vm_page **, int);

#define MAX_READ_AHEAD	16 				/* XXXUBC 16 */
int genfs_rapages = 	MAX_READ_AHEAD; /* # of pages in each chunk of readahead */
int genfs_racount = 	2;				/* # of page chunks to readahead */
int genfs_raskip = 		2;				/* # of busy page chunks allowed to skip */

int
genfs_poll(v)
	void *v;
{
	struct vop_poll_args /* {
		struct vnode *a_vp;
		int a_events;
		struct proc *a_p;
	} */ *ap = v;

	return (ap->a_events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

int
genfs_fsync(v)
	void *v;
{
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;
	register struct vnode *vp = ap->a_vp;
	int wait;

	wait = (ap->a_flags & FSYNC_WAIT) != 0;
	vflushbuf(vp, wait);
	if ((ap->a_flags & FSYNC_DATAONLY) != 0)
		return (0);
	else
		return (VOP_UPDATE(ap->a_vp, NULL, NULL, wait));
}

int
genfs_seek(v)
	void *v;
{
	struct vop_seek_args /* {
		struct vnode *a_vp;
		off_t a_oldoff;
		off_t a_newoff;
		struct ucred *a_ucred;
	} */ *ap = v;

	if (ap->a_newoff < 0)
		return (EINVAL);

	return (0);
}

int
genfs_abortop(v)
	void *v;
{
	struct vop_abortop_args /* {
		struct vnode *a_dvp;
		struct componentname *a_cnp;
	} */ *ap = v;

	if ((ap->a_cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF)
		FREE(ap->a_cnp->cn_pnbuf, M_NAMEI);
	return (0);
}

int
genfs_fcntl(void *v)
{
	struct vop_fcntl_args /* {
		struct vnode *a_vp;
		u_int a_command;
		caddr_t a_data;
		int a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap = v;

	if (ap->a_command == F_SETFL)
		return (0);
	else
		return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
genfs_badop(v)
	void *v;
{

	panic("genfs: bad op");
}

/*ARGSUSED*/
int
genfs_nullop(v)
	void *v;
{

	return (0);
}

/*ARGSUSED*/
int
genfs_einval(v)
	void *v;
{

	return (EINVAL);
}

/*ARGSUSED*/
int
genfs_eopnotsupp(v)
	void *v;
{

	return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
genfs_ebadf(v)
	void *v;
{

	return (EBADF);
}

/* ARGSUSED */
int
genfs_enoioctl(v)
	void *v;
{

	return (ENOTTY);
}


/*
 * Eliminate all activity associated with  the requested vnode
 * and with all vnodes aliased to the requested vnode.
 */
int
genfs_revoke(v)
	void *v;
{
	struct vop_revoke_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp, *vq;
	struct proc *p = curproc;	/* XXX */

#ifdef DIAGNOSTIC
	if ((ap->a_flags & REVOKEALL) == 0)
		panic("genfs_revoke: not revokeall");
#endif

	vp = ap->a_vp;
	simple_lock(&vp->v_interlock);

	if (vp->v_flag & VALIASED) {
		/*
		 * If a vgone (or vclean) is already in progress,
		 * wait until it is done and return.
		 */
		if (vp->v_flag & VXLOCK) {
			vp->v_flag |= VXWANT;
			simple_unlock(&vp->v_interlock);
			tsleep((caddr_t) vp, PINOD, "vop_revokeall", 0);
			return (0);
		}
		/*
		 * Ensure that vp will not be vgone'd while we
		 * are eliminating its aliases.
		 */
		vp->v_flag |= VXLOCK;
		simple_unlock(&vp->v_interlock);
		while (vp->v_flag & VALIASED) {
			simple_lock(&spechash_slock);
			for (vq = *vp->v_hashchain; vq; vq = vq->v_specnext) {
				if (vq->v_rdev != vp->v_rdev || vq->v_type != vp->v_type
						|| vp == vq)
					continue;
				simple_unlock(&spechash_slock);
				vgone(vq);
				break;
			}
			if (vq == NULLVP)
				simple_unlock(&spechash_slock);
		}
		/*
		 * Remove the lock so that vgone below will
		 * really eliminate the vnode after which time
		 * vgone will awaken any sleepers.
		 */
		simple_lock(&vp->v_interlock);
		vp->v_flag &= ~VXLOCK;
	}
	vgonel(vp, p);
	return (0);
}

/*
 * Lock the node.
 */
int
genfs_lock(void *v)
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct proc *p = ap->a_p;

	return (lockmgr(&vp->v_vnlock, ap->a_flags, &vp->v_interlock, p->p_pid));
}

/*
 * Unlock the node.
 */
int
genfs_unlock(void *v)
{
	struct vop_unlock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct proc *p = ap->a_p;

	return (lockmgr(&vp->v_vnlock, ap->a_flags | LK_RELEASE, &vp->v_interlock, p->p_pid));
}

/*
 * Return whether or not the node is locked.
 */
int
genfs_islocked(void *v)
{
	struct vop_islocked_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;

	return (lockstatus(&vp->v_vnlock));
}

/*
 * Stubs to use when there is no locking to be done on the underlying object.
 */
int
genfs_nolock(void *v)
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap = v;

	/*
	 * Since we are not using the lock manager, we must clear
	 * the interlock here.
	 */
	if (ap->a_flags & LK_INTERLOCK)
		simple_unlock(&ap->a_vp->v_interlock);
	return (0);
}

int
genfs_nounlock(void *v)
{

	return (0);
}

int
genfs_noislocked(void *v)
{

	return (0);
}


/*
 * Local lease check for NFS servers.  Just set up args and let
 * nqsrv_getlease() do the rest.  If NFSSERVER is not in the kernel,
 * this is a null operation.
 */
int
genfs_lease_check(v)
	void *v;
{
#ifdef NFSSERVER
	struct vop_lease_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
		struct ucred *a_cred;
		int a_flag;
	} */ *ap = v;
	u_int32_t duration = 0;
	int cache;
	u_quad_t frev;

	(void) nqsrv_getlease(ap->a_vp, &duration, ND_CHECK | ap->a_flag,
	    NQLOCALSLP, ap->a_p, (struct mbuf *)0, &cache, &frev, ap->a_cred);
	return (0);
#else
	return (0);
#endif /* NFSSERVER */
}

int
genfs_mmap(void *v)
{
	return (0);
}

void
genfs_node_init(struct vnode *vp, struct genfs_ops *ops)
{
	struct genfs_node *gp = VTOG(vp);

	lockinit(&gp->g_glock, PINOD, "glock", 0, 0);
	gp->g_op = ops;
}
