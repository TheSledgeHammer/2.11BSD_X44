/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_pty.c	1.3 (2.11BSD GTE) 1997/5/2
 */

/*
 * Pseudo-teletype Driver
 * (Actually two drivers, requiring two entries in 'cdevsw')
 */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/pty.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/poll.h>

#if NPTY == 1
#undef NPTY
#define	NPTY	16		/* crude XXX */
#endif

#define	DEFAULT_NPTYS		16	/* default number of initial ptys */
#define DEFAULT_MAXPTYS		992	/* default maximum number of ptys */

#define BUFSIZ 	100		/* Chunk size iomoved to/from user */

/*
 * pts == /dev/tty[pqrs]?
 * ptc == /dev/pty[pqrs]?
 */
struct pt_ioctl {
	struct	tty 	*pt_tty;
	int				pt_flags;
	struct selinfo  *pt_selr, *pt_selw;
	u_char			pt_send;
	u_char			pt_ucntl;
};

struct tty pt_tty[DEFAULT_NPTYS];
static struct pt_ioctl **pt_ioctl;
int	npty = DEFAULT_NPTYS;		/* for pstat -t */
int maxptys = DEFAULT_MAXPTYS;	/* maximum number of ptys (sysctable) */

#define	PF_RCOLL	0x01
#define	PF_WCOLL	0x02
#define	PF_PKT		0x08		/* packet mode */
#define	PF_STOPPED	0x10		/* user told stopped */
#define	PF_REMOTE	0x20		/* remote and flow controlled input */
#define	PF_NOSTOP	0x40
#define PF_UCNTL	0x80		/* user control mode */

void	ptyattach(int);
void	ptcwakeup(struct tty *, int);
void	ptsstart(struct tty *);
int		pty_maxptys(int, int);

static struct pt_ioctl **ptyarralloc(int);
static int check_pty(int);

dev_type_open(ptsopen);
dev_type_close(ptsclose);
dev_type_read(ptsread);
dev_type_write(ptswrite);
dev_type_poll(ptspoll);
dev_type_stop(ptsstop);

dev_type_open(ptcopen);
dev_type_close(ptcclose);
dev_type_read(ptcread);
dev_type_write(ptcwrite);
dev_type_poll(ptcpoll);

dev_type_ioctl(ptyioctl);
dev_type_tty(ptytty);

const struct cdevsw ptc_cdevsw = {
		.d_open = ptcopen,
		.d_close = ptcclose,
		.d_read = ptcread,
		.d_write = ptcwrite,
		.d_ioctl = ptyioctl,
		.d_stop = nullstop,
		.d_tty = ptytty,
		.d_poll = ptcpoll,
		.d_mmap = nommap,
		.d_type = D_TTY
};

const struct cdevsw pts_cdevsw = {
		.d_open = ptsopen,
		.d_close = ptsclose,
		.d_read = ptsread,
		.d_write = ptswrite,
		.d_ioctl = ptyioctl,
		.d_stop = ptsstop,
		.d_tty = ptytty,
		.d_poll = ptspoll,
		.d_mmap = nommap,
		.d_type = D_TTY
};

/*ARGSUSED*/
int
ptsopen(dev, flag, devtype, p)
	dev_t dev;
	int flag, devtype;
	struct proc *p;
{
	register struct tty *tp;
	const struct linesw *line;
	int error, s;

#ifdef lint
	npty = npty;
#endif

	p = u.u_procp;
	if ((error = check_pty(dev))) {
		return (error);
	}
	if (minor(dev) >= DEFAULT_NPTYS)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp); /* Set up default chars */
		tp->t_ispeed = tp->t_ospeed = EXTB;
		tp->t_flags = 0; /* No features (nor raw mode) */
	} else if ((tp->t_state & TS_XCLUDE) && u.u_uid != 0)
		return (EBUSY);
	if (tp->t_oproc) /* Ctrlr still around. */
		tp->t_state |= TS_CARR_ON;

	if (!ISSET(flag, O_NONBLOCK)) {
		s = spltty();
		TTY_LOCK(tp);
		while ((tp->t_state & TS_CARR_ON) == 0) {
			tp->t_state |= TS_WOPEN;
			sleep((caddr_t) & tp->t_rawq, TTIPRI);
		}
		TTY_UNLOCK(tp);
		splx(s);
	}
	line = linesw_lookup(tp->t_line);
	error = (*line->l_open)(dev, tp);
	ptcwakeup(tp, FREAD | FWRITE);
	return (error);
}

