/*
 * Copyright (c) 1992 The Regents of the University of California
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 * All rights reserved.
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
 *	@(#)lofs_vnops.c	7.2 (Berkeley) 7/15/92
 *
 * $Id: lofs_vnops.c,v 1.11 1992/05/30 10:05:43 jsp Exp jsp $
 */

/*
 * Loopback Filesystem
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <miscfs/lofs/lofs.h>

/*
 * Basic strategy: as usual, do as little work as possible.
 * Nothing is ever locked in the lofs'ed filesystem, all
 * locks are held in the underlying filesystems.
 */

/*
 * Save a vnode and replace with
 * the lofs'ed one
 */
#define PUSHREF(v, nd) \
{ \
	struct { struct vnode *vnp; } v; \
	v.vnp = (nd); \
	(nd) = LOFSVP(v.vnp)

/*
 * Undo the PUSHREF
 */
#define POP(v, nd) \
	\
	(nd) = v.vnp; \
}

int
lofs_lookup(ap)
	struct vop_lookup_args /* {
		struct vnode * a_dvp;
		struct vnode ** a_vpp;
		struct componentname * a_cnp;
	} */ *ap;
{
	struct componentname *cnp = ap->a_cnp;
	struct proc *p = cnp->cn_proc;
	int flags = cnp->cn_flags;
	struct vop_lock_args lockargs;
	struct vop_unlock_args unlockargs;
	struct vnode *dvp, *vp;
	struct vnode *newvp;
	struct vnode *targetdvp;
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_lookup(ap->a_dvp = %x->%x, \"%s\", op = %d)\n",
		dvp, LOFSVP(dvp), ap->a_cnp->cn_nameptr, flag);
#endif

	if ((flags & ISLASTCN) && (ap->a_dvp->v_mount->mnt_flag & MNT_RDONLY) &&
		    (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME))
			return (EROFS);
	/*
	 * (ap->a_dvp) was locked when passed in, and it will be replaced
	 * with the target vnode, BUT that will already have been
	 * locked when (ap->a_dvp) was locked [see lofs_lock].  all that
	 * must be done here is to keep track of reference counts.
	 */
	targetdvp = LOFSVP(dvp);

#ifdef LOFS_DIAGNOSTIC
	vprint("lofs VOP_LOOKUP", targetdvp);
#endif

	error = VOP_LOOKUP(targetdvp, &newvp, ap->a_cnp);
	if(error) {
		*ap->a_vpp = NULLVP;
#ifdef LOFS_DIAGNOSTIC
		printf("lofs_lookup(%x->%x) = %d\n", dvp, LOFSVP(dvp), error);
#endif
	} else if (error == EJUSTRETURN && (flags & ISLASTCN) && (ap->a_dvp->v_mount->mnt_flag & MNT_RDONLY) && (cnp->cn_nameiop == CREATE || cnp->cn_nameiop == RENAME)) {
		error = EROFS;
	}
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_lookup(%x->%x) = OK\n", dvp, LOFSVP(dvp));
#endif
	/*
	 * We must do the same locking and unlocking at this layer as
	 * is done in the layers below us. We could figure this out
	 * based on the error return and the LASTCN, LOCKPARENT, and
	 * LOCKLEAF flags. However, it is more expidient to just find
	 * out the state of the lower level vnodes and set ours to the
	 * same state.
	 */

	*ap->a_vpp = newvp;
	dvp = ap->a_dvp;
	vp = *ap->a_vpp;

	if (dvp == vp)
		return (error);
	if (!VOP_ISLOCKED(dvp)) {
		unlockargs.a_vp = dvp;
		unlockargs.a_flags = 0;
		unlockargs.a_p = p;
		vop_nounlock(&unlockargs);
	}
	if (vp != NULL && VOP_ISLOCKED(vp)) {
		lockargs.a_vp = vp;
		lockargs.a_flags = LK_SHARED;
		lockargs.a_p = p;
		vop_nolock(&lockargs);
	}

	/*
	 * If we just found a directory then make
	 * a loopback node for it and return the loopback
	 * instead of the real vnode.  Otherwise simply
	 * return the aliased directory and vnode.
	 */
	if (newvp && newvp->v_type == VDIR && flags == LOOKUP) {
#ifdef LOFS_DIAGNOSTIC
		printf("lofs_lookup: found VDIR\n");
#endif
		/*
		 * At this point, newvp is the vnode to be looped.
		 * Activate a loopback and return the looped vnode.
		 */
		return (make_lofs(dvp->v_mount, ap->a_vpp));
	}

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_lookup: not VDIR\n");
#endif

	return (error);
}

