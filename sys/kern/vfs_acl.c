/*-
 * Copyright (c) 1999, 2000 Robert N. M. Watson
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
 *
 * $FreeBSD: src/sys/kern/kern_acl.c,v 1.2.2.1 2000/07/28 18:48:16 rwatson Exp $
 * $DragonFly: src/sys/kern/kern_acl.c,v 1.17 2007/02/19 00:51:54 swildner Exp $
 */
/*
 * Developed by the TrustedBSD Project.
 *
 * ACL system calls and other functions common across different ACL types.
 * Type-specific routines go into subr_acl_<type>.c.
 */

/*
 * Generic routines to support file system ACLs, at a syntactic level
 * Semantics are the responsibility of the underlying file system
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
#include <sys/acl.h>
#include <sys/user.h>
#include <sys/sysdecl.h>

static int vacl_set_acl(struct proc *p, struct vnode *vp, acl_type_t type, struct acl *aclp);
static int vacl_get_acl(struct proc *p, struct vnode *vp, acl_type_t type, struct acl *aclp);
static int vacl_delete(struct proc *p, struct vnode *vp, acl_type_t type);
static int vacl_aclcheck(struct proc *p, struct vnode *vp, acl_type_t type, struct acl *aclp);

/* file */
static int kern_acl_get_file(int, const char *, acl_type_t, struct acl *);
static int kern_acl_set_file(int, const char *, acl_type_t, struct acl *);
static int kern_acl_delete_file(int, const char *, acl_type_t);
static int kern_acl_aclcheck_file(int, const char *, acl_type_t, struct acl *);
/* file descriptor */
static int kern_acl_get_filedesc(int, int, acl_type_t, struct acl *);
static int kern_acl_set_filedesc(int, int, acl_type_t, struct acl *);
static int kern_acl_delete_filedesc(int, int, acl_type_t);
static int kern_acl_aclcheck_filedesc(int, int, acl_type_t, struct acl *);

/*
 * These calls wrap the real vnode operations, and are called by the syscall
 * code once the syscall has converted the path or file descriptor to a vnode
 * (unlocked).  The aclp pointer is assumed still to point to userland, so
 * this should not be consumed within the kernel except by syscall code.
 * Other code should directly invoke VOP_{SET,GET}ACL.
 */

/*
 * Given a vnode, set its ACL.
 */
static int
vacl_set_acl(p, vp, type, aclp)
	struct proc *p;
	struct vnode *vp;
	acl_type_t type;
	const struct acl *aclp;
{
	struct acl inkernacl;
	int error;

	inkernacl = acl_alloc(M_WAITOK);
	error = copyin(aclp, &inkernacl, sizeof(struct acl));
	if (error) {
		return(error);
	}

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_SETACL(vp, type, inkernacl, p->p_cred);
	VOP_UNLOCK(vp);
	return (error);
}

/*
 * Given a vnode, get its ACL.
 */
static int
vacl_get_acl(p, vp, type, aclp)
	struct proc *p;
	struct vnode *vp;
	acl_type_t type;
	struct acl *aclp;
{
	struct acl inkernelacl;
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_GETACL(vp, type, &inkernelacl, p->p_cred);
	VOP_UNLOCK(vp);
	if (error == 0) {
		error = copyout(&inkernelacl, aclp, sizeof(struct acl));
	}
	return (error);
}

/*
 * Given a vnode, delete its ACL.
 */
static int
vacl_delete(p, vp, type)
	struct proc *p;
	struct vnode *vp;
	acl_type_t type;
{
	int error;

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	error = VOP_SETACL(vp, ACL_TYPE_DEFAULT, 0, p->p_cred);
	vn_unlock(vp);
	return (error);
}

/*
 * Given a vnode, check whether an ACL is appropriate for it
 *
 * XXXRW: No vnode lock held so can't audit vnode state...?
 */
static int
vacl_aclcheck(p, vp, type, aclp)
	struct proc *p;
	struct vnode *vp;
	acl_type_t type;
	const struct acl *aclp;
{
	struct acl inkernelacl;
	int error;

	error = copyin(aclp, &inkernelacl, sizeof(struct acl));
	if (error) {
		return (error);
	}
	error = VOP_ACLCHECK(vp, type, &inkernelacl, p->p_cred);
	return (error);
}

static int
kern_acl_get_file(p, cmd, path, follow, type, aclp)
	struct proc *p;
	int cmd;
	const char *path;
	int follow;
	acl_type_t type;
	struct acl *aclp;
{
	struct nameidata nd;
	int error;

	if (cmd != GETACL) {
		return (EINVAL);
	}
	NDINIT(&nd, LOOKUP, follow, UIO_USERSPACE, path, p);
	error = namei(&nd);
	if (error == 0) {
		error = vacl_get_acl(p, nd.ni_vp, type, aclp);
		vrele(nd.ni_vp);
	}
	return (error);
}

static int
kern_acl_set_file(p, cmd, path, follow, type, aclp)
	struct proc *p;
	int cmd;
	const char *path;
	int follow;
	acl_type_t type;
	const struct acl *aclp;
{
	struct nameidata nd;
	int error;

	if (cmd != SETACL) {
		return (EINVAL);
	}
	NDINIT(&nd, LOOKUP, follow, UIO_USERSPACE, path, p);
	error = namei(&nd);
	if (error == 0) {
		error = vacl_set_acl(p, nd.ni_vp, type, aclp);
		vrele(nd.ni_vp);
	}
	return (error);
}

