/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)vfs_vnops.c	8.14 (Berkeley) 6/15/95
 */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>

#include <vm/include/vm.h>

struct fileops vnops = {
		.fo_rw = vn_rw,
		.fo_read = vn_read,
		.fo_write = vn_write,
		.fo_ioctl = vn_ioctl,
		.fo_poll = vn_poll,
		.fo_stat = vn_fstat,
		.fo_close = vn_closefile,
		.fo_kqfilter = vn_kqfilter
};

/*
 * Common code for vnode open operations.
 * Check permissions, and call the VOP_OPEN or VOP_CREATE routine.
 */
int
vn_open(ndp, fmode, cmode)
	register struct nameidata *ndp;
	int fmode, cmode;
{
	register struct vnode *vp;
	register struct proc *p = ndp->ni_cnd.cn_proc;
	register struct ucred *cred = p->p_ucred;
	struct vattr vat;
	struct vattr *vap = &vat;
	int error;

	if (fmode & O_CREAT) {
		ndp->ni_cnd.cn_nameiop = CREATE;
		ndp->ni_cnd.cn_flags = LOCKPARENT | LOCKLEAF;
		if ((fmode & O_EXCL) == 0)
			ndp->ni_cnd.cn_flags |= FOLLOW;
		if ((error = namei(ndp)))
			return (error);
		if (ndp->ni_vp == NULL) {
			VATTR_NULL(vap);
			vap->va_type = VREG;
			vap->va_mode = cmode;
			if (fmode & O_EXCL)
				vap->va_vaflags |= VA_EXCLUSIVE;
			VOP_LEASE(ndp->ni_dvp, p, cred, LEASE_WRITE);
			if ((error = VOP_CREATE(ndp->ni_dvp, &ndp->ni_vp, &ndp->ni_cnd, vap)))
				return (error);
			fmode &= ~O_TRUNC;
			vp = ndp->ni_vp;
		} else {
			VOP_ABORTOP(ndp->ni_dvp, &ndp->ni_cnd);
			if (ndp->ni_dvp == ndp->ni_vp)
				vrele(ndp->ni_dvp);
			else
				vput(ndp->ni_dvp);
			ndp->ni_dvp = NULL;
			vp = ndp->ni_vp;
			if (fmode & O_EXCL) {
				error = EEXIST;
				goto bad;
			}
			fmode &= ~O_CREAT;
		}
	} else {
		ndp->ni_cnd.cn_nameiop = LOOKUP;
		ndp->ni_cnd.cn_flags = FOLLOW | LOCKLEAF;
		if ((error = namei(ndp)))
			return (error);
		vp = ndp->ni_vp;
	}
	if (vp->v_type == VSOCK) {
		error = EOPNOTSUPP;
		goto bad;
	}
	if ((fmode & O_CREAT) == 0) {
		if (fmode & FREAD) {
			if ((error = VOP_ACCESS(vp, VREAD, cred, p)))
				goto bad;
		}
		if (fmode & (FWRITE | O_TRUNC)) {
			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto bad;
			}
			if ((error = vn_writechk(vp))
					|| (error = VOP_ACCESS(vp, VWRITE, cred, p)))
				goto bad;
		}
	}
	if (fmode & O_TRUNC) {
		VOP_UNLOCK(vp, 0, p); /* XXX */
		VOP_LEASE(vp, p, cred, LEASE_WRITE);
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p); /* XXX */
		VATTR_NULL(vap);
		vap->va_size = 0;
		if ((error = VOP_SETATTR(vp, vap, cred, p)))
			goto bad;
	}
	if ((error = VOP_OPEN(vp, fmode, cred, p)))
		goto bad;
	if (fmode & FWRITE)
		vp->v_writecount++;
	return (0);
bad:
	vput(vp);
	return (error);
}

/*
 * Check for write permissions on the specified vnode.
 * Prototype text segments cannot be written.
 */
int
vn_writechk(vp)
	register struct vnode *vp;
{

	/*
	 * If there's shared text associated with
	 * the vnode, try to free it up once.  If
	 * we fail, we can't allow writing.
	 */
	if ((vp->v_flag & VTEXT) && !vnode_pager_uncache(vp))
		return (ETXTBSY);
	return (0);
}