int
ptsclose(dev, flag, devtype, p)
	dev_t dev;
	int flag, devtype;
	struct proc *p;
{
	register struct tty *tp;
	const struct linesw *line;
	int err;

	tp = &pt_tty[minor(dev)];
	line = linesw_lookup(tp->t_line);
	err = (*line->l_close)(tp, flag);
	ttyclose(tp);
	ptcwakeup(tp, FREAD|FWRITE);
	return (err);
}
int
ptsread(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	const struct linesw *line;
	int error = 0;
	int s;

again:
	if (pti->pt_flags & PF_REMOTE) {
		while ((tp == u.u_ttyp) && (u.u_procp->p_pgrp != tp->t_pgrp) &&
				isbackground(u.u_procp, tp)) {
			if ((u.u_procp->p_sigignore & sigmask(SIGTTIN))
					|| (u.u_procp->p_sigmask & sigmask(SIGTTIN))
					|| (u.u_procp->p_flag & P_SVFORK)) {
				return (EIO);
			}
			gsignal(u.u_procp->p_pgrp->pg_id, SIGTTIN);
			s = spltty();
			TTY_LOCK(tp);
			sleep((caddr_t)&lbolt, TTIPRI);
			splx(s);
		}
		s = spltty();
		TTY_LOCK(tp);
		if (tp->t_canq.c_cc == 0) {
			if (flag & IO_NDELAY) {
				TTY_UNLOCK(tp);
				splx(s);
				return (EWOULDBLOCK);
			}
			sleep((caddr_t)&tp->t_canq, TTIPRI);
			goto again;
		}
		while (tp->t_canq.c_cc > 1 && uio->uio_resid) {
			TTY_UNLOCK(tp);
			splx(s);
			if (ureadc(getc(&tp->t_canq), uio) < 0) {
				error = EFAULT;
				s = spltty();
				TTY_LOCK(tp);
				break;
			}
		}
		if (tp->t_canq.c_cc == 1) {
			(void) getc(&tp->t_canq);
		}
		if (tp->t_canq.c_cc) {
			TTY_UNLOCK(tp);
			splx(s);
			return (error);
		}
	} else {
		if (tp->t_oproc) {
			line = linesw_lookup(tp->t_line);
			error = (*line->l_read)(tp, uio, flag);
		}
	}
	ptcwakeup(tp, FWRITE);
	return (error);
}

/*
 * Write to pseudo-tty.
 * Wakeups of controlling tty will happen
 * indirectly, when tty driver calls ptsstart.
 */
int
ptswrite(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct tty *tp;
	const struct linesw *line;

	tp = &pt_tty[minor(dev)];
	line = linesw_lookup(tp->t_line);
	if (tp->t_oproc == 0)
		return (EIO);
	return ((*line->l_write)(tp, uio, flag));
}

/*
 * Start output on pseudo-tty.
 * Wake up process selecting or sleeping for input from controlling tty.
 */
void
ptsstart(tp)
	struct tty *tp;
{
	register struct pt_ioctl *pti = pt_ioctl[minor(tp->t_dev)];

	if (tp->t_state & TS_TTSTOP)
		return;
	if (pti->pt_flags & PF_STOPPED) {
		pti->pt_flags &= ~PF_STOPPED;
		pti->pt_send = TIOCPKT_START;
	}

	selnotify(pti->pt_selr, NOTE_SUBMIT);
	ptcwakeup(tp, FREAD);
}

int
ptspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	register struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	const struct linesw *line;
	struct tty *tp = pti->pt_tty;

	if (tp->t_oproc == NULL) {
		return (POLLHUP);
	}
	line = linesw_lookup(tp->t_line);
	return ((*line->l_poll)(tp, events, p));
}

void
ptcwakeup(tp, flag)
	struct tty *tp;
	int flag;
{
	struct pt_ioctl *pti = pt_ioctl[minor(tp->t_dev)];
	int s;

	s = spltty();
	TTY_LOCK(tp);
	if (flag & FREAD) {
		if (pti->pt_selr) {
			selnotify(pti->pt_selr, NOTE_SUBMIT);
			pti->pt_selr = 0;
			pti->pt_flags &= ~PF_RCOLL;
		}
		wakeup((caddr_t)&tp->t_outq.c_cf);
	}
	if (flag & FWRITE) {
		if (pti->pt_selw) {
			selnotify(pti->pt_selw, NOTE_SUBMIT);
			pti->pt_selw = 0;
			pti->pt_flags &= ~PF_WCOLL;
		}
		wakeup((caddr_t)&tp->t_rawq.c_cf);
	}
	TTY_UNLOCK(tp);
	splx(s);
}

