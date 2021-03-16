/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	sys_kern.c 1.2 (2.11BSD) 1997/1/30
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/mbuf.h>
#include <sys/map.h>
#include <sys/stat.h>

int knetisr;

void
netcrash()
{
	panic("Network crashed");
}

/*
 * These next entry points exist to allow the network kernel to
 * access components of structures that exist in kernel data space.
 */
void
netpsignal(p, sig)		/* XXX? sosend, sohasoutofband, sowakeup */
	struct proc *p;		/* if necessary, wrap psignal in save/restor */
	int sig;
{
	psignal(p, sig);
}

struct proc *
netpfind(pid)
	int pid;
{
	register struct proc *p;

	p = pfind(pid);
	return(p);
}

void
fpflags(fp, set, clear)
	struct file *fp;
	int set, clear;
{
	fp->f_flag |= set;
	fp->f_flag &= ~clear;
}

void
fadjust(fp, msg, cnt)
	struct file *fp;
	int msg, cnt;
{
	fp->f_msgcount += msg;
	fp->f_count += cnt;
}

int
fpfetch(fp, fpp)
	struct file *fp, *fpp;
{
	*fpp = *fp;
	return(fp->f_count);
}

void
unpdet(vp)
	struct vnode *vp;
{
	vp->v_socket = 0;
	vrele(vp);
}

int
unpbind(path, len, vpp, unpsock)
	char *path;
	int len;
	struct vnode **vpp;
	struct socket *unpsock;
{
	/*
	 * As far as I could find out, the 'path' is in the _u area because
	 * a fake mbuf was MBZAP'd in bind().  The inode pointer is in the
	 * kernel stack so we can modify it.  SMS
	 */
	register struct vnode *vp;
	char pth[MLEN];
	int error;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	bcopy(path, pth, len);
	NDINIT(ndp, CREATE, FOLLOW, UIO_SYSSPACE, pth);
	ndp->ni_dirp[len - 2] = 0;
	*vpp = 0;
	vp = namei(ndp);
	if (vp) {
		vput(vp);
		return(EADDRINUSE);
	}
	if (u->u_error || !(vp = MAKEIMODE(S_IFSOCK | 0777, ndp))) {
		error = u->u_error;
		u->u_error = 0;
		return(error);
	}
	*vpp = vp;
	vp->v_socket = unpsock;
	vrele(vp);
	return(0);
}

int
unpconn(path, len, so2, vpp)
	char *path;
	int len;
	struct socket **so2;
	struct vnode **vpp;
{
	register struct vnode *vp;
	register struct vattr *vap;
	char pth[MLEN];
	int error;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	vap = vattr_null(vap);

	bcopy(path, pth, len);
	if (!len)
		return(EINVAL);		/* paranoia */
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_SYSSPACE, pth);
	ndp->ni_dirp[len - 2] = 0;
	vp = namei(ndp);
	*vpp = vp;
	if (!vp || access(vp, S_IWRITE)) {
		error = u->u_error;
		u->u_error = 0;
		return(error);
	}

	if ((vp->v_socket & S_IFMT) != S_IFSOCK)
		return(ENOTSOCK);
	*so2 = vp->v_socket;
	if (*so2 == 0)
		return(ECONNREFUSED);
	return(0);
}

void
unpgc1(beginf, endf)
	struct file **beginf, **endf;
{
	register struct file *fp;

	for (*beginf = fp = file; fp < fileNFILE; fp++)
		fp->f_flag &= ~(FMARK|FDEFER);
	*endf = fileNFILE;
}

int
unpdisc(fp)
	struct file *fp;
{
	--fp->f_msgcount;
	return(closef(fp));
}