/*
 * Vnode close call
 */
int
vn_close(vp, flags, cred, p)
	register struct vnode *vp;
	int flags;
	struct ucred *cred;
	struct proc *p;
{
	int error;

	if (flags & FWRITE)
		vp->v_writecount--;
	error = VOP_CLOSE(vp, flags, cred, p);
	vrele(vp);
	return (error);
}

/*
 * Package up an I/O request on a vnode into a uio and do it.
 */
int
vn_rdwr(rw, vp, base, len, offset, segflg, ioflg, cred, aresid, p)
	enum uio_rw rw;
	struct vnode *vp;
	caddr_t base;
	int len;
	off_t offset;
	enum uio_seg segflg;
	int ioflg;
	struct ucred *cred;
	int *aresid;
	struct proc *p;
{
	struct uio auio;
	struct iovec aiov;
	int error;

	if ((ioflg & IO_NODELOCKED) == 0)
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_resid = len;
	auio.uio_offset = offset;
	auio.uio_segflg = segflg;
	auio.uio_rw = rw;
	auio.uio_procp = p;
	if (rw == UIO_READ) {
		error = VOP_READ(vp, &auio, ioflg, cred);
	} else {
		error = VOP_WRITE(vp, &auio, ioflg, cred);
	}
	if (aresid)
		*aresid = auio.uio_resid;
	else
		if (auio.uio_resid && error == 0)
			error = EIO;
	if ((ioflg & IO_NODELOCKED) == 0)
		VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * File table vnode read/write routine.
 */
int
vn_rw(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	enum uio_rw rw = uio->uio_rw;
	int error;

	if (rw == UIO_READ) {
		error = vn_read(fp, uio, cred);
	} else {
		error = vn_write(fp, uio, cred);
	}
	return (error);
}

/*
 * File table vnode read routine.
 */
int
vn_read(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	struct vnode *vp = (struct vnode *)fp->f_data;
	struct proc *p = uio->uio_procp;
	int count, error;

	VOP_LEASE(vp, p, cred, LEASE_READ);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	uio->uio_offset = fp->f_offset;
	count = uio->uio_resid;
	error = VOP_READ(vp, uio, (fp->f_flag & FNONBLOCK) ? IO_NDELAY : 0,
		cred);
	fp->f_offset += count - uio->uio_resid;
	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * File table vnode write routine.
 */
int
vn_write(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	struct vnode *vp = (struct vnode *)fp->f_data;
	struct proc *p = uio->uio_procp;
	int count, error, ioflag = IO_UNIT;

	if (vp->v_type == VREG && (fp->f_flag & O_APPEND))
		ioflag |= IO_APPEND;
	if (fp->f_flag & FNONBLOCK)
		ioflag |= IO_NDELAY;
	if ((fp->f_flag & O_FSYNC) ||
	    (vp->v_mount && (vp->v_mount->mnt_flag & MNT_SYNCHRONOUS)))
		ioflag |= IO_SYNC;
	VOP_LEASE(vp, p, cred, LEASE_WRITE);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	uio->uio_offset = fp->f_offset;
	count = uio->uio_resid;
	error = VOP_WRITE(vp, uio, ioflag, cred);
	if (ioflag & IO_APPEND)
		fp->f_offset = uio->uio_offset;
	else
		fp->f_offset += count - uio->uio_resid;
	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * File table vnode stat routine.
 */
int
vn_stat(vp, sb, p)
	struct vnode *vp;
	register struct stat *sb;
	struct proc *p;
{
	struct vattr vattr;
	register struct vattr *vap;
	int error;
	u_short mode;
        
	vap = &vattr;
	error = VOP_GETATTR(vp, vap, p->p_ucred, p);
	if (error)
		return (error);
	/*
	 * Copy from vattr table
	 */
	sb->st_dev = vap->va_fsid;
	sb->st_ino = vap->va_fileid;
	mode = vap->va_mode;
	switch (vp->v_type) {
	case VREG:
		mode |= S_IFREG;
		break;
	case VDIR:
		mode |= S_IFDIR;
		break;
	case VBLK:
		mode |= S_IFBLK;
		break;
	case VCHR:
		mode |= S_IFCHR;
		break;
	case VLNK:
		mode |= S_IFLNK;
		break;
	case VSOCK:
		mode |= S_IFSOCK;
		break;
	case VFIFO:
		mode |= S_IFIFO;
		break;
	default:
		return (EBADF);
	};
	sb->st_mode = mode;
	sb->st_nlink = vap->va_nlink;
	sb->st_uid = vap->va_uid;
	sb->st_gid = vap->va_gid;
	sb->st_rdev = vap->va_rdev;
	sb->st_size = vap->va_size;
	sb->st_atime = vap->va_atime;
	sb->st_mtime = vap->va_mtime;
	sb->st_ctime = vap->va_ctime;
	sb->st_blksize = vap->va_blocksize;
	sb->st_flags = vap->va_flags;
	sb->st_gen = vap->va_gen;
	sb->st_blocks = vap->va_bytes / S_BLKSIZE;

	return (0);
}

/*
 * File table vnode stat compat routine.
 */
int
vn_fstat(fp, sb, p)
	struct file *fp;
	register struct stat *sb;
	struct proc *p;
{
	struct vnode *vp;
	vp = (struct vnode *)fp->f_data;

	return (vn_stat(vp, sb, p));
}

/*
 * File table vnode ioctl routine.
 */
int
vn_ioctl(fp, com, data, p)
	struct file *fp;
	u_long com;
	void *data;
	struct proc *p;
{
	register struct vnode *vp = ((struct vnode *)fp->f_data);
	struct vattr vattr;
	int error;

	switch (vp->v_type) {

	case VREG:
	case VDIR:
		if (com == FIONREAD) {
			if ((error = VOP_GETATTR(vp, &vattr, p->p_ucred, p)))
				break;
			*(int *)data = vattr.va_size - fp->f_offset;
		} else if (com == FIONBIO || com == FIOASYNC)	/* XXX */
			error = 0;									/* XXX */
		break;

	case VFIFO:
	case VCHR:
	case VBLK:
		error = VOP_IOCTL(vp, com, data, fp->f_flag, p->p_ucred, p);
		if (error == 0 && com == TIOCSCTTY) {
			if (p->p_session->s_ttyvp)
				vrele(p->p_session->s_ttyvp);
			p->p_session->s_ttyvp = vp;
			VREF(vp);
		}
		break;

	default:
		break;
	}
	return (error);
}

/*
 * File table vnode poll routine.
 */
int
vn_poll(fp, events, p)
	struct file *fp;
	int events;
	struct proc *p;
{
	return (VOP_POLL(((struct vnode *)fp->f_data), fp->f_flag, events, p));
}

/*
 * File table vnode kqfilter routine.
 */
int
vn_kqfilter(fp, kn)
	struct file *fp;
	struct knote *kn;
{
	return (VOP_KQFILTER((struct vnode *)fp->f_data, kn));
}

/*
 * Check that the vnode is still valid, and if so
 * acquire requested lock.
 */
int
vn_lock(vp, flags, p)
	struct vnode *vp;
	int flags;
	struct proc *p;
{
	int error;

	do {
		if ((flags & LK_INTERLOCK) == 0)
			simple_lock(&vp->v_interlock);
		if (vp->v_flag & VXLOCK) {
			vp->v_flag |= VXWANT;
			simple_unlock(&vp->v_interlock);
			tsleep((caddr_t)vp, PINOD, "vn_lock", 0);
			error = ENOENT;
		} else {
			error = VOP_LOCK(vp, flags | LK_INTERLOCK, p);
			if (error == 0)
				return (error);
		}
		flags &= ~LK_INTERLOCK;
	} while (flags & LK_RETRY);
	return (error);
}

/*
 * File table vnode close routine.
 */
int
vn_closefile(fp, p)
	struct file *fp;
	struct proc *p;
{
	return (vn_close(((struct vnode *)fp->f_data), fp->f_flag, fp->f_cred, p));
}

/*
 * Mark a vnode as being the text of a process.
 * Fail if the vnode is currently writable.
 */
int
vn_marktext(vp)
	struct vnode *vp;
{
	if (vp->v_writecount != 0) {
		KASSERT((vp->v_flag & VTEXT) == 0);
		return (ETXTBSY);
	}
	vp->v_flag |= VTEXT;
	return (0);
}