/*ARGSUSED*/
int
ptcopen(dev, flag, devtype, p)
	dev_t dev;
	int flag, devtype;
	struct proc *p;
{
	register struct tty *tp;
	const struct linesw *line;
	struct pt_ioctl *pti;
	int error;
	int s;

	if ((error = check_pty(dev))) {
		return (error);
	}
	if (minor(dev) >= DEFAULT_NPTYS)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];

	s = spltty();
	TTY_LOCK(tp);
	if (tp->t_oproc) {
		TTY_UNLOCK(tp);
		splx(s);
		return (EIO);
	}
	tp->t_oproc = ptsstart;
	TTY_UNLOCK(tp);
	splx(s);
	line = linesw_lookup(tp->t_line);
	(void)(*line->l_modem)(tp, 1);
	pti = pt_ioctl[minor(dev)];
	pti->pt_flags = 0;
	pti->pt_send = 0;
	pti->pt_ucntl = 0;
	return (0);
}

int
ptcclose(dev, flag, devtype, p)
	dev_t dev;
	int flag, devtype;
	struct proc *p;
{
	register struct tty *tp;
	const struct linesw *line;
	int s;

	tp = &pt_tty[minor(dev)];
	line = linesw_lookup(tp->t_line);
	(void)(*line->l_modem)(tp, 0);
	tp->t_state &= ~TS_CARR_ON;
	s = spltty();
	TTY_LOCK(tp);
	tp->t_oproc = 0;		/* mark closed */
	TTY_UNLOCK(tp);
	splx(s);
	return (0);
}

int
ptcread(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	char buf[BUFSIZ];
	int error = 0, cc;
	int s;

	/*
	 * We want to block until the slave
	 * is open, and there's something to read;
	 * but if we lost the slave or we're NBIO,
	 * then return the appropriate error instead.
	 */
	s = spltty();
	TTY_LOCK(tp);
	for (;;) {
		if (tp->t_state&TS_ISOPEN) {
			if ((pti->pt_flags & PF_PKT) && pti->pt_send) {
				TTY_UNLOCK(tp);
				splx(s);
				error = ureadc((int) pti->pt_send, uio);
				if (error)
					return (error);
				pti->pt_send = 0;
				return (0);
			}
			if ((pti->pt_flags & PF_UCNTL) && pti->pt_ucntl) {
				TTY_UNLOCK(tp);
				splx(s);
				error = ureadc((int) pti->pt_ucntl, uio);
				if (error)
					return (error);
				pti->pt_ucntl = 0;
				return (0);
			}
			if (tp->t_outq.c_cc && (tp->t_state & TS_TTSTOP) == 0)
				break;
		}
		if ((tp->t_state & TS_CARR_ON) == 0)
			return (0); /* EOF */
		if (flag & IO_NDELAY)
			return (EWOULDBLOCK);
		sleep((caddr_t) & tp->t_outq.c_cf, TTIPRI);
	}
	if (pti->pt_flags & (PF_PKT | PF_UCNTL)) {
		TTY_UNLOCK(tp);
		splx(s);
		error = ureadc(0, uio);
		s = spltty();
		TTY_LOCK(tp);
	}
	while (uio->uio_resid && error == 0) {
		cc = q_to_b(&tp->t_outq, buf, MIN(uio->uio_resid, BUFSIZ));
		if (cc <= 0)
			break;
		TTY_UNLOCK(tp);
		splx(s);
		error = uiomove(buf, cc, uio);
		s = spltty();
		TTY_LOCK(tp);
	}
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t) & tp->t_outq);
		}
		selnotify(&tp->t_wsel, NOTE_SUBMIT);
		tp->t_state &= ~TS_WCOLL;
	}
	TTY_UNLOCK(tp);
	splx(s);
	return (error);
}

