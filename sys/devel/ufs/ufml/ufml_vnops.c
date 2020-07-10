/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * John Heidemann of the UCLA Ficus project.
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
 *	@(#)null_vnops.c	8.6 (Berkeley) 5/27/95
 *
 * Ancestors:
 *	@(#)lofs_vnops.c	1.2 (Berkeley) 6/18/92
 *	$Id: lofs_vnops.c,v 1.11 1992/05/30 10:05:43 jsp Exp jsp $
 *	...and...
 *	@(#)null_vnodeops.c 1.20 92/07/07 UCLA Ficus project
 */

/*
 * Null Layer
 *
 * (See mount_null(8) for more information.)
 *
 * The null layer duplicates a portion of the file system
 * name space under a new name.  In this respect, it is
 * similar to the loopback file system.  It differs from
 * the loopback fs in two respects:  it is implemented using
 * a stackable layers techniques, and it's "null-node"s stack above
 * all lower-layer vnodes, not just over directory vnodes.
 *
 * The null layer has two purposes.  First, it serves as a demonstration
 * of layering by proving a layer which does nothing.  (It actually
 * does everything the loopback file system does, which is slightly
 * more than nothing.)  Second, the null layer can serve as a prototype
 * layer.  Since it provides all necessary layer framework,
 * new file system layers can be created very easily be starting
 * with a null layer.
 *
 * The remainder of this man page examines the null layer as a basis
 * for constructing new layers.
 *
 *
 * INSTANTIATING NEW NULL LAYERS
 *
 * New null layers are created with mount_null(8).
 * Mount_null(8) takes two arguments, the pathname
 * of the lower vfs (target-pn) and the pathname where the null
 * layer will appear in the namespace (alias-pn).  After
 * the null layer is put into place, the contents
 * of target-pn subtree will be aliased under alias-pn.
 *
 *
 * OPERATION OF A NULL LAYER
 *
 * The null layer is the minimum file system layer,
 * simply bypassing all possible operations to the lower layer
 * for processing there.  The majority of its activity centers
 * on the bypass routine, though which nearly all vnode operations
 * pass.
 *
 * The bypass routine accepts arbitrary vnode operations for
 * handling by the lower layer.  It begins by examing vnode
 * operation arguments and replacing any null-nodes by their
 * lower-layer equivlants.  It then invokes the operation
 * on the lower layer.  Finally, it replaces the null-nodes
 * in the arguments and, if a vnode is return by the operation,
 * stacks a null-node on top of the returned vnode.
 *
 * Although bypass handles most operations, vop_getattr, vop_lock,
 * vop_unlock, vop_inactive, vop_reclaim, and vop_print are not
 * bypassed. Vop_getattr must change the fsid being returned.
 * Vop_lock and vop_unlock must handle any locking for the
 * current vnode as well as pass the lock request down.
 * Vop_inactive and vop_reclaim are not bypassed so that
 * they can handle freeing null-layer specific data. Vop_print
 * is not bypassed to avoid excessive debugging information.
 * Also, certain vnode operations change the locking state within
 * the operation (create, mknod, remove, link, rename, mkdir, rmdir,
 * and symlink). Ideally these operations should not change the
 * lock state, but should be changed to let the caller of the
 * function unlock them. Otherwise all intermediate vnode layers
 * (such as union, umapfs, etc) must catch these functions to do
 * the necessary locking at their layer.
 *
 *
 * INSTANTIATING VNODE STACKS
 *
 * Mounting associates the null layer with a lower layer,
 * effect stacking two VFSes.  Vnode stacks are instead
 * created on demand as files are accessed.
 *
 * The initial mount creates a single vnode stack for the
 * root of the new null layer.  All other vnode stacks
 * are created as a result of vnode operations on
 * this or other null vnode stacks.
 *
 * New vnode stacks come into existance as a result of
 * an operation which returns a vnode.  
 * The bypass routine stacks a null-node above the new
 * vnode before returning it to the caller.
 *
 * For example, imagine mounting a null layer with
 * "mount_null /usr/include /dev/layer/null".
 * Changing directory to /dev/layer/null will assign
 * the root null-node (which was created when the null layer was mounted).
 * Now consider opening "sys".  A vop_lookup would be
 * done on the root null-node.  This operation would bypass through
 * to the lower layer which would return a vnode representing 
 * the UFS "sys".  Null_bypass then builds a null-node
 * aliasing the UFS "sys" and returns this to the caller.
 * Later operations on the null-node "sys" will repeat this
 * process when constructing other vnode stacks.
 *
 *
 * CREATING OTHER FILE SYSTEM LAYERS
 *
 * One of the easiest ways to construct new file system layers is to make
 * a copy of the null layer, rename all files and variables, and
 * then begin modifing the copy.  Sed can be used to easily rename
 * all variables.
 *
 * The umap layer is an example of a layer descended from the 
 * null layer.
 *
 *
 * INVOKING OPERATIONS ON LOWER LAYERS
 *
 * There are two techniques to invoke operations on a lower layer 
 * when the operation cannot be completely bypassed.  Each method
 * is appropriate in different situations.  In both cases,
 * it is the responsibility of the aliasing layer to make
 * the operation arguments "correct" for the lower layer
 * by mapping an vnode arguments to the lower layer.
 *
 * The first approach is to call the aliasing layer's bypass routine.
 * This method is most suitable when you wish to invoke the operation
 * currently being hanldled on the lower layer.  It has the advantage
 * that the bypass routine already must do argument mapping.
 * An example of this is null_getattrs in the null layer.
 *
 * A second approach is to directly invoked vnode operations on
 * the lower layer with the VOP_OPERATIONNAME interface.
 * The advantage of this method is that it is easy to invoke
 * arbitrary operations on the lower layer.  The disadvantage
 * is that vnodes arguments must be manualy mapped.
 *
 */

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
#include <sys/buf.h>
#include "ufs/ufml/ufml.h"

/*
 * Basic strategy: as usual, do as little work as possible.
 * Nothing is ever locked in the lofs'ed filesystem, all
 * locks are held in the underlying filesystems.
 */

/*
 * Save a vnode and replace with
 * the lofs'ed one
 */
#define PUSHREF(v, nd) 					\
{ 										\
	struct { struct vnode *vnp; } v; 	\
	v.vnp = (nd); 						\
	(nd) = LOFSVP(v.vnp)

/*
 * Undo the PUSHREF
 */
#define POP(v, nd) 	\
					\
	(nd) = v.vnp; 	\
}

int
ufml_lookup(ap)
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

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_lookup(ap->a_dvp = %x->%x, \"%s\", op = %d)\n",
		dvp, UFMLVPTOLOWERVP(dvp), ap->a_cnp->cn_nameptr, flag);
#endif

	if ((flags & ISLASTCN) && (ap->a_dvp->v_mount->mnt_flag & MNT_RDONLY)
			&& (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME))
		return (EROFS);
	/*
	 * (ap->a_dvp) was locked when passed in, and it will be replaced
	 * with the target vnode, BUT that will already have been
	 * locked when (ap->a_dvp) was locked [see lofs_lock].  all that
	 * must be done here is to keep track of reference counts.
	 */
	targetdvp = UFMLVPTOLOWERVP(dvp);

#ifdef UFMLFS_DIAGNOSTIC
	vprint("ufml VOP_LOOKUP", targetdvp);
#endif

	error = VOP_LOOKUP(targetdvp, &newvp, ap->a_cnp);
	if (error) {
		*ap->a_vpp = NULLVP;
#ifdef UFMLFS_DIAGNOSTIC
		printf("ufml_lookup(%x->%x) = %d\n", dvp, UFMLVPTOLOWERVP(dvp), error);
#endif
	} else if (error == EJUSTRETURN && (flags & ISLASTCN)
			&& (ap->a_dvp->v_mount->mnt_flag & MNT_RDONLY)
			&& (cnp->cn_nameiop == CREATE || cnp->cn_nameiop == RENAME)) {
		error = EROFS;
	}
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_lookup(%x->%x) = OK\n", dvp, UFMLVPTOLOWERVP(dvp));
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
#ifdef UFMLFS_DIAGNOSTIC
		printf("ufml_lookup: found VDIR\n");
#endif
		/*
		 * At this point, newvp is the vnode to be looped.
		 * Activate a loopback and return the looped vnode.
		 */
		return (ufml_node_create(dvp->v_mount, targetdvp, ap->a_vpp));
	}

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_lookup: not VDIR\n");
#endif

	return (error);
}

/*
 * this = ni_dvp
 * ni_dvp references the locked directory.
 * ni_vp is NULL.
 */
ufml_mknod(ap)
	struct vop_mknod_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	int error;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_mknod(vp = %x->%x)\n", ap->a_dvp, UFMLVPTOLOWERVP(ap->a_dvp));
#endif

	PUSHREF(xdvp, ap->a_dvp);
	VREF(ap->a_dvp);

	error = VOP_MKNOD(ap->a_dvp, ap->a_vpp, ap->a_cnp, ap->a_vap);

	POP(xdvp, ap->a_dvp);
	vrele(ap->a_dvp);

	return (error);
}

