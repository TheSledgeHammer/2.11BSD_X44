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
#include <sys/cdefs.h>
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

#include <ufs/ufml/ufml.h>

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
	(nd) = UFMLVPTOLOWERVP(v.vnp)

/*
 * Undo the PUSHREF
 */
#define POP(v, nd) 						\
										\
	(nd) = v.vnp; 						\
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
int
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
int
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

	if ((error = VOP_GETATTR(UFMLVPTOLOWERVP(ap->a_vp), ap->a_vap, ap->a_cred, ap->a_p)))
		return (error);

	/* Requires that arguments be restored. */
	ap->a_vap->va_fsid = ap->a_vp->v_mount->mnt_stat.f_fsid.val[0];
	return (0);
}


int
ufml_open(ap)
	struct vop_open_args /* {
		struct vnode *a_vp;
		int  a_mode;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
#ifdef LOFS_DIAGNOSTIC
	printf("ufml_open(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_OPEN(UFMLVPTOLOWERVP(ap->a_vp), ap->a_mode, ap->a_cred, ap->a_p);
}

int
ufml_close(ap)
	struct vop_close_args /* {
		struct vnode *a_vp;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{
#ifdef LOFS_DIAGNOSTIC
	printf("ufml_close(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_CLOSE(UFMLVPTOLOWERVP(ap->a_vp), ap->a_fflag, ap->a_cred, ap->a_p);
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
	printf("ufml_access(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
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

int
ufml_read(ap)
	struct vop_read_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_read(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_READ(UFMLVPTOLOWERVP(ap->a_vp), ap->a_uio, ap->a_ioflag, ap->a_cred);
}

int
ufml_write(ap)
	struct vop_write_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		int  a_ioflag;
		struct ucred *a_cred;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_write(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_WRITE(UFMLVPTOLOWERVP(ap->a_vp), ap->a_uio, ap->a_ioflag, ap->a_cred);
}

int
ufml_ioctl(ap)
	struct vop_ioctl_args /* {
		struct vnode *a_vp;
		int  a_command;
		caddr_t  a_data;
		int  a_fflag;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_ioctl(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_IOCTL(UFMLVPTOLOWERVP(ap->a_vp), ap->a_command, ap->a_data, ap->a_fflag, ap->a_cred, ap->a_p);
}

int
ufml_select(ap)
	struct vop_select_args /* {
		struct vnode *a_vp;
		int  a_which;
		int  a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_select(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_SELECT(UFMLVPTOLOWERVP(ap->a_vp), ap->a_which, ap->a_fflags, ap->a_cred, ap->a_p);
}

int
ufml_poll(ap)
	struct vop_poll_args /* {
		struct vnode 	*a_vp;
		int 			a_fflags;
		int 			a_events;
		struct proc 	*a_p;
	} */ *ap;
{
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_poll(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_POLL(UFMLVPTOLOWERVP(ap->a_vp), ap->a_fflags, ap->a_events, ap->a_p);
}

int
ufml_mmap(ap)
	struct vop_mmap_args /* {
		struct vnode *a_vp;
		int  a_fflags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_mmap(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_MMAP(UFMLVPTOLOWERVP(ap->a_vp), ap->a_fflags, ap->a_cred, ap->a_p);
}

int
ufml_fsync(ap)
	struct vop_fsync_args /* {
		struct vnode *a_vp;
		struct ucred *a_cred;
		int  a_waitfor;
		int a_flags;
		struct proc *a_p;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_fsync(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_FSYNC(UFMLVPTOLOWERVP(ap->a_vp), ap->a_cred, ap->a_waitfor, ap->a_flags, ap->a_p);
}

int
ufml_seek(ap)
	struct vop_seek_args /* {
		struct vnode *a_vp;
		off_t  a_oldoff;
		off_t  a_newoff;
		struct ucred *a_cred;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_seek(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return VOP_SEEK(UFMLVPTOLOWERVP(ap->a_vp), ap->a_oldoff, ap->a_newoff, ap->a_cred);
}

int
ufml_remove(ap)
	struct vop_remove_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap;
{
	int error;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_remove(ap->a_vp = %x->%x)\n", ap->a_dvp, UFMLVPTOLOWERVP(ap->a_dvp));
#endif

	PUSHREF(xdvp, ap->a_dvp);
	VREF(ap->a_dvp);
	PUSHREF(xvp, ap->a_vp);
	VREF(ap->a_vp);

	error = VOP_REMOVE(ap->a_dvp, ap->a_vp, ap->a_cnp);

	POP(xvp, ap->a_vp);
	vrele(ap->a_vp);
	POP(xdvp, ap->a_dvp);
	vrele(ap->a_dvp);

	return (error);
}

/*
 * vp is this.
 * ni_dvp is the locked parent of the target.
 * ni_vp is NULL.
 */
int
ufml_link(ap)
	struct vop_link_args /* {
		struct vnode *a_vp;
		struct vnode *a_tdvp;
		struct componentname *a_cnp;
	} */ *ap;
{
	int error;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_link(ap->a_tdvp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	PUSHREF(xdvp, ap->a_vp);
	VREF(ap->a_vp);

	error = VOP_LINK(ap->a_vp, UFMLVPTOLOWERVP(ap->a_tdvp), ap->a_cnp);

	POP(xdvp, ap->a_vp);
	vrele(ap->a_vp);

	return (error);
}

int
ufml_rename(ap)
	struct vop_rename_args  /* {
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap;
{
	struct vnode *fvp, *tvp;
	struct vnode *tdvp;
#ifdef notdef
	struct vnode *fsvp, *tsvp;
#endif
	int error;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename(fdvp = %x->%x)\n", ap->a_fdvp, UFMLVPTOLOWERVP(ap->a_fdvp));
#endif

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - switch source dvp\n");
#endif
	/*
	 * Switch source directory to point to lofsed vnode
	 */
	PUSHREF(fdvp, ap->a_fdvp);
	VREF(ap->a_fdvp);

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - switch source vp\n");
#endif
	/*
	 * And source object if it is lofsed...
	 */
	fvp = ap->a_fvp;
	if (fvp && fvp->v_op == &ufml_vnodeops) {
		ap->a_fvp = UFMLVPTOLOWERVP(fvp);
		VREF(ap->a_fvp);
	} else {
		fvp = 0;
	}

#ifdef notdef
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - switch source start vp\n");
#endif
	/*
	 * And source startdir object if it is lofsed...
	 */
	fsvp = fndp->ni_startdir;
	if (fsvp && fsvp->v_op == &ufml_vnodeops) {
		fndp->ni_startdir = UFMLVPTOLOWERVP(fsvp);
		VREF(fndp->ni_startdir);
	} else {
		fsvp = 0;
	}
#endif

#ifdef LOFS_DIAGNOSTIC
	printf("ufml_rename - switch target dvp\n");
#endif
	/*
 	 * Switch target directory to point to lofsed vnode
	 */
	tdvp = ap->a_tdvp;
	if (tdvp && tdvp->v_op == &ufml_vnodeops) {
		ap->a_tdvp = UFMLVPTOLOWERVP(tdvp);
		VREF(ap->a_tdvp);
	} else {
		tdvp = 0;
	}

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - switch target vp\n");
#endif
	/*
	 * And target object if it is lofsed...
	 */
	tvp = ap->a_tvp;
	if (tvp && tvp->v_op == &ufml_vnodeops) {
		ap->a_tvp = UFMLVPTOLOWERVP(tvp);
		VREF(ap->a_tvp);
	} else {
		tvp = 0;
	}

#ifdef notdef
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - switch target start vp\n");
#endif
	/*
	 * And target startdir object if it is lofsed...
	 */
	tsvp = tndp->ni_startdir;
	if (tsvp && tsvp->v_op == &ufml_vnodeops) {
		tndp->ni_startdir = UFMLVPTOLOWERVP(fsvp);
		VREF(tndp->ni_startdir);
	} else {
		tsvp = 0;
	}
#endif

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - VOP_RENAME(%x, %x, %x, %x)\n", ap->a_fdvp, ap->a_fvp,
			ap->a_tdvp, ap->a_tvp);
	vprint("ap->a_fdvp", ap->a_fdvp);
	vprint("ap->a_fvp", ap->a_fvp);
	vprint("ap->a_tdvp", ap->a_tdvp);
	if (ap->a_tvp)
		vprint("ap->a_tvp", ap->a_tvp);
	DELAY(16000000);
#endif

	error = VOP_RENAME(ap->a_fdvp, ap->a_fvp, ap->a_fcnp, ap->a_tdvp, ap->a_tvp, ap->a_tcnp);

	/*
	 * Put everything back...
	 */

#ifdef notdef
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - restore target startdir\n");
#endif

	if (tsvp) {
		if (tndp->ni_startdir)
			vrele(tndp->ni_startdir);
		tndp->ni_startdir = tsvp;
	}
#endif

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - restore target vp\n");
#endif

	if (tvp) {
		ap->a_tvp = tvp;
		vrele(ap->a_tvp);
	}

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - restore target dvp\n");
#endif

	if (tdvp) {
		ap->a_tdvp = tdvp;
		vrele(ap->a_tdvp);
	}

#ifdef notdef
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - restore source startdir\n");
#endif

	if (fsvp) {
		if (fndp->ni_startdir)
			vrele(fndp->ni_startdir);
		fndp->ni_startdir = fsvp;
	}
#endif

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - restore source vp\n");
#endif


	if (fvp) {
		ap->a_fvp = fvp;
		vrele(ap->a_fvp);
	}

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rename - restore source dvp\n");
#endif

	POP(fdvp, ap->a_fdvp);
	vrele(ap->a_fdvp);

	return (error);
}

