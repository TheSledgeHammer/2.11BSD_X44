/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)vnode.h	7.39 (Berkeley) 6/27/91
 */

struct vnodeops {
	int	(*vop_lookup)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp);
	int	(*vop_create)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
	int (*vop_whiteout)		(struct vnode *dvp, struct componentname *cnp, int flags);
	int	(*vop_mknod)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
	int	(*vop_open)	    	(struct vnode *vp, int mode, struct ucred *cred, struct proc *p);
	int	(*vop_close)		(struct vnode *vp, int fflag, struct ucred *cred, struct proc *p);
	int	(*vop_access)		(struct vnode *vp, int mode, struct ucred *cred, struct proc *p);
	int	(*vop_getattr)		(struct vnode *vp, struct vattr *vap, struct ucred *cred, struct proc *p);
	int	(*vop_setattr)		(struct vnode *vp, struct vattr *vap, struct ucred *cred, struct proc *p);
	int	(*vop_read)	    	(struct vnode *vp, struct uio *uio, int ioflag, struct ucred *cred);
	int	(*vop_write)		(struct vnode *vp, struct uio *uio, int ioflag, struct ucred *cred);
	int (*vop_lease)		(struct vnode *vp, struct proc *p, struct ucred *cred, int flag);
	int	(*vop_ioctl)		(struct vnode *vp, u_long command, caddr_t data, int fflag, struct ucred *cred, struct proc *p);
	int	(*vop_select)		(struct vnode *vp, int which, int fflags, struct ucred *cred, struct proc *p);
	int (*vop_revoke)		(struct vnode *vp, int flags);
	int	(*vop_mmap)	    	(struct vnode *vp, int fflags, struct ucred *cred, struct proc *p);
	int	(*vop_fsync)		(struct vnode *vp, int fflags, struct ucred *cred, int waitfor, struct proc *p);
	int	(*vop_seek)	    	(struct vnode *vp, off_t oldoff, off_t newoff, struct ucred *cred);
	int	(*vop_remove)		(struct vnode *dvp, struct vnode *vp, struct componentname *cnp);
	int	(*vop_link)	    	(struct vnode *vp, struct vnode *tdvp, struct componentname *cnp);
	int	(*vop_rename)		(struct vnode *fdvp, struct vnode *fvp, struct componentname *fcnp, struct vnode *tdvp, struct vnode *tvp, struct componentname *tcnp);
	int	(*vop_mkdir)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap);
	int	(*vop_rmdir)		(struct vnode *dvp, struct vnode *vp, struct componentname *cnp);
	int	(*vop_symlink)		(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp, struct vattr *vap, char *target);
	int	(*vop_readdir)		(struct vnode *vp, struct uio *uio, struct ucred *cred, int *eofflag, int *ncookies, u_long **cookies);
	int	(*vop_readlink)		(struct vnode *vp, struct uio *uio, struct ucred *cred);
	int	(*vop_abortop)		(struct vnode *dvp, struct componentname *cnp);
	int	(*vop_inactive)		(struct vnode *vp, struct proc *p);
	int	(*vop_reclaim)		(struct vnode *vp, struct proc *p);
	int	(*vop_lock)	    	(struct vnode *vp, int flags, struct proc *p);
	int	(*vop_unlock)		(struct vnode *vp, int flags, struct proc *p);
	int	(*vop_bmap)	    	(struct vnode *vp, daddr_t bn, struct vnode **vpp, daddr_t *bnp, int *runp);
	int	(*vop_print)		(struct vnode *vp);
	int	(*vop_islocked)		(struct vnode *vp);
	int (*vop_pathconf)		(struct vnode *vp, int name, register_t *retval);
	int	(*vop_advlock)		(struct vnode *vp, caddr_t id, int op, struct flock *fl, int flags);
	int (*vop_blkatoff)		(struct vnode *vp, off_t offset, char **res, struct buf **bpp);
	int (*vop_valloc)		(struct vnode *pvp, int mode, struct ucred *cred, struct vnode **vpp);
	int (*vop_reallocblks)	(struct vnode *vp, struct cluster_save *buflist);
	int (*vop_vfree)		(struct vnode *pvp, ino_t ino, int mode);
	int (*vop_truncate)		(struct vnode *vp, off_t length, int flags, struct ucred *cred, struct proc *p);
	int (*vop_update)		(struct vnode *vp, struct timeval *access, struct timeval *modify, int waitfo);
	int	(*vop_strategy)		(struct buf *bp);
	int (*vop_bwrite)		(struct buf *bp);
};

extern struct vnodeops *vops;

