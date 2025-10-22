/*	$NetBSD: subr_log.c,v 1.17 1998/08/18 06:27:01 thorpej Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)subr_log.c	8.3 (Berkeley) 2/14/95
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)subr_log.c	2.1 (2.11BSD) 1999/4/29
 */

/*
 * logioctl() had the wrong number of arguments.  Argh!  Apparently this
 * driver was overlooked when 'dev' was added to ioctl entry points.
 *
 * logclose() returned garbage.  this went unnoticed because most programs
 * don't check status when doing a close.

 * Remove vax ifdefs - this driver is never going back to vaxland.
 *
 * Add support for multiple log devices.  Minor device 0 is the traditional
 * kernel logger (/dev/klog), minor device 1 is reserved for the future device 
 * error logging daemon.  Minor device 2 is used by the 'accounting' daemon
 * 'acctd'.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/msgbuf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/poll.h>
#include <sys/queue.h>

#define	NLOG	3
int	nlog = NLOG;

#define LOG_RDPRI	(PZERO + 1)

#define	LOG_OPEN	0x01
#define LOG_ASYNC	0x04
#define LOG_RDWAIT	0x08

/*
 * This is an efficiency hack.  This global is provided for exit() to
 * test and avoid the overhead of function calls when accounting is
 * turned off.
*/
int	Acctopen;
struct	msgbuf	msgbuf[NLOG];
static struct logsoftc {
	int				sc_state;			/* see above for possibilities */
	struct selinfo 	*sc_selp;			/* process waiting on select call */
	int				sc_pgid;			/* process/group for async I/O */
	int				sc_overrun;			/* full buffer count */
} logsoftc[NLOG];

dev_type_open(logopen);
dev_type_close(logclose);
dev_type_read(logread);
dev_type_ioctl(logioctl);
dev_type_poll(logpoll);
dev_type_kqfilter(logkqfilter);

const struct cdevsw log_cdevsw = {
		.d_open = logopen,
		.d_close = logclose,
		.d_read = logread,
		.d_ioctl = logioctl,
		.d_poll = logpoll,
		.d_kqfilter = logkqfilter
};

/*ARGSUSED*/
int
logopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register int unit = minor(dev);

	if	(unit >= NLOG)
		return(ENODEV);
	if	(logisopen(unit))
		return(EBUSY);
	if	(msgbuf[unit].msg_click == 0)	/* no buffer allocated */
		return(ENOMEM);
	logsoftc[unit].sc_state |= LOG_OPEN;
	if (unit == logACCT)
		Acctopen = 1;
	logsoftc[unit].sc_pgid = p->p_pid;  /* signal process only */
	logsoftc[unit].sc_overrun = 0;
	return(0);
}

/*ARGSUSED*/
int
logclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register int unit = minor(dev);

	logsoftc[unit].sc_state = 0;
	if (unit == logACCT)
		Acctopen = 0;
	return (0);
}

/*
 * This is a helper function to keep knowledge of this driver's data
 * structures away from the rest of the kernel.
*/
int
logisopen(unit)
	int	unit;
{
	if (logsoftc[unit].sc_state & LOG_OPEN)
		return (1);
	return (0);
}

/*ARGSUSED*/
int
logread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct logsoftc *lp;
	register struct msgbuf *mp;
	register long l;
	register int s;
	int unit, error;

    unit = minor(dev);
    error = 0;
	lp = &logsoftc[unit];
	mp = &msgbuf[unit];
	s = splhigh();
	while (mp->msg_bufr == mp->msg_bufx) {
		if (flag & IO_NDELAY) {
			splx(s);
			return (EWOULDBLOCK);
		}
		lp->sc_state |= LOG_RDWAIT;
		if ((error = tsleep((caddr_t) mp, LOG_RDPRI | PCATCH, "klog", 0))) {
			splx(0);
			return (error);
		}
	}

	splx(s);
	lp->sc_state &= ~LOG_RDWAIT;

	while (uio->uio_resid > 0) {
		l = mp->msg_bufx - mp->msg_bufr;
		/*
		 * If the reader and writer are equal then we have caught up and there
		 * is nothing more to transfer.
		 */
		if (l == 0)
			break;

		/*
		 * If the write pointer is behind the reader then only consider as
		 * available for now the bytes from the read pointer thru the end of
		 * the buffer.
		 */
		if (l < 0) {
			l = MSG_BSIZE - mp->msg_bufr;
			/*
			 * If the reader is exactly at the end of the buffer it is
			 * time to wrap it around to the beginning and recalculate the
			 * amount of data to transfer.
			 */
			if (l == 0) {
			    if (mp->msg_bufr < 0 || mp->msg_bufr >= MSG_BSIZE) {
    			    mp->msg_bufr = 0;
        		}
				continue;
			}
		}
		l = MIN(l, uio->uio_resid);
		l = MIN(l, sizeof buf);
		error = uiomove((caddr_t)&mp->msg_bufc[mp->msg_bufr], (int)l, uio);
		if (error)
			break;
		mp->msg_bufr += l;
	}
	splx(s);
	return (error);
}

/*ARGSUSED*/
int
logpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	register int s = splhigh();
	int revents = 0;
	int	unit = minor(dev);
	if (events & (POLLIN | POLLRDNORM)) {
		if (msgbuf[unit].msg_bufr != msgbuf[unit].msg_bufx) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(p, logsoftc->sc_selp);
		}
	}
	splx(s);
	return (revents);
}