/*
 * ni_dvp is the locked (alias) parent.
 * ni_vp is NULL.
 */
int
ufml_mkdir(ap)
	struct vop_mkdir_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap;
{
	int error;
	struct vnode *dvp = ap->a_dvp;
	struct vnode *xdvp;
	struct vnode *newvp;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_mkdir(vp = %x->%x)\n", dvp, UFMLVPTOLOWERVP(dvp));
#endif

	xdvp = dvp;
	dvp = UFMLVPTOLOWERVP(xdvp);
	VREF(dvp);

	error = VOP_MKDIR(dvp, &newvp, ap->a_cnp, ap->a_vap);

	if (error) {
		*ap->a_vpp = NULLVP;
		vrele(xdvp);
		return (error);
	}

	/*
	 * Make a new lofs node
	 */
	/*VREF(dvp);*/

	error = ufml_node_create(dvp->v_mount, dvp, &newvp);

	*ap->a_vpp = newvp;

	return (error);
}

/*
 * ni_dvp is the locked parent.
 * ni_vp is the entry to be removed.
 */
int
ufml_rmdir(ap)
	struct vop_rmdir_args /* {
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = ap->a_dvp;
	int error;

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_rmdir(dvp = %x->%x)\n", dvp, UFMLVPTOLOWERVP(dvp));
#endif

	PUSHREF(xdvp, dvp);
	VREF(dvp);
	PUSHREF(xvp, vp);
	VREF(vp);

	error = VOP_RMDIR(dvp, vp, ap->a_cnp);

	POP(xvp, vp);
	vrele(vp);
	POP(xdvp, dvp);
	vrele(dvp);

	return (error);
}


/*
 * ni_dvp is the locked parent.
 * ni_vp is NULL.
 */