/* Macros to call the vnode ops */
#define	VOP_LOOKUP(dvp, vpp, cnp)	    						(*((dvp)->v_op->vop_lookup))(dvp, vpp, cnp)
#define	VOP_CREATE(dvp, vpp, cnp, vap)	    					(*((dvp)->v_op->vop_create))(dvp, vpp, cnp, vap)
#define VOP_WHITEOUT(dvp, cnp, flags)							(*((dvp)->v_op->vop_whiteout))(dvp, cnp, flags)
#define	VOP_MKNOD(dvp, vpp, cnp, vap)	    					(*((dvp)->v_op->vop_mknod))(dvp, vpp, cnp, vap)
#define	VOP_OPEN(vp, mode, cred, p)	    						(*((vp)->v_op->vop_open))(vp, mode, cred, p)
#define	VOP_CLOSE(vp, fflag, cred, p)	    					(*((vp)->v_op->vop_close))(vp, fflag, cred, p)
#define	VOP_ACCESS(vp, mode, cred, p)	    					(*((vp)->v_op->vop_access))(vp, mode, cred, p)
#define	VOP_GETATTR(vp, vap, cred, p)							(*((vp)->v_op->vop_getattr))(vp, vap, cred, p)
#define	VOP_SETATTR(vp, vap, cred, p)							(*((vp)->v_op->vop_setattr))(vp, vap, cred, p)
#define	VOP_READ(vp, uio, ioflag, cred)	    					(*((vp)->v_op->vop_read))(vp, uio, ioflag, cred)
#define	VOP_WRITE(vp, uio, ioflag, cred)						(*((vp)->v_op->vop_write))(vp, uio, ioflag, cred)
#define VOP_LEASE(vp, p, cred, flag)							(*((vp)->v_op->vop_lease))(vp, p, cred, flag)
#define	VOP_IOCTL(vp, command, data, fflag, cred, p)			(*((vp)->v_op->vop_ioctl))(vp, command, data, fflag, cred, p)
#define	VOP_SELECT(vp, which, fflags, cred, p)					(*((vp)->v_op->vop_select))(vp, which, fflags, cred, p)
#define VOP_REVOKE(vp, flags)									(*((vp)->v_op->vop_revoke))(vp, flags)
#define	VOP_MMAP(vp, fflags, cred, p)		    				(*((vp)->v_op->vop_mmap))(vp, fflags, cred, p)
#define	VOP_FSYNC(vp, cred, waitfor, p)							(*((vp)->v_op->vop_fsync))(vp, cred, waitfor, p)
#define	VOP_SEEK(vp, oldoff, newoff, cred)	    				(*((vp)->v_op->vop_seek))(vp, oldoff, newoff, cred)
#define	VOP_REMOVE(dvp, vp, cnp)		    					(*((dvp)->v_op->vop_remove))(dvp, vp, cnp)
#define	VOP_LINK(vp, tdvp, cnp)		    						(*((vp)->v_op->vop_link))(vp, tdvp, cnp)
#define	VOP_RENAME(fdvp, fvp, fcnp, tdvp, tvp, tcnp)			(*((fdvp)->v_op->vop_rename))(fdvp, fvp, fcnp, tdvp, tvp, tcnp)
#define	VOP_MKDIR(dvp, vpp, cnp, vap)	   						(*((dvp)->v_op->vop_mkdir))(dvp, vpp, cnp, vap)
#define	VOP_RMDIR(dvp, vp, cnp)		    						(*((dvp)->v_op->vop_rmdir))(dvp, vp, cnp)
#define	VOP_SYMLINK(dvp, vpp, cnp, vap, target)					(*((dvp)->v_op->vop_symlink))(dvp, vpp, cnp, vap, target)
#define	VOP_READDIR(vp, uio, cred, eofflag, ncookies, cookies)	(*((vp)->v_op->vop_readdir))(vp, uio, cred, eofflag, ncookies, cookies)
#define	VOP_READLINK(vp, uio, cred)	    						(*((vp)->v_op->vop_readlink))(vp, uio, cred)
#define	VOP_ABORTOP(dvp, cnp)		    						(*((dvp)->v_op->vop_abortop))(dvp, cnp)
#define	VOP_INACTIVE(vp, p)	    								(*((vp)->v_op->vop_inactive))(vp, p)
#define	VOP_RECLAIM(vp, p)		    							(*((vp)->v_op->vop_reclaim))(vp, p)
#define	VOP_LOCK(vp, flags, p)		        					(*((vp)->v_op->vop_lock))(vp, flags, p)
#define	VOP_UNLOCK(vp, flags, p)		    					(*((vp)->v_op->vop_unlock))(vp, flags, p)
#define	VOP_BMAP(vp, bn, vpp, bnp, runp)	    				(*((vp)->v_op->vop_bmap))(vp, bn, vpp, bnp, runp)
#define	VOP_PRINT(vp)		    								(*((vp)->v_op->vop_print))(vp)
#define	VOP_ISLOCKED(vp)		    							(*((v)->v_op->vop_islocked))(vp))
#define VOP_PATHCONF(vp, name, retval)							(*((vp)->v_op->vop_pathconf))(vp, name, retval)
#define	VOP_ADVLOCK(vp, id, op, fl, flags)						(*((vp)->v_op->vop_advlock))(vp, id, op, fl, flags)
#define VOP_BLKATOFF(vp, offset, res, bpp)						(*(vp)->v_op->vop_blkatoff))(vp, offset, res, bpp)
#define VOP_VALLOC(pvp, mode, cred, vpp)						(*(pvp)->v_op->vop_valloc))(pvp, mode, cred, vpp)
#define VOP_REALLOCBLKS(vp, buflist)							(*(vp)->v_op->vop_reallocblks))(vp, buflist)
#define VOP_VFREE(pvp, ino, mode)								(*(pvp)->v_op->vop_vfree))(pvp, ino, mode)
#define VOP_TRUNCATE(vp, length, flags, cred, p)				(*(vp)->v_op->vop_truncate))(vp, length, flags, cred, p)
#define VOP_UPDATE(vp, access, modify, waitfor)					(*(vp)->v_op->vop_update))(vp, access, modify, waitfor)
#define	VOP_STRATEGY(bp)										(*((bp)->b_vp->v_op->vop_strategy))(bp)
#define VOP_BWRITE(bp)											(*((bp)->b_vp->v_op->vop_strategy))(bp)
