/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)sys_socket.c	7.2 (Berkeley) 3/31/88
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <net/if.h>
#include <net/route.h>

struct fileops socketops = {
		.fo_rw = soo_rw,
		.fo_read = soo_read,
		.fo_write = soo_write,
		.fo_ioctl = soo_ioctl,
		.fo_poll = soo_poll,
		.fo_close = soo_close,
		.fo_kqfilter = soo_kqfilter
};

/*ARGSUSED*/
int
soo_ioctl(fp, cmd, data, p)
	struct file *fp;
	u_long cmd;
	register caddr_t data;
	struct proc *p;
{
	register struct socket *so = (struct socket *)fp->f_socket;

	switch (cmd) {

	case FIONBIO:
		if (*(int *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		return (0);

	case FIOASYNC:
		if (*(int *)data) {
			so->so_state |= SS_ASYNC;
			so->so_rcv.sb_flags |= SB_ASYNC;
			so->so_snd.sb_flags |= SB_ASYNC;
		} else {
			so->so_state &= ~SS_ASYNC;
			so->so_rcv.sb_flags &= ~SB_ASYNC;
			so->so_snd.sb_flags &= ~SB_ASYNC;
		}
		return (0);

	case FIONREAD:
		*(long *)data = so->so_rcv.sb_cc;
		return (0);

	case SIOCSPGRP:
		so->so_pgrp = *(int *)data;
		return (0);

	case SIOCGPGRP:
		*(int *)data = so->so_pgrp;
		return (0);

	case SIOCATMARK:
		*(int*) data = (so->so_state & SS_RCVATMARK) != 0;
		return (0);
	}
	/*
	 * Interface/routing/protocol specific ioctls:
	 * interface and routing ioctls should have a
	 * different entry since a socket's unnecessary
	 */
#ifdef nonet
	if (IOCGROUP(cmd) == 'i')
		return (u.u_error = ifioctl(so, cmd, data));
	if (IOCGROUP(cmd) == 'r')
		return (u.u_error = rtioctl(cmd, data));
#endif
	return ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,(struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0, u.u_procp));
}

int
soo_poll(fp, events, p)
	struct file *fp;
	int events;
	struct proc *p;
{
	return sopoll(fp->f_socket, events);
}

/*ARGSUSED*/
int
soo_stat(so, ub)
	register struct socket *so;
	register struct stat *ub;
{
	bzero((caddr_t) ub, sizeof(*ub));
	ub->st_mode = S_IFSOCK;
	return ((*so->so_proto->pr_usrreq)(so, PRU_SENSE, (struct mbuf*) ub, (struct mbuf*) 0, (struct mbuf*) 0, u.u_procp));
}

int
soo_rw(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	enum uio_rw rw = uio->uio_rw;
	int error;
	if (rw == UIO_READ) {
		error = soo_read(fp, uio, cred);
	} else {
		error = soo_write(fp, uio, cred);
	}

	return (error);
}

/* ARGSUSED */
int
soo_read(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	struct socket *so;
	int flags;

	so = (struct socket *)fp->f_socket;
	flags = 0;
	return (soreceive(so, NULL, uio, flags, NULL));
}

/* ARGSUSED */
int
soo_write(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
	struct socket *so;
	int flags;

	so = (struct socket *)fp->f_socket;
	flags = 0;
	return (sosend(so, NULL, uio, flags, NULL));
}

/* ARGSUSED */
int
soo_close(fp, p)
	struct file *fp;
	struct proc *p;
{
	struct socket *so;
	int error;
	
	so = (struct socket *)fp->f_socket;
	error = 0;
	if (fp->f_socket) {
		error = soclose(so);
	}
	fp->f_socket = 0;
	return (error);
}

int
soo_kqfilter(fp, kn)
	struct file *fp;
	struct knote *kn;
{
	return (sokqfilter(fp, kn));
}