static int
kern_acl_delete_file(p, cmd, path, follow, type)
	struct proc *p;
	int cmd;
	const char *path;
	int follow;
	acl_type_t type;
{
	struct nameidata nd;
	int error;

	if (cmd != DELACL) {
		return (EINVAL);
	}
	NDINIT(&nd, LOOKUP, follow, UIO_USERSPACE, path, p);
	error = namei(&nd);
	if (error == 0) {
		error = vacl_delete(p, nd.ni_vp, type);
		vrele(nd.ni_vp);
	}
	return (error);
}

static int
kern_acl_aclcheck_file(p, cmd, path, follow, type, aclp)
	struct proc *p;
	int cmd;
	const char *path;
	int follow;
	acl_type_t type;
	const struct acl *aclp;
{
	struct nameidata nd;
	int error;

	if (cmd != CHECKACL) {
		return (EINVAL);
	}
	NDINIT(&nd, LOOKUP, follow, UIO_USERSPACE, path, p);
	error = namei(&nd);
	if (error == 0) {
		error = vacl_aclcheck(p, nd.ni_vp, type, aclp);
		vrele(nd.ni_vp);
	}
	return (error);
}

static int
kern_acl_get_filedesc(p, cmd, filedes, type, aclp)
	struct proc *p;
	int cmd;
	int filedes;
	acl_type_t type;
	const struct acl *aclp;
{
	struct file *fp;
	int error;

	if (cmd != GETACL) {
		return (EINVAL);
	}
	error = checkfiledesc(&fp, filedes);
	if (error == 0) {
		error =  vacl_get_acl(p, (struct vnode *)fp->f_data, type, aclp);
	}
	return (error);
}

static int
kern_acl_set_filedesc(p, cmd, filedes, type, aclp)
	struct proc *p;
	int cmd;
	int filedes;
	acl_type_t type;
	const struct acl *aclp;
{
	struct file *fp;
	int error;

	if (cmd != SETACL) {
		return (EINVAL);
	}
	error = checkfiledesc(&fp, filedes);
	if (error == 0) {
		error = vacl_set_acl(p, (struct vnode *)fp->f_data, type, aclp);
	}
	return (error);
}

static int
kern_acl_delete_filedesc(p, cmd, filedes, type)
	struct proc *p;
	int cmd;
	int filedes;
	acl_type_t type;
{
	struct file *fp;
	int error;

	if (cmd != DELACL) {
		return (EINVAL);
	}
	error = checkfiledesc(&fp, filedes);
	if (error == 0) {
		error = vacl_delete(p, (struct vnode *)fp->f_data, type);
	}
	return (error);
}

static int
kern_acl_aclcheck_filedesc(p, cmd, filedes, type, aclp)
	struct proc *p;
	int cmd;
	int filedes;
	acl_type_t type;
	const struct acl *aclp;
{
	struct file *fp;
	int error;

	if (cmd != CHECKACL) {
		return (EINVAL);
	}
	error = checkfiledesc(&fp, filedes);
	if (error == 0) {
		error = vacl_aclcheck(p, (struct vnode *)fp->f_data, type, aclp);
	}
	return (error);
}

int
acl_file()
{
	register struct acl_file_args {
		syscallarg(int) cmd;
		syscallarg(const char *) path;
		syscallarg(acl_type_t) type;
		syscallarg(struct acl *) aclp;
	} *uap = (struct acl_file_args *)u.u_ap;
	register struct proc *p;
	int error;

	p = u.u_procp;
	switch (SCARG(uap, cmd)) {
	case GETACL:
		error = kern_acl_get_file(p, SCARG(uap, cmd), SCARG(uap, path), SCARG(uap, type), SCARG(uap, aclp));
		break;
	case SETACL:
		error = kern_acl_set_file(p, SCARG(uap, cmd), SCARG(uap, path), SCARG(uap, type), SCARG(uap, aclp));
		break;
	case DELACL:
		error = kern_acl_delete_file(p, SCARG(uap, cmd), SCARG(uap, path), SCARG(uap, type));
		break;
	case CHECKACL:
		error = kern_acl_aclcheck_file(p, SCARG(uap, cmd), SCARG(uap, path), SCARG(uap, type), SCARG(uap, aclp));
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

int
acl_filedesc()
{
	register struct acl_filedesc_args {
		syscallarg(int) cmd;
		syscallarg(int) filedes;
		syscallarg(acl_type_t) type;
		syscallarg(struct acl *) aclp;
	} *uap = (struct acl_filedesc_args *)u.u_ap;
	register struct proc *p;
	int error;

	p = u.u_procp;
	switch (SCARG(uap, cmd)) {
	case GETACL:
		error = kern_acl_get_filedesc(p, SCARG(uap, cmd), SCARG(uap, filedes), SCARG(uap, type), SCARG(uap, aclp));
		break;
	case SETACL:
		error = kern_acl_set_filedesc(p, SCARG(uap, cmd), SCARG(uap, filedes), SCARG(uap, type), SCARG(uap, aclp));
		break;
	case DELACL:
		error = kern_acl_delete_filedesc(p, SCARG(uap, cmd), SCARG(uap, filedes), SCARG(uap, type));
		break;
	case CHECKACL:
		error = kern_acl_aclcheck_filedesc(p, SCARG(uap, cmd), SCARG(uap, filedes), SCARG(uap, type), SCARG(uap, aclp));
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}