int
ptsstop(tp, flush)
	register struct tty *tp;
	int flush;
{
	struct pt_ioctl *pti = pt_ioctl[minor(tp->t_dev)];
	int flag;

	/* note: FLUSHREAD and FLUSHWRITE already ok */
	if (flush == 0) {
		flush = TIOCPKT_STOP;
		pti->pt_flags |= PF_STOPPED;
	} else {
		pti->pt_flags &= ~PF_STOPPED;
	}
	pti->pt_send |= flush;
	/* change of perspective */
	flag = 0;
	if (flush & FREAD) {
		flag |= FWRITE;
		selnotify(pti->pt_selw, NOTE_SUBMIT);
		wakeup((caddr_t)&tp->t_rawq.c_cf);
	}
	if (flush & FWRITE) {
		flag |= FREAD;
		selnotify(pti->pt_selr, NOTE_SUBMIT);
		wakeup((caddr_t)&tp->t_outq.c_cf);
	}
	ptcwakeup(tp, flag);
	return (0);
}

int
ptcpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	const struct linesw *line;
	struct tty *tp = pti->pt_tty;
	int revents = 0;
	int s = splsoftclock();

	if (events & (POLLIN | POLLRDNORM))
		if (ISSET(tp->t_state, TS_ISOPEN) &&
		    ((tp->t_outq.c_cc > 0 && !ISSET(tp->t_state, TS_TTSTOP)) ||
		     ((pti->pt_flags & PF_PKT) && pti->pt_send) ||
		     ((pti->pt_flags & PF_UCNTL) && pti->pt_ucntl)))
			revents |= events & (POLLIN | POLLRDNORM);

	if (events & (POLLOUT | POLLWRNORM))
		if (ISSET(tp->t_state, TS_ISOPEN) &&
		    ((pti->pt_flags & PF_REMOTE) ?
		     (tp->t_canq.c_cc == 0) :
		     ((tp->t_rawq.c_cc + tp->t_canq.c_cc < TTYHOG-2) ||
		      (tp->t_canq.c_cc == 0 && ISSET(tp->t_lflag, ICANON)))))
			revents |= events & (POLLOUT | POLLWRNORM);

	if (events & POLLHUP)
		if (!ISSET(tp->t_state, TS_CARR_ON))
			revents |= POLLHUP;

	if (revents == 0) {
		if (events & (POLLIN | POLLHUP | POLLRDNORM))
			selrecord(p, pti->pt_selr);

		if (events & (POLLOUT | POLLWRNORM))
			selrecord(p, pti->pt_selw);
	}

	splx(s);
	return (revents);
}

int
ptcwrite(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	const struct linesw *line;
	register char *cp;
	register int cc = 0;
	char locbuf[BUFSIZ];
	int cnt = 0;
	struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	int error = 0;
	int s;

again:
	s = spltty();
	TTY_LOCK(tp);
	if ((tp->t_state&TS_ISOPEN) == 0)
		goto block;
	if (pti->pt_flags & PF_REMOTE) {
		if (tp->t_canq.c_cc)
			goto block;
		while (uio->uio_resid && tp->t_canq.c_cc < TTYHOG - 1) {
			if (cc == 0) {
				cc = MIN(uio->uio_resid, BUFSIZ);
				cc = MIN(cc, TTYHOG - 1 - tp->t_canq.c_cc);
				cp = locbuf;
				TTY_UNLOCK(tp);
				splx(s);
				error = uiomove(cp, cc, uio);
				if (error)
					return (error);
				s = spltty();
				TTY_LOCK(tp);
				/* check again for safety */
				if ((tp->t_state&TS_ISOPEN) == 0)
					return (EIO);
			}
			if (cc)
				(void) b_to_q(cp, cc, &tp->t_canq);
			cc = 0;
		}
		(void) putc(0, &tp->t_canq);
		ttwakeup(tp);
		wakeup((caddr_t)&tp->t_canq);
		return (0);
	}
	line = linesw_lookup(tp->t_line);
	while (uio->uio_resid > 0) {
		if (cc == 0) {
			cc = MIN(uio->uio_resid, BUFSIZ);
			cp = locbuf;
			TTY_UNLOCK(tp);
			splx(s);
			error = uiomove(cp, cc, uio);
			if (error)
				return (error);
			s = spltty();
			TTY_LOCK(tp);
			/* check again for safety */
			if ((tp->t_state&TS_ISOPEN) == 0)
				return (EIO);
		}
		while (cc > 0) {
			if ((tp->t_rawq.c_cc + tp->t_canq.c_cc) >= TTYHOG - 2 &&
			   (tp->t_canq.c_cc > 0 ||
			      (tp->t_flags & (RAW|CBREAK)))) {
				wakeup((caddr_t)&tp->t_rawq);
				goto block;
			}
			TTY_UNLOCK(tp);
			splx(s);
			(*line->l_rint)(*cp++, tp);
			s = spltty();
			TTY_LOCK(tp);
			cnt++;
			cc--;
		}
		cc = 0;
	}
	goto out;

block:
	/*
	 * Come here to wait for slave to open, for space
	 * in outq, or space in rawq.
	 */
	if ((tp->t_state & TS_CARR_ON) == 0)
		return (EIO);
	if (flag & IO_NDELAY) {
		/* adjust for data copied in but not written */
		uio->uio_resid += cc;
		if (cnt == 0)
			return (EWOULDBLOCK);
		return (0);
	}
	sleep((caddr_t)&tp->t_rawq.c_cf, TTOPRI);
	goto again;

out:
	TTY_UNLOCK(tp);
	splx(s);
	return (0);
}