int
ufml_symlink(ap)
	struct vop_symlink_args /* {
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap;
{
	int error;

#ifdef UFMLFS_DIAGNOSTIC
	printf("VOP_SYMLINK(vp = %x->%x)\n", ap->a_dvp, UFMLVPTOLOWERVP(ap->a_dvp));
#endif

	PUSHREF(xdvp, ap->a_dvp);
	VREF(ap->a_dvp);

	error = VOP_SYMLINK(ap->a_dvp, ap->a_vpp, ap->a_cnp, ap->a_vap, ap->a_target);

	POP(xdvp, ap->a_dvp);
	vrele(ap->a_dvp);

	return (error);
}

int
ufml_readdir(ap)
	struct vop_readdir_args /* {
		struct vnode 			*a_vp;
		struct uio 				*a_uio;
		struct ucred 			*a_cred;
		int 					*a_eofflag;
		int 					*a_ncookies;
		u_long 					**a_cookies;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_readdir(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return (VOP_READDIR(UFMLVPTOLOWERVP(ap->a_vp), ap->a_uio, ap->a_cred, ap->a_eofflag, ap->a_ncookies, ap->a_cookies));
}

int
ufml_readlink(ap)
	struct vop_readlink_args /* {
		struct vnode *a_vp;
		struct uio *a_uio;
		struct ucred *a_cred;
	} */ *ap;
{

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_readlink(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return (VOP_READLINK(UFMLVPTOLOWERVP(ap->a_vp), ap->a_uio, ap->a_cred));
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
	/*vprint("ufml_lock ap->a_vp", ap->a_vp);
	if (targetvp)
		vprint("ufml_lock ->ap->a_vp", targetvp);
	else
		printf("ufml_lock ->ap->a_vp = NIL\n");*/
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
ufml_islocked(ap)
	struct vop_islocked_args /* {
		struct vnode *a_vp;
	} */ *ap;
{

	struct vnode *targetvp = UFMLVPTOLOWERVP(ap->a_vp);
	if (targetvp)
		return (VOP_ISLOCKED(targetvp));
	return (0);
}

/*
 * Anyone's guess...
 */
int
ufml_abortop(ap)
	struct vop_abortop_args /* {
		struct vnode *a_dvp;
		struct componentname *a_cnp;
	} */ *ap;
{
	int error;

	PUSHREF(xdvp, ap->a_dvp);

	error = VOP_ABORTOP(ap->a_dvp, ap->a_cnp);

	POP(xdvp, ap->a_dvp);

	return (error);
}

int
ufml_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap;
{
	struct vnode *targetvp = UFMLVPTOLOWERVP(ap->a_vp);

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
	if (targetvp) {
		vrele(targetvp);
		VTOUFML(ap->a_vp)->ufml_lowervp = 0;
	}
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
	LIST_REMOVE(xp, ufml_cache);
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
	register struct vnode *vp = UFMLVPTOLOWERVP(ap->a_vp);
	printf ("\ttag VT_UFMLFS, vp=%x, lowervp=%x\n", vp, UFMLVPTOLOWERVP(vp));
	if (vp)
		return (VOP_PRINT(vp));
	printf("NULLVP\n");
	return (0);
}

int
ufml_bmap(ap)
	struct vop_bmap_args /* {
		struct vnode 	*a_vp;
		daddr_t  		a_bn;
		struct vnode 	**a_vpp;
		daddr_t 		*a_bnp;
		int				a_runp;
	} */ *ap;
{
#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_bmap(ap->a_vp = %x->%x)\n", ap->a_vp, UFMLVPTOLOWERVP(ap->a_vp));
#endif

	return (VOP_BMAP(UFMLVPTOLOWERVP(ap->a_vp), ap->a_bn, ap->a_vpp, ap->a_bnp, ap->a_runp));
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

#ifdef UFMLFS_DIAGNOSTIC
	printf("ufml_strategy(vp = %x->%x)\n", ap->a_bp->b_vp, UFMLVPTOLOWERVP(ap->a_bp->b_vp));
#endif

	savedvp = bp->b_vp;
	bp->b_vp = UFMLVPTOLOWERVP(bp->b_vp);

	PUSHREF(vp, ap->a_bp->b_vp);

	error = VOP_STRATEGY(bp);

	POP(vp, ap->a_bp->b_vp);

	bp->b_vp = savedvp;

	return (error);
}

int
ufml_advlock(ap)
	struct vop_advlock_args /* {
		struct vnode *a_vp;
		caddr_t  a_id;
		int  a_op;
		struct flock *a_fl;
		int  a_flags;
	} */ *ap;
{

	return (VOP_ADVLOCK(UFMLVPTOLOWERVP(ap->a_vp), ap->a_id, ap->a_op, ap->a_fl, ap->a_flags));
}

/*
 * UFML directory offset lookup.
 * Currently unsupported.
 */
int
ufml_blkatoff(ap)
	struct vop_blkatoff_args /* {
		struct vnode *a_vp;
		off_t a_offset;
		char **a_res;
		struct buf **a_bpp;
	} */ *ap;
{

	return (VOP_BLKATOFF(UFMLVPTOLOWERVP(ap->a_vp), ap->a_offset, ap->a_res, ap->a_bpp));
}

/*
 * UFML flat namespace allocation.
 * Currently unsupported.
 */
int
ufml_valloc(ap)
	struct vop_valloc_args /* {
		struct vnode *a_pvp;
		int a_mode;
		struct ucred *a_cred;
		struct vnode **a_vpp;
	} */ *ap;
{

	return (VOP_VALLOC(UFMLVPTOLOWERVP(ap->a_pvp), ap->a_mode, ap->a_cred, ap->a_vpp));
}

/*
 * UFML flat namespace free.
 * Currently unsupported.
 */
/*void*/
int
ufml_vfree(ap)
	struct vop_vfree_args /* {
		struct vnode *a_pvp;
		ino_t a_ino;
		int a_mode;
	} */ *ap;
{

	return (VOP_VFREE(UFMLVPTOLOWERVP(ap->a_pvp), ap->a_ino, ap->a_mode));
}

/*
 * UFML file truncation.
 */
int
ufml_truncate(ap)
	struct vop_truncate_args /* {
		struct vnode *a_vp;
		off_t a_length;
		int a_flags;
		struct ucred *a_cred;
		struct proc *a_p;
	} */ *ap;
{

	/* Use lofs_setattr */
	//printf("lofs_truncate: need to implement!!");
	return (VOP_TRUNCATE(UFMLVPTOLOWERVP(ap->a_vp), ap->a_length, ap->a_flags, ap->a_cred, ap->a_p));
}

/*
 * UFML update.
 */
int
ufml_update(ap)
	struct vop_update_args /* {
		struct vnode *a_vp;
		struct timeval *a_access;
		struct timeval *a_modify;
		int a_waitfor;
	} */ *ap;
{

	/* Use lofs_setattr */
	//printf("lofs_update: need to implement!!");
	return (VOP_UPDATE(UFMLVPTOLOWERVP(ap->a_vp), ap->a_access, ap->a_modify, ap->a_waitfor));
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
 * UFML failed operation
 */
int
ufml_ebadf(void)
{

	return (EBADF);
}

/*
 * UFML bad operation
 */
int
ufml_badop(v)
	void *v;
{

	panic("ufml_badop called");
	/* NOTREACHED */
}

#define ufml_lease_check 	((int (*) (struct  vop_lease_args *))nullop)
#define	ufml_revoke 		((int (*) (struct  vop_revoke_args *))vop_norevoke)
#define ufml_pathconf 		((int (*) (struct  vop_pathconf_args *))ufml_badop)
#define ufml_reallocblks	((int (*) (struct  vop_reallocblks_args *))ufml_badop)

/*
 * Global vfs data structures ufml
 */
struct vnodeops ufml_vnodeops = {
		.vop_default = vop_default_error,/* default */
		.vop_lookup = ufml_lookup,		/* lookup */
		.vop_create = ufml_create,		/* create */
		.vop_mknod = ufml_mknod,		/* mknod */
		.vop_open = ufml_open,			/* open */
		.vop_close = ufml_close,		/* close */
		.vop_access = ufml_access,		/* access */
		.vop_getattr = ufml_getattr,	/* getattr */
		.vop_setattr = ufml_setattr,	/* setattr */
		.vop_read = ufml_read,			/* read */
		.vop_write = ufml_write,		/* write */
		.vop_lease = ufml_lease_check,	/* lease */
		.vop_ioctl = ufml_ioctl,		/* ioctl */
		.vop_select = ufml_select,		/* select */
		.vop_poll = ufml_poll,			/* poll */
		.vop_revoke = ufml_revoke,		/* revoke */
		.vop_mmap = ufml_mmap,			/* mmap */
		.vop_fsync = ufml_fsync,		/* fsync */
		.vop_seek = ufml_seek,			/* seek */
		.vop_remove = ufml_remove,		/* remove */
		.vop_link = ufml_link,			/* link */
		.vop_rename = ufml_rename,		/* rename */
		.vop_mkdir = ufml_mkdir,		/* mkdir */
		.vop_rmdir = ufml_rmdir,		/* rmdir */
		.vop_symlink = ufml_symlink,	/* symlink */
		.vop_readdir = ufml_readdir,	/* readdir */
		.vop_readlink = ufml_readlink,	/* readlink */
		.vop_abortop = ufml_abortop,	/* abortop */
		.vop_inactive = ufml_inactive,	/* inactive */
		.vop_reclaim = ufml_reclaim,	/* reclaim */
		.vop_lock = ufml_lock,			/* lock */
		.vop_unlock = ufml_unlock,		/* unlock */
		.vop_bmap = ufml_bmap,			/* bmap */
		.vop_strategy = ufml_strategy,	/* strategy */
		.vop_print = ufml_print,		/* print */
		.vop_islocked = ufml_islocked,	/* islocked */
		.vop_pathconf = ufml_pathconf,	/* pathconf */
		.vop_advlock = ufml_advlock,	/* advlock */
		.vop_blkatoff = ufml_blkatoff,	/* blkatoff */
		.vop_valloc = ufml_valloc,		/* valloc */
		.vop_reallocblks = ufml_reallocblks,/* reallocblks */
		.vop_vfree = ufml_vfree,		/* vfree */
		.vop_truncate = ufml_truncate,	/* truncate */
		.vop_update = ufml_update,		/* update */
		.vop_bwrite = ufml_bwrite,		/* bwrite */
};
