/*
 * sys_mpx.c
 *
 *  Created on: 19 Dec 2019
 *      Author: marti
 *
 * Equivalent to mx2.c
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/user.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include "mpx/mx.h"

struct fileops mpxops =
        { mpx_read, mpx_write, mpx_ioctl, mpx_select, mpx_close };

struct mpx_chan chans[NCHANS];
struct mpx_group *groups[NGROUPS];
struct mpx_schan schans[NPORTS];

int	mpxline;
struct mpx_chan *xcp();
dev_t mpxdev = -1;
char mcdebugs[NDEBUGS];

int
mpx_read(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
    return (mpxreceive((struct mpx *)fp->f_data, (struct mbuf **)0, uio, (struct mbuf **)0, (struct mbuf **)0, (int *)0));
}

int
mpx_write(fp, uio, cred)
    struct file *fp;
    struct uio *uio;
    struct ucred *cred;
{
    return (mpxsend((struct mpx *)fp->f_data, (struct mbuf *)0, uio, (struct mbuf *)0, (struct mbuf *)0, (int *)0));
}

int
mpx_ioctl(fp, cmd, data, p)
    struct file *fp;
    u_long cmd;
    register caddr_t data;
    struct proc *p;
{
    register struct mpx *mpx = (struct mpx *)fp->f_data;

    switch (cmd) {
    case FIONBIO:
    	if (*(int *)data) {
    		mpx->mpx_state |= MPX_NBIO;
    	} else {
    		mpx->mpx_state &= ~MPX_NBIO;
    	}
    	return (0);
    case FIOASYNC:
    	if (*(int *)data) {
    		mpx->mpx_state |= MPX_ASYNC;
    	} else {
    		mpx->mpx_state &= ~MPX_ASYNC;
		}
    	return (0);
    case FIONREAD:
    	*(int *)data = mpx->mpx_buf->cnt;
    	return (0);
	case SIOCSPGRP:
		mpx->mpx_pgrp = *(int *)data;
		return (0);
    case SIOCGPGRP:
		*(int *)data = mpx->mpx_pgrp;
		return (0);
    }
    return ((*mpx->mpx_proto->pr_usrreq)(mpx, PRU_CONTROL, (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));
}

int
mpx_select(fp, which, p)
    struct file *fp;
    int which;
    struct proc *p;
{
    register struct mpx *mpx = (struct mpx *)fp->f_data;

    return (0);
}

int
mpx_stat(mpx, ub)
    register struct mpx *mpx;
    register struct stat *ub;
{
    bzero((caddr_t)ub, sizeof (*ub));
    return ((*mpx->mpx_proto->pr_usrreq)(mpx, PRU_SENSE, (struct mbuf *)ub, (struct mbuf *)0, (struct mbuf *)0));
}

int
mpx_close(fp, p)
    struct file *fp;
    struct proc *p;
{
    int error = 0;
    if(fp->f_data)
        error = mpxclose((struct mpx *)fp->f_data);
    fp->f_data = 0;
    return (error);
}

/* Included in mx2.c */
short	cmask[16]	={
	01,	02,	04,
	010,	020,	040,
	0100,	0200,	0400,
	01000,	02000,	04000,
	010000,	020000,	040000, 0100000
};

struct mpx_group *
getmpx(dev)
	dev_t dev;
{
	register d;

	d = minor(dev);
	if (d >= NGROUPS) {
		u->u_error = ENXIO;
		return(NULL);
	}
	return(groups[d]);
}

char mxnmbuf[NMSIZE];
int	 nmsize;

void
mpxname(cp)
	register struct mpx_chan *cp;
{
	register char *np;
	register c;

	np = mxnmbuf;
	u.u_dirp = (caddr_t)u->u_arg[0];

	while (np < &mxnmbuf[NMSIZE]) {
		c = uchar();
		if (c <= 0)
			break;
		*np++ = c;
	}
	*np++ = '\0';
	nmsize = np - mxnmbuf;

	cp->c_flags |= NMBUF;
}