/*
 * Allocate and zero array of nelem elements.
 */
static struct pt_ioctl **
ptyarralloc(nelem)
	int nelem;
{
	struct pt_ioctl **pt;

	nelem += 10;
	pt = calloc(nelem, sizeof *pt, M_DEVBUF, M_WAITOK | M_ZERO);
	return (pt);
}

/*
 * Check if the minor is correct and ensure necessary structures
 * are properly allocated.
 */
static int
check_pty(ptn)
	int ptn;
{
	struct pt_ioctl *pti;
	
	if (ptn >= npty) {
		struct pt_ioctl **newpt, **oldpt;
		int newnpty;

		/* check if the requested pty can be granted */
		if (ptn >= maxptys) {
	limit_reached: 
			tablefull("increase kern.maxptys");
			return (ENXIO);
		}

		/* Allocate a larger pty array */
		for (newnpty = npty; newnpty <= ptn;)
			newnpty *= 2;
		if (newnpty > maxptys)
			newnpty = maxptys;
		newpt = ptyarralloc(newnpty);

		/*
		 * Now grab the pty array mutex - we need to ensure
		 * that the pty array is consistent while copying it's
		 * content to newly allocated, larger space; we also
		 * need to be safe against pty_maxptys().
		 */
		//simple_lock(&pt_softc_mutex);

		if (newnpty >= maxptys) {
			/* limit cut away beneath us... */
			newnpty = maxptys;
			if (ptn >= newnpty) {
				//simple_unlock(&pt_softc_mutex);
				free(newpt, M_DEVBUF);
				goto limit_reached;
			}
		}

		/*
		 * If the pty array was not enlarged while we were waiting
		 * for mutex, copy current contents of pt_softc[] to newly
		 * allocated array and start using the new bigger array.
		 */
		if (newnpty > npty) {
			memcpy(newpt, pt_ioctl, npty * sizeof(struct pt_ioctl *));
			oldpt = pt_ioctl;
			pt_ioctl = newpt;
			npty = newnpty;
		} else {
			/* was enlarged when waited for lock, free new space */
			oldpt = newpt;
		}

		//simple_unlock(&pt_softc_mutex);
		free(oldpt, M_DEVBUF);
	}

	/*
	 * If the entry is not yet allocated, allocate one. The mutex is
	 * needed so that the state of pt_softc[] array is consistant
	 * in case it has been lengthened above.
	 */
	if (!pt_ioctl[ptn]) {
		MALLOC(pti, struct pt_ioctl *, sizeof(struct pt_ioctl), M_DEVBUF, M_WAITOK);
		bzero(pti, sizeof(struct pt_ioctl));

		pti->pt_tty = ttymalloc();

//		simple_lock(&pt_softc_mutex);

		/*
		 * Check the entry again - it might have been
		 * added while we were waiting for mutex.
		 */
		if (!pt_ioctl[ptn]) {
			tty_init_console(pti->pt_tty, 1000000);
			//tty_attach(pti->pt_tty);
			pt_ioctl[ptn] = pti;
		} else {
			ttyfree(pti->pt_tty);
			free(pti, M_DEVBUF);
		}

//		simple_unlock(&pt_softc_mutex);
	}

	return (0);
}

/*
 * Set maxpty in thread-safe way. Returns 0 in case of error, otherwise
 * new value of maxptys.
 */