int
lofs_setattr(ap)
	struct vop_setattr_args /* {
		struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct vattr *vap = ap->a_vap;
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_setattr(ap->a_vp = %x->%x)\n", ap->a_vp, LOFSVP(ap->a_vp));
#endif

	if ((vap->va_flags != VNOVAL || vap->va_uid != (uid_t) VNOVAL
			|| vap->va_gid != (gid_t) VNOVAL || vap->va_atime.tv_sec != VNOVAL
			|| vap->va_mtime.tv_sec != VNOVAL || vap->va_mode != (mode_t) VNOVAL)
			&& (vp->v_mount->mnt_flag & MNT_RDONLY))
		return (EROFS);
	if (vap->va_size != VNOVAL) {
		switch (vp->v_type) {
		case VDIR:
			return (EISDIR);
		case VCHR:
		case VBLK:
		case VSOCK:
		case VFIFO:
			return (0);
		case VREG:
		case VLNK:
		default:
			/*
			 * Disallow write attempts if the filesystem is
			 * mounted read-only.
			 */
			if (vp->v_mount->mnt_flag & MNT_RDONLY)
				return (EROFS);
		}
	}
	error = VOP_SETATTR(LOFSVP(vp), vap, ap->a_cred, ap->a_p);
	if(error) {
		return (error);
	}

	return (0);
}

/*
 *  We handle getattr only to change the fsid.
 */
int
lofs_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	int error;

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_getattr(ap->a_vp = %x->%x)\n", ap->a_vp, LOFSVP(ap->a_vp));
#endif

	/*
	 * Get the stats from the underlying filesystem
	 */
	error = VOP_GETATTR(LOFSVP(ap->a_vp), ap->a_vap, ap->a_cred, ap->a_p);
	if (error)
		return (error);

	/* Requires that arguments be restored. */
	ap->a_vap->va_fsid = ap->a_vp->v_mount->mnt_stat.f_fsid.val[0];
	return (0);
}

int
lofs_access(ap)
	struct vop_access_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	mode_t mode = ap->a_mode;
	int error;
#ifdef LOFS_DIAGNOSTIC
	printf("lofs_access(ap->a_vp = %x->%x)\n", ap->a_vp, LOFSVP(ap->a_vp));
#endif

	/*
	 * Disallow write attempts on read-only layers;
	 * unless the file is a socket, fifo, or a block or
	 * character device resident on the file system.
	 */
	if (mode & VWRITE) {
		switch (vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			if (vp->v_mount->mnt_flag & MNT_RDONLY)
				return (EROFS);
			break;
		}
	}

	error = VOP_ACCESS(LOFSVP(ap->a_vp), ap->a_mode, ap->a_cred, ap->a_p);

	if (error)
		return (error);

	return (0);
}

int
lofs_lock(ap)
	struct vop_lock_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	int error;
	struct vnode *targetvp = LOFSVP(ap->a_vp);

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_lock(ap->a_vp = %x->%x)\n", ap->a_vp, targetvp);
	/*vprint("lofs_lock ap->a_vp", ap->a_vp);
	if (targetvp)
		vprint("lofs_lock ->ap->a_vp", targetvp);
	else
		printf("lofs_lock ->ap->a_vp = NIL\n");*/
#endif

	vop_nolock(ap);
	if ((ap->a_flags & LK_TYPE_MASK) == LK_DRAIN)
		return (0);
	ap->a_flags &= ~LK_INTERLOCK;

	if (targetvp) {
		error = VOP_LOCK(targetvp, 0, ap->a_p);
		if (error)
			return (error);
	}

	return (0);
}

int
lofs_unlock(ap)
	struct vop_unlock_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	struct vnode *targetvp = LOFSVP(ap->a_vp);

#ifdef LOFS_DIAGNOSTIC
	printf("lofs_unlock(ap->a_vp = %x->%x)\n", ap->a_vp, targetvp);
#endif

	vop_nounlock(ap);
	ap->a_flags &= ~LK_INTERLOCK;

	if (targetvp)
		return (VOP_UNLOCK(targetvp, 0, ap->a_p));
	return (0);
}

/*
 * XXX - vop_strategy must be hand coded because it has no
 * vnode in its arguments.
 * This goes away with a merged VM/buffer cache.
 */
int
lofs_strategy(ap)
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	struct buf *bp = ap->a_bp;
	int error;
	struct vnode *savedvp;

	savedvp = bp->b_vp;
	bp->b_vp = NULLVPTOLOWERVP(bp->b_vp);

	error = VOP_STRATEGY(bp);

	bp->b_vp = savedvp;

	return (error);
}

/*
 * XXX - like vop_strategy, vop_bwrite must be hand coded because it has no
 * vnode in its arguments.
 * This goes away with a merged VM/buffer cache.
 */
int
lofs_bwrite(ap)
	struct vop_bwrite_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	struct buf *bp = ap->a_bp;
	int error;
	struct vnode *savedvp;

	savedvp = bp->b_vp;
	bp->b_vp = NULLVPTOLOWERVP(bp->b_vp);

	error = VOP_BWRITE(bp);

	bp->b_vp = savedvp;

	return (error);
}
