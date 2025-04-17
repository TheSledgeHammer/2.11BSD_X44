/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 1999-2001 Robert N. M. Watson
 * All rights reserved.
 *
 * This software was developed by Robert Watson for the TrustedBSD Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * VFS extended attribute support.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/extattr.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/sysdecl.h>

static int extattr_get_vp(struct proc *, int, struct vnode *, int, const char *, void *, size_t);
static int extattr_set_vp(struct proc *, int, struct vnode *, int, const char *, void *, size_t);
static int extattr_delete_vp(struct proc *, int, struct vnode *, int, const char *);

/* file */
static int kern_extattr_get_file(struct proc *, int, char *, int, void *, size_t);
static int kern_extattr_set_file(struct proc *, int, char *, int, void *, size_t);
static int kern_extattr_delete_file(struct proc *, int, char *, int);

/* file descriptor */
static int kern_extattr_get_filedesc(struct proc *, int, int, int, void *, size_t);
static int kern_extattr_set_filedesc(struct proc *, int, int, int, void *, size_t);
static int kern_extattr_delete_filedesc(struct proc *, int, int, int);

/*
 * extattr_get_vp:
 *
 *	Get a named extended attribute on a file or directory.
 */
static int
extattr_get_vp(p, cmd, vp, attrnamespace, attrname, data, nbytes)
	struct proc *p;
	int cmd;
	struct vnode *vp;
	int attrnamespace;
	const char *attrname;
	void *data;
	size_t nbytes;
{
	struct uio auio, *auiop;
	struct iovec aiov;
	ssize_t cnt;
	size_t size, *sizep;
	int error;

	if (nbytes > INT_MAX) {
		return (EINVAL);
	}

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);

	/*
	 * Slightly unusual semantics: if the user provides a NULL data
	 * pointer, they don't want to receive the data, just the maximum
	 * read length.
	 */
	auiop = NULL;
	sizep = NULL;
	cnt = 0;
	if (data != NULL) {
		aiov.iov_base = data;
		aiov.iov_len = nbytes;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_resid = nbytes;
		auio.uio_rw = UIO_READ;
		auio.uio_segflg = UIO_USERSPACE;
		auiop = &auio;
		cnt = nbytes;
	} else {
		sizep = &size;
	}

	error = VOP_GETEXTATTR(vp, attrnamespace, attrname, auiop, sizep, u.u_ucred);

	if (auiop != NULL) {
		cnt -= auio.uio_resid;
		u.u_r.r_val1 = cnt;
	} else {
		u.u_r.r_val1 = size;
	}

done:
	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * extattr_set_vp:
 *
 *	Set a named extended attribute on a file or directory.
 */
static int
extattr_set_vp(p, cmd, vp, attrnamespace, attrname, data, nbytes)
	struct proc *p;
	int cmd;
	struct vnode *vp;
	int attrnamespace;
	const char *attrname;
	void *data;
	size_t nbytes;
{
	struct uio auio;
	struct iovec aiov;
	ssize_t cnt;
	int error;

	if (nbytes > INT_MAX) {
		return (EINVAL);
	}

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);

	aiov.iov_base = data;
	aiov.iov_len = nbytes;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_resid = nbytes;
	auio.uio_rw = UIO_WRITE;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_procp = p;
	cnt = nbytes;

	error = VOP_SETEXTATTR(vp, attrnamespace, attrname, &auio, u.u_ucred);
	cnt -= auio.uio_resid;
	u.u_r.r_val1 = cnt;

	VOP_UNLOCK(vp, 0, p);
	return (error);
}

/*
 * extattr_delete_vp:
 *
 *	Delete a named extended attribute on a file or directory.
 */
static int
extattr_delete_vp(p, cmd, vp, attrnamespace, attrname)
	struct proc *p;
	int cmd;
	struct vnode *vp;
	int attrnamespace;
	const char *attrname;
{
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);

	error = VOP_DELETEEXTATTR(vp, attrnamespace, attrname, p->p_ucred);
	if (error == EOPNOTSUPP) {
		error = VOP_SETEXTATTR(vp, attrnamespace, attrname, NULL, u.u_ucred);
	}

	VOP_UNLOCK(vp, 0, p);
	return (error);
}

int
extattrctl()
{
	int error;

	return (error);
}

static int
kern_extattr_get_file(p, cmd, path, attrnamespace, data, nbytes)
	struct proc *p;
	int cmd;
	char *path;
	int attrnamespace;
	void *data;
	size_t nbytes;
{
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN + 1];
	int error;

	if (cmd != GETEXTATTR) {
		return (EINVAL);
	}

	error = copyinstr(attrname, attrname, sizeof(attrname), NULL);
	if (error) {
		return (error);
	}

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, path, p);
	error = namei(&nd);
	if (error) {
		return (error);
	}

	error = extattr_get_vp(p, cmd, nd.ni_vp, attrnamespace, attrname, data, nbytes);
	vrele(nd.ni_vp);
	return (error);
}

static int
kern_extattr_set_file(p, cmd, path, attrnamespace, data, nbytes)
	struct proc *p;
	int cmd;
	char *path;
	int attrnamespace;
	void *data;
	size_t nbytes;
{
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN + 1];
	int error;

	if (cmd != SETEXTATTR) {
		return (EINVAL);
	}

	error = copyinstr(attrname, attrname, sizeof(attrname), NULL);
	if (error) {
		return (error);
	}

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, path, p);
	error = namei(&nd);
	if (error) {
		return (error);
	}

	error = extattr_set_vp(p, cmd, nd.ni_vp, attrnamespace, attrname, data, nbytes);
	vrele(nd.ni_vp);
	return (error);
}