/*
 * this = ni_dvp;
 * ni_dvp references the locked directory
 * ni_vp is NULL.
 */
ufml_create(ap)
	struct vop_create_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	int error;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_create(ap->a_dvp = %x->%x)\n", ap->a_dvp, UFMLVPTOLOWERVP(ap->a_dvp));
#endif

	PUSHREF(xdvp, ap->a_dvp);
	VREF(ap->a_dvp);

	error = VOP_CREATE(ap->a_dvp, ap->a_vpp, ap->a_cnp, ap->a_vap);

	POP(xdvp, ap->a_dvp);
	vrele(ap->a_dvp);

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_create(ap->a_dvp = %x->%x)\n", ap->a_dvp, UFMLVPTOLOWERVP(ap->a_dvp));
#endif

	return (error);
}

/*
 * Setattr call. Disallow write attempts if the layer is mounted read-only.
 */
int
ufml_setattr(ap)
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

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_setattr(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

  	if ((vap->va_flags != VNOVAL || vap->va_uid != (uid_t)VNOVAL ||
	    vap->va_gid != (gid_t)VNOVAL || vap->va_atime.tv_sec != VNOVAL ||
	    vap->va_mtime.tv_sec != VNOVAL || vap->va_mode != (mode_t)VNOVAL) &&
	    (vp->v_mount->mnt_flag & MNT_RDONLY))
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
	error = VOP_SETATTR(UFMLVPTOLOWERVP(vp), vap, ap->a_cred, ap->a_p);
	if(error) {
		return (error);
	}
	return (0);
}

