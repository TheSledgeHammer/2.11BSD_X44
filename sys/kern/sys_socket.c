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

//#include <net/if.h>
//#include <net/route.h>

struct fileops socketops =
    { soo_read, soo_write, soo_ioctl, soo_select, soo_close };

/*ARGSUSED*/
int
soo_ioctl(fp, cmd, data, p)
	struct file *fp;
	int cmd;
	register caddr_t data;
	struct proc *p;
{
	register struct socket *so = (struct socket *)fp->f_data;

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
		*(int *)data = (so->so_state&SS_RCVATMARK) != 0;
		return (0);
	}
	/*
	 * Interface/routing/protocol specific ioctls:
	 * interface and routing ioctls should have a
	 * different entry since a socket's unnecessary
	 */
#define	cmdbyte(x)	(((x) >> 8) & 0xff) /* IOCGROUP */

	if (cmdbyte(cmd) == 'i')
		return(u->u_error = ifioctl(so, cmd, data));
	if (cmdbyte(cmd) == 'r')
		return(u->u_error = rtioctl(cmd, data));
	return(u->u_error = (*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
	    (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));
}

int
soo_select(fp, which, p)
	struct file *fp;
	int which;
	struct proc *p;
{
	register struct socket *so = (struct socket *)fp->f_data;
	register int s = splnet();

	switch (which) {

	case FREAD:
		if (soreadable(so)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_rcv.sb_sel);
		so->so_rcv.sb_flags |= SB_SEL;
		break;

	case FWRITE:
		if (sowriteable(so)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_snd.sb_sel);
		so->so_snd.sb_flags |= SB_SEL;
		break;

	case 0:
		if (so->so_oobmark || (so->so_state & SS_RCVATMARK)) {
			splx(s);
			return (1);
		}
		sbselqueue(&so->so_rcv.sb_sel);
		so->so_rcv.sb_flags |= SB_SEL;
		break;
	}
	splx(s);
	return (0);
}

/*ARGSUSED*/
int
soo_stat(so, ub)
	register struct socket *so;
	register struct stat *ub;
{

	bzero((caddr_t)ub, sizeof (*ub));
	return ((*so->so_proto->pr_usrreq)(so, PRU_SENSE,
	    (struct mbuf *)ub, (struct mbuf *)0, 
	    (struct mbuf *)0));
}

/* ARGSUSED */
int
soo_read(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
		return (soreceive((struct socket *)fp->f_data, (struct mbuf **)0,
				uio, (struct mbuf **)0, (struct mbuf **)0, (int *)0));
}

/* ARGSUSED */
int
soo_write(fp, uio, cred)
	struct file *fp;
	struct uio *uio;
	struct ucred *cred;
{
		return (sosend((struct socket *)fp->f_data, (struct mbuf *)0,
				uio, (struct mbuf *)0, (struct mbuf *)0, 0));
}

/* ARGSUSED */
int
soo_close(fp, p)
	struct file *fp;
	struct proc *p;
{
		int error = 0;
		if(fp->f_data)
			error = soclose((struct socket *)fp->f_data);
		fp->f_data = 0;
		return (error);
}