static int
kern_extattr_delete_file(p, cmd, path, attrnamespace)
	struct proc *p;
	int cmd;
	char *path;
	int attrnamespace;
{
	struct nameidata nd;
	char attrname[EXTATTR_MAXNAMELEN + 1];
	int error;

	if (cmd != DELEXTATTR) {
		return (EINVAL);
	}

	error = copyinstr(attrname, attrname, sizeof(attrname), NULL);
	if (error) {
		return (error);
	}

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, path, p);
	error = namei(&nd);
	if (error) {
		return (error);
	}

	error = extattr_delete_vp(p, cmd, nd.ni_vp, attrnamespace, attrname);
	vrele(nd.ni_vp);
	return (error);
}

static int
kern_extattr_get_filedesc(p, cmd, filedes, attrnamespace, data, nbytes)
	struct proc *p;
	int cmd, filedes, attrnamespace;
	void *data;
	size_t nbytes;
{
	struct file *fp;
	char attrname[EXTATTR_MAXNAMELEN + 1];
	int error;

	if (cmd != GETEXTATTR) {
		return (EINVAL);
	}

	error = copyinstr(attrname, attrname, sizeof(attrname), NULL);
	if (error) {
		return (error);
	}

	error = checkfiledesc(&fp, filedes);
	if (error) {
		return (error);
	}

	error = extattr_get_vp(p, cmd, (struct vnode *)fp->f_data, attrnamespace, attrname, data, nbytes);
	return (error);
}

static int
kern_extattr_set_filedesc(p, cmd, filedes, attrnamespace, data, nbytes)
	struct proc *p;
	int cmd, filedes, attrnamespace;
	void *data;
	size_t nbytes;
{
	struct file *fp;
	char attrname[EXTATTR_MAXNAMELEN + 1];
	int error;

	if (cmd != SETEXTATTR) {
		return (EINVAL);
	}

	error = copyinstr(attrname, attrname, sizeof(attrname), NULL);
	if (error) {
		return (error);
	}

	error = checkfiledesc(&fp, filedes);
	if (error) {
		return (error);
	}

	error = extattr_set_vp(p, cmd, (struct vnode *)fp->f_data, attrnamespace, attrname, data, nbytes);
	return (error);
}

static int
kern_extattr_delete_filedesc(p, cmd, filedes, attrnamespace)
	struct proc *p;
	int cmd, filedes, attrnamespace;
{
	struct file *fp;
	char attrname[EXTATTR_MAXNAMELEN + 1];
	int error;

	if (cmd != DELEXTATTR) {
		return (EINVAL);
	}

	error = copyinstr(attrname, attrname, sizeof(attrname), NULL);
	if (error) {
		return (error);
	}

	error = checkfiledesc(&fp, filedes);
	if (error) {
		return (error);
	}

	error = extattr_delete_vp(p, cmd, (struct vnode *)fp->f_data, attrnamespace, attrname);
	return (error);
}

int
extattr_file()
{
	register struct extattr_file_args {
		syscallarg(int) cmd;
		syscallarg(char *) path;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
		syscallarg(const void *) data;
		syscallarg(size_t) nbytes;
	} *uap = (struct extattr_file_args *)u.u_ap;
	register struct proc *p;
	int error;

	p = u.u_procp;
	switch (SCARG(uap, cmd)) {
	case GETEXTATTR:
		error = kern_extattr_get_file(p, SCARG(uap, cmd), SCARG(uap, path), SCARG(uap, attrnamespace), SCARG(uap, attrname), SCARG(uap, data), SCARG(uap, nbytes));
		break;
	case SETEXTATTR:
		error = kern_extattr_set_file(p, SCARG(uap, cmd), SCARG(uap, path), SCARG(uap, attrnamespace), SCARG(uap, attrname), SCARG(uap, data), SCARG(uap, nbytes));
		break;
	case DELEXTATTR:
		error = kern_extattr_delete_file(p, SCARG(uap, cmd), SCARG(uap, path), SCARG(uap, attrnamespace), SCARG(uap, attrname));
		break;
	default:
		error = EINVAL;
		break;
	}
	u.u_error = error;
	return (error);
}

int
extattr_filedesc()
{
	struct extattr_filedesc_args {
		syscallarg(int) cmd;
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const char *) attrname;
		syscallarg(const void *) data;
		syscallarg(size_t) nbytes;
	} *uap = (struct extattr_filedesc_args *)u.u_ap;
	register struct proc *p;
	int error;

	p = u.u_procp;
	switch (SCARG(uap, cmd)) {
	case GETEXTATTR:
		error = kern_extattr_get_filedesc(p, SCARG(uap, cmd), SCARG(uap, fd), SCARG(uap, attrnamespace), SCARG(uap, attrname), SCARG(uap, data), SCARG(uap, nbytes));
		break;
	case SETEXTATTR:
		error = kern_extattr_set_filedesc(p, SCARG(uap, cmd), SCARG(uap, fd), SCARG(uap, attrnamespace), SCARG(uap, attrname), SCARG(uap, data), SCARG(uap, nbytes));
		break;
	case DELEXTATTR:
		error = kern_extattr_delete_filedesc(p, SCARG(uap, cmd), SCARG(uap, fd), SCARG(uap, attrnamespace), SCARG(uap, attrname));
		break;
	default:
		error = EINVAL;
		break;
	}
	u.u_error = error;
	return (error);
}