static void
filt_logrdetach(struct knote *kn)
{
	int s;

	s = splhigh();
	SIMPLEQ_REMOVE(&logsoftc->sc_selp->si_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_logread(struct knote *kn, long hint)
{

	if (msgbuf->msg_bufr == msgbuf->msg_bufx)
		return (0);

	if (msgbuf->msg_bufr < msgbuf->msg_bufx)
		kn->kn_data = msgbuf->msg_bufx - msgbuf->msg_bufr;
	else
		kn->kn_data = (msgbuf->msg_click - msgbuf->msg_bufr) +
		    msgbuf->msg_bufx;

	return (1);
}

static const struct filterops logread_filtops = {
		1,
		NULL,
		filt_logrdetach,
		filt_logread
};

int
logkqfilter(dev, kn)
	dev_t dev;
	struct knote *kn;
{
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &logsoftc->sc_selp->si_klist;
		kn->kn_fop = &logread_filtops;
		break;

	default:
		return (1);
	}
	kn->kn_hook = NULL;

	s = splhigh();
	SIMPLEQ_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

void
logwakeup(unit)
	int	unit;
{
	register struct proc *p;
	register struct logsoftc *lp;
	register struct msgbuf *mp;

	if (!logisopen(unit))
		return;
	lp = &logsoftc[unit];
	mp = &msgbuf[unit];
	if (lp->sc_selp) {
		selwakeup(u.u_procp, (long) 0);
		lp->sc_selp = 0;
	}
	if ((lp->sc_state & LOG_ASYNC) && (mp->msg_bufx != mp->msg_bufr)) {
		if (lp->sc_pgid < 0)
			gsignal(-lp->sc_pgid, SIGIO);
		else if (p == pfind(lp->sc_pgid))
			psignal(p, SIGIO);
	}
	if (lp->sc_state & LOG_RDWAIT) {
		wakeup((caddr_t) mp);
		lp->sc_state &= ~LOG_RDWAIT;
	}
}

/*ARGSUSED*/
int
logioctl(dev, com, data, flag, p)
	dev_t	dev;
	u_long	com;
	caddr_t data;
	int	flag;
	struct proc *p;
{
	long l;
	register int s;
	int	unit;
	register struct logsoftc *lp;
	register struct msgbuf *mp;

	unit = minor(dev);
	lp = &logsoftc[unit];
	mp = &msgbuf[unit];

	switch (com) {
	case FIONREAD:
		s = splhigh();
		l = mp->msg_bufx - mp->msg_bufr;
		splx(s);
		if (l < 0)
			l += MSG_BSIZE;
		*(off_t*) data = l;
		break;
	case FIONBIO:
		break;
	case FIOASYNC:
		if (*(int*) data)
			lp->sc_state |= LOG_ASYNC;
		else
			lp->sc_state &= ~LOG_ASYNC;
		break;
	case TIOCSPGRP:
		lp->sc_pgid = *(int*) data;
		break;
	case TIOCGPGRP:
		*(int*) data = lp->sc_pgid;
		break;
	default:
		return (-1);
	}
	return (0);
}

/*
 * This is inefficient for single character writes.  Alas, changing this
 * to be buffered would affect the networking code's use of printf.  
*/
int
logwrt(buf, len, log)
	char *buf;
	int	len;
	int	log;
{
	register struct msgbuf *mp = &msgbuf[log];
	struct logsoftc *lp = &logsoftc[log];
	register int infront;
	int s, n, writer, err = 0;

	if (mp->msg_magic != MSG_MAGIC || (len > MSG_BSIZE))
		return (-1);
	/*
	 * Hate to do this but since this can be called from anywhere in the kernel
	 * we have to hold off any interrupt service routines so they don't change
	 * things.  This looks like a lot of code but it isn't really.
	 */
	s = splhigh();
	while (len) {
again:
		infront = MSG_BSIZE - mp->msg_bufx;
		if (infront <= 0) {
			mp->msg_bufx = 0;
			infront = MSG_BSIZE - mp->msg_bufr;
		}
		n = mp->msg_bufr - mp->msg_bufx;
		if (n < 0) /* bufr < bufx */
			writer = (MSG_BSIZE - mp->msg_bufx) + mp->msg_bufr;
		else if (n == 0)
			writer = MSG_BSIZE;
		else {
			writer = n;
			infront = n;
		}
		if (len > writer) {
			/*
			 * won't fit.  the total number of bytes to be written is
			 * greater than the number available.  the buffer is full.
			 * throw away the old data and keep the current data by resetting
			 * the 'writer' pointer to the current 'reader' position.  Bump the
			 * overrun counter in case anyone wants to look at it for debugging.
			 */
			lp->sc_overrun++;
			mp->msg_bufx = mp->msg_bufr;
			goto again;
		}
		if (infront > len)
			infront = len;
		bcopy(buf, &mp->msg_bufc[mp->msg_bufx], infront);
		mp->msg_bufx += infront;
		len -= infront;
		buf += infront;
	}
out:
	splx(s);
	return(err);
}

void
loginit(void)
{
	register struct msgbuf *mp;
	long new_bufs;

	/* Sanity-check the given size. */
	if (MSG_BSIZE < sizeof(struct msgbuf)) {
		return;
	}

	new_bufs = MSG_BSIZE - offsetof(struct msgbuf, msg_bufc);
	for (mp = &msgbuf[0]; mp < &msgbuf[NLOG]; mp++) {
		mp->msg_click = btoc(new_bufs);
		if (!mp->msg_click) {
			return;
		}
		mp->msg_magic = MSG_MAGIC;
		mp->msg_bufc = (char*) new_bufs;
		mp->msg_bufx = mp->msg_bufr = 0;
	}
}