/*
 *  We handle getattr only to change the fsid.
 */
int
ufml_getattr(ap)
	struct vop_getattr_args /* {
		struct vnode *a_vp;
		struct vattr *a_vap;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
	int error;

	if (error == VOP_GETATTR(UFMLVPTOLOWERVP(ap->a_vp), ap->a_vap, ap->a_cred, ap->a_p))
		return (error);

	/* Requires that arguments be restored. */
	ap->a_vap->va_fsid = ap->a_vp->v_mount->mnt_stat.f_fsid.val[0];
	return (0);
}

int
ufml_access(ap)
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
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_access(ap->a_vp = %x->%x)\n", ap->a_vp, LOFSVP(ap->a_vp));
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

	error = VOP_ACCESS(UFMLVPTOLOWERVP(ap->a_vp), ap->a_mode, ap->a_cred, ap->a_p);

	if (error)
		return (error);

	return (0);
}

/*
 * We need to process our own vnode lock and then clear the
 * interlock flag as it applies only to our vnode, not the
 * vnodes below us on the stack.
 */
int
ufml_lock(ap)
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap;
{
	int error;
	struct vnode *targetvp = UFMLVPTOLOWERVP(ap->a_vp);
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_lock(ap->a_vp = %x->%x)\n", ap->a_vp, targetvp);
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

/*
 * We need to process our own vnode unlock and then clear the
 * interlock flag as it applies only to our vnode, not the
 * vnodes below us on the stack.
 */
int
ufml_unlock(ap)
	struct vop_unlock_args /* {
		struct vnode *a_vp;
		int a_flags;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *targetvp = UFMLVPTOLOWERVP(ap->a_vp);
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_unlock(ap->a_vp = %x->%x)\n", ap->a_vp, targetvp);
#endif

	vop_nounlock(ap);
	ap->a_flags &= ~LK_INTERLOCK;

	if (targetvp)
		return (VOP_UNLOCK(targetvp, 0, ap->a_p));
	return (0);
}

int
ufml_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap;
{
	/*
	 * Do nothing (and _don't_ bypass).
	 * Wait to vrele lowervp until reclaim,
	 * so that until then our null_node is in the
	 * cache and reusable.
	 *
	 * NEEDSWORK: Someday, consider inactive'ing
	 * the lowervp and then trying to reactivate it
	 * with capabilities (v_id)
	 * like they do in the name lookup cache code.
	 * That's too much work for now.
	 */
	VOP_UNLOCK(ap->a_vp, 0, ap->a_p);
	return (0);
}