int
pty_maxptys(newmax, set)
	int newmax, set;
{
	if (!set)
		return (maxptys);

	/*
	 * We have to grab the pt_softc lock, so that we would pick correct
	 * value of npty (might be modified in check_pty()).
	 */
//	simple_lock(&pt_softc_mutex);

	/*
	 * The value cannot be set to value lower than the highest pty
	 * number ever allocated.
	 */
	if (newmax >= npty) {
		maxptys = newmax;
	} else {
		newmax = 0;
	}

//	simple_unlock(&pt_softc_mutex);

	return newmax;
}

/*
 * Establish n (or default if n is 1) ptys in the system.
 */
void
ptyattach(num)
	int num;
{
	/* maybe should allow 0 => none? */
	if (num <= 1)
		num = DEFAULT_NPTYS;
	pt_ioctl = ptyarralloc(num);
	npty = num;
}

struct tty *
ptytty(dev)
	dev_t dev;
{
	struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	struct tty *tp = pti->pt_tty;

	return (tp);
}

/*ARGSUSED*/
int
ptyioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = pt_ioctl[minor(dev)];
	const struct cdevsw *cdev;
	const struct linesw *line;
	int stop, error;

	/*
	 * IF CONTROLLER STTY THEN MUST FLUSH TO PREVENT A HANG.
	 * ttywflush(tp) will hang if there are characters in the outq.
	 */
	cdev = cdevsw_lookup(dev);
	if (cdev->d_open == ptcopen) {
		switch (cmd) {

		case TIOCPKT:
			if (*(int*) data) {
				if (pti->pt_flags & PF_UCNTL)
					return (EINVAL);
				pti->pt_flags |= PF_PKT;
			} else
				pti->pt_flags &= ~PF_PKT;
			return (0);

		case TIOCUCNTL:
			if (*(int*) data) {
				if (pti->pt_flags & PF_PKT)
					return (EINVAL);
				pti->pt_flags |= PF_UCNTL;
			} else
				pti->pt_flags &= ~PF_UCNTL;
			return (0);

		case TIOCREMOTE:
			if (*(int*) data)
				pti->pt_flags |= PF_REMOTE;
			else
				pti->pt_flags &= ~PF_REMOTE;
			ttyflush(tp, FREAD | FWRITE);
			return (0);

		case TIOCSETP:
		case TIOCSETN:
		case TIOCSETD:
			while (getc(&tp->t_outq) >= 0)
				;
			break;
		}
	}
	/*
	 * Unsure if the comment below still applies or not.  For now put the
	 * new code in ifdef'd out.
	 */
	line = linesw_lookup(tp->t_line);
#ifdef	four_four_bsd
	error = (*line->l_ioctl)(tp, cmd, data, flag, p);
	if (error < 0)
		error = ttioctl(tp, cmd, data, flag, p);
#else
	error = ttioctl(tp, cmd, data, flag, p);
	/*
	 * Since we use the tty queues internally,
	 * pty's can't be switched to disciplines which overwrite
	 * the queues.  We can't tell anything about the discipline
	 * from here...
	 */
	if (line->l_rint != ttyinput) {
		(*line->l_close)(tp, flag);
		tp->t_line = 0;
		(void)(*line->l_open)(dev, tp);
		error = ENOTTY;
	}
#endif
	if (error < 0) {
		if ((pti->pt_flags & PF_UCNTL) &&
		    (cmd & ~0xff) == UIOCCMD(0)) {
			if (cmd & 0xff) {
				pti->pt_ucntl = (u_char)cmd;
				ptcwakeup(tp, FREAD);
			}
			return (0);
		}
		error = ENOTTY;
	}
	stop = (tp->t_flags & RAW) == 0 &&
	    tp->t_stopc == CTRL(s) && tp->t_startc == CTRL(q);
	if (pti->pt_flags & PF_NOSTOP) {
		if (stop) {
			pti->pt_send &= ~TIOCPKT_NOSTOP;
			pti->pt_send |= TIOCPKT_DOSTOP;
			pti->pt_flags &= ~PF_NOSTOP;
			ptcwakeup(tp, FREAD);
		}
	} else {
		if (!stop) {
			pti->pt_send &= ~TIOCPKT_DOSTOP;
			pti->pt_send |= TIOCPKT_NOSTOP;
			pti->pt_flags |= PF_NOSTOP;
			ptcwakeup(tp, FREAD);
		}
	}
	return (error);
}
