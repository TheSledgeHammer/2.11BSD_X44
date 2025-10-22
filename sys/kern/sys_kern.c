/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	sys_kern.c 1.2 (2.11BSD) 1997/1/30
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/unpcb.h>
#include <sys/mbuf.h>
#include <sys/map.h>
#include <sys/stat.h>
#include <sys/domain.h>

#include <net/if.h>

#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#endif

void
netstart(void)
{
	int s;

#ifdef	FAST_IPSEC
	/* Attach network crypto subsystem */
	ipsec_attach();
#endif

	s = splimp();
	ifinit();
	domaininit();
	if_attachdomain();
	splx(s);
}

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
	return (p);
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
	return (fp->f_count);
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
	register struct unpcb *unp;
	register struct vnode *vp;
	struct vattr vattr;
	char pth[MLEN];
	int error;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	unp = sotounpcb(unpsock);
	bcopy(path, pth, len);
	NDINIT(ndp, CREATE, FOLLOW | LOCKPARENT, UIO_SYSSPACE, pth, u.u_procp);
	ndp->ni_dirp[len - 2] = 0;
	*vpp = 0;
	if ((error = namei(ndp))) {
	    return (error);
	}
	vp = ndp->ni_vp;
	if (vp != NULL) {
		VOP_ABORTOP(ndp->ni_dvp, &ndp->ni_cnd);
		if (ndp->ni_dvp == vp)
			vrele(ndp->ni_dvp);
		else
			vput(ndp->ni_dvp);
		vrele(vp);
		return (EADDRINUSE);
	}
	VATTR_NULL(&vattr);
	vattr.va_type = VSOCK;
	vattr.va_mode = ACCESSPERMS;
	VOP_LEASE(nd.ni_dvp, u.u_procp, u.u_ucred, LEASE_WRITE);
	if ((error = VOP_CREATE(ndp->ni_dvp, &ndp->ni_vp, &ndp->ni_cnd, &vattr))) {
    	u.u_error = error;
		return (error);
	}
	vp = ndp->ni_vp;
	vp->v_socket = unp->unp_socket;
	unp->unp_vnode = vp;
	*vpp = vp;
	vrele(vp);
	VOP_UNLOCK(vp, 0, u.u_procp);
	return (0);
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

	VATTR_NULL(vap);
	bcopy(path, pth, len);
	if (!len) {
		return (EINVAL); /* paranoia */
	}
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_SYSSPACE, pth, u.u_procp);
	ndp->ni_dirp[len - 2] = 0;
	if ((error = namei(ndp))) {
	    return (error);
	}
	vp = ndp->ni_vp;
	*vpp = vp;
	if (vp->v_type != VSOCK) {
		error = ENOTSOCK;
		u.u_error = error;
		vput(vp);
		return (error);
	}
	if ((error = VOP_ACCESS(vp, VWRITE, u.u_ucred, u.u_procp))) {
		u.u_error = error;
		vput(vp);
		return (error);
	}
	*so2 = vp->v_socket;
	if (*so2 == 0) {
		return (ECONNREFUSED);
	}
	return (0);
}

int
unpdisc(fp)
	struct file *fp;
{
	--fp->f_msgcount;
	return (closef(fp));
}