int
ufml_reclaim(ap)
	struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct ufml_node *xp = VTOUFML(vp);
	struct vnode *lowervp = xp->ufml_lowervp;

	/*
	 * Note: in vop_reclaim, vp->v_op == dead_vnodeop_p,
	 * so we can't call VOPs on ourself.
	 */
	/* After this assignment, this node will not be re-used. */
	xp->ufml_lowervp = NULL;
	LIST_REMOVE(xp, ufml_hash);
	FREE(vp->v_data, M_TEMP);
	vp->v_data = NULL;
	vrele (lowervp);
	return (0);
}

int
ufml_print(ap)
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap;
{
	register struct vnode *vp = ap->a_vp;
	printf ("\ttag VT_UFMLFS, vp=%x, lowervp=%x\n", vp, UFMLVPTOLOWERVP(vp));
	return (0);
}

/*
 * XXX - vop_strategy must be hand coded because it has no
 * vnode in its arguments.
 * This goes away with a merged VM/buffer cache.
 */
int
ufml_strategy(ap)
	struct vop_strategy_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	struct buf *bp = ap->a_bp;
	int error;
	struct vnode *savedvp;

	savedvp = bp->b_vp;
	bp->b_vp = UFMLVPTOLOWERVP(bp->b_vp);

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
ufml_bwrite(ap)
	struct vop_bwrite_args /* {
		struct buf *a_bp;
	} */ *ap;
{
	struct buf *bp = ap->a_bp;
	int error;
	struct vnode *savedvp;

	savedvp = bp->b_vp;
	bp->b_vp = UFMLVPTOLOWERVP(bp->b_vp);

	error = VOP_BWRITE(bp);

	bp->b_vp = savedvp;

	return (error);
}

/*
 * Global vfs data structures ufml
 */
struct vnodeops ufml_vnodeops = {
	.vop_lookup_desc = ufml_lookup,
	.vop_setattr_desc = ufml_setattr,
	.vop_getattr_desc = ufml_getattr,
	.vop_access_desc = ufml_access,
	.vop_lock_desc = ufml_lock,
	.vop_unlock_desc = ufml_unlock,
	.vop_inactive_desc = ufml_inactive,
	.vop_reclaim_desc = ufml_reclaim,
	.vop_print_desc = ufml_print,
	.vop_strategy_desc = ufml_strategy,
	.vop_bwrite_desc = ufml_bwrite,
	(struct vnodeops*)NULL, (int(*)())NULL
};
