/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty.c	1.5 (2.11BSD GTE) 1997/5/4
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#define	TTYDEFCHARS
#include <sys/tty.h>
#undef	TTYDEFCHARS
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/devsw.h>
#include <sys/dk.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/event.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/sysctl.h>
#include <sys/msgbuf.h>
#include <sys/unistd.h>
#include <sys/lock.h>
#include <sys/map.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/user.h>
#include <dev/misc/cons/cons.h>

#include <vm/include/vm.h>

/* Symbolic sleep message strings. */
char ttclos[]	= "ttycls";
char ttopen[]	= "ttyopn";
char ttybg[]	= "ttybg";
char ttyin[]	= "ttyin";
char ttyout[]	= "ttyout";

#define	CONNECTED(tp)	(ISSET(tp->t_state, TS_CARR_ON) ||	\
			 ISSET(tp->t_cflag, CLOCAL | MDMBUF))

/*
 * These were moved here from tty.h so that they could be easily modified
 * and/or patched instead of recompiling the kernel.  There is only 1 other
 * place which references these - see tty_pty.c
 *
 * The block and unblock numbers may look low but certain devices (the DHV-11
 * for example) have poor silo handling and at high data rates (19200) the
 * raw queue overflows even though we've stopped the sending device.  At 192
 * characters for the 'block' point c-kermit would regularily see dropped data
 * during interactive mode at 19200.
 *
 * It would be nice to have a larger than 8kb clist area and raise these limits
 * but that would require 2 mapping registers and/or a rewrite of the entire
 * clist handling.
*/

int	TTYBLOCK = 			128;
int	TTYUNBLOCK = 		64;

#define	E				0x00	/* Even parity. */
#define	O				0x80	/* Odd parity. */
#define	PARITY(c)		(char_type[c] & O)

#define	ALPHA			0x40	/* Alpha or underscore. */
#define	ISALPHA(c)		(char_type[(c) & TTY_CHARMASK] & ALPHA)

#define	CCLASSMASK		0x3f
#define	CCLASS(c)		(char_type[c] & CCLASSMASK)

#define	BS				BACKSPACE
#define	CC				CONTROL
#define	CR				RETURN
#define	NA				ORDINARY | ALPHA
#define	NL				NEWLINE
#define	NO				ORDINARY
#define	TB				TAB
#define	VT				VTAB

char const char_type[] = {
	E|CC, O|CC, O|CC, E|CC, O|CC, E|CC, E|CC, O|CC,	/* nul - bel */
	O|BS, E|TB, E|NL, O|CC, E|VT, O|CR, O|CC, E|CC, /* bs - si */
	O|CC, E|CC, E|CC, O|CC, E|CC, O|CC, O|CC, E|CC, /* dle - etb */
	E|CC, O|CC, O|CC, E|CC, O|CC, E|CC, E|CC, O|CC, /* can - us */
	O|NO, E|NO, E|NO, O|NO, E|NO, O|NO, O|NO, E|NO, /* sp - ' */
	E|NO, O|NO, O|NO, E|NO, O|NO, E|NO, E|NO, O|NO, /* ( - / */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA, /* 0 - 7 */
	O|NA, E|NA, E|NO, O|NO, E|NO, O|NO, O|NO, E|NO, /* 8 - ? */
	O|NO, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA, /* @ - G */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA, /* H - O */
	E|NA, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA, /* P - W */
	O|NA, E|NA, E|NA, O|NO, E|NO, O|NO, O|NO, O|NA, /* X - _ */
	E|NO, O|NA, O|NA, E|NA, O|NA, E|NA, E|NA, O|NA, /* ` - g */
	O|NA, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA, /* h - o */
	O|NA, E|NA, E|NA, O|NA, E|NA, O|NA, O|NA, E|NA, /* p - w */
	E|NA, O|NA, O|NA, E|NO, O|NO, E|NO, E|NO, O|CC, /* x - del */
	/*
	 * Meta chars; should be settable per character set;
	 * for now, treat them all as normal characters.
	 */
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
	NA,   NA,   NA,   NA,   NA,   NA,   NA,   NA,
};
#undef	BS
#undef	CC
#undef	CR
#undef	NA
#undef	NL
#undef	NO
#undef	TB
#undef	VT

short tthiwat[NSPEEDS] = {
		100, 100, 100, 100, 100, 100, 100, 200, 200, 400, 400, 400, 650, 650, 1300, 2000
};

short ttlowat[NSPEEDS] = {
		30, 30, 30, 30, 30, 30, 30, 50, 50, 120, 120, 120, 125, 125, 125, 125
};

struct ttychars ttydefaults = {
	CERASE,	CKILL,	CINTR,	CQUIT,	CSTART,	 CSTOP,  CEOF, CBRK,
	CSUSP,	CDSUSP, CRPRNT, CFLUSH, CWERASE, CLNEXT, CMIN, CTIME
};

long tk_nin;
long tk_nout;
long tk_cancc;
long tk_rawcc;

static int  ttnread(struct tty *);
static void ttyrubo(struct tty *, int);
static void ttyecho(int, struct tty *);
static void tty_init_termios(struct tty *);
static int  proc_compare(struct proc *, struct proc *);

void
ttychars(tp)
	struct tty *tp;
{
	tp->t_chars = ttydefaults;
}

/*
 * Wait for output to drain, then flush input waiting.
 */
int
ttywflush(tp)
	register struct tty *tp;
{
	int	error;
	int	s;
	if ((error = ttywait(tp)) == 0) {
		s = spltty();
		TTY_LOCK(tp);
		ttyflush(tp, FREAD);
		TTY_UNLOCK(tp);
		splx(s);
	}
	return (error);
}

int
ttywait(tp)
	register struct tty *tp;
{
	int	error, s;

	error = 0;
	s = spltty();
	TTY_LOCK(tp);
	while ((tp->t_outq.c_cc || (tp->t_state & TS_BUSY))
			&& (tp->t_state & TS_CARR_ON) && tp->t_oproc) {
		(*tp->t_oproc)(tp);
		/*
		 * If the output routine drains the queue and the device is no longer busy
		 * then don't wait for something that's already happened.
		 */
		if (tp->t_outq.c_cc == 0 && !ISSET(tp->t_state, TS_BUSY))
			break;
		SET(tp->t_state, TS_ASLEEP);
		error = ttysleep(tp, &tp->t_outq, TTOPRI | PCATCH, ttyout, 0);
		if(error) {
			break;
		}
	}
	TTY_UNLOCK(tp);
	splx(s);
	return (error);
}

/*
 * Flush all TTY queues
 */
void
ttyflush(tp, rw)
	register struct tty *tp;
	int rw;
{
	const struct cdevsw *cdev;
	register int s;

	s = spltty();
	if (rw & FREAD) {
		while (getc(&tp->t_canq) >= 0)
			;
		while (getc(&tp->t_rawq) >= 0)
			;
		tp->t_rocount = 0;
		tp->t_rocol = 0;
		CLR(tp->t_state, TS_LOCAL);
		ttwakeup(tp);
	}
	if (rw & FWRITE) {
		cdev = cdevsw_lookup(tp->t_dev);
		tp->t_state &= ~TS_TTSTOP;
		(*cdev->d_stop)(tp, rw);
		while (getc(&tp->t_outq) >= 0)
			;
		wakeup((caddr_t)&tp->t_outq);
		selnotify(&tp->t_wsel, NOTE_SUBMIT);
		CLR(tp->t_state, TS_WCOLL);
	}
	if ((rw & FREAD) && ISSET(tp->t_state, TS_TBLOCK))
		ttyunblock(tp);
	splx(s);
}

static int	rts = TIOCM_RTS;

/*
 * Send stop character on input overflow.
 */
void
ttyblock(tp)
	register struct tty *tp;
{
	const struct cdevsw *cdev;
	register int total;

	cdev = cdevsw_lookup(tp->t_dev);
	total = tp->t_rawq.c_cc + tp->t_canq.c_cc;
	/*
	 * Block further input iff:
	 * Current input > threshold AND input is available to user program
	 */
	if (total >= TTYBLOCK
			&& ((tp->t_flags & (RAW | CBREAK)) || (tp->t_canq.c_cc > 0))
			&& (tp->t_state & TS_TBLOCK) == 0) {
		/*
		 * TANDEM is the same as IXOFF for all intents and purposes.  Since we could
		 * get called for either software or hardware flow control we need to check
		 * the IXOFF bit.
		 */
		if (ISSET(tp->t_flags, TANDEM) && tp->t_stopc != _POSIX_VDISABLE
				&& putc(tp->t_stopc, &tp->t_outq) == 0) {
			SET(tp->t_state, TS_TBLOCK);
			ttstart(tp);
		}
		/*
		 * If queue is full, drop RTS to tell modem to stop sending us stuff
		 */
		if (ISSET(tp->t_flags, RTSCTS)
				&& (*cdev->d_ioctl)(tp->t_dev, TIOCMBIC, (char *)rts, 0, u.u_procp) == 0) {
			SET(tp->t_state, TS_TBLOCK);
		}
	}
}

void
ttyunblock(tp)
	register struct tty *tp;
{
	const struct cdevsw *cdev;
	register int s;

	s = spltty();
	cdev = cdevsw_lookup(tp->t_dev);
	if (ISSET(tp->t_flags, TANDEM) && tp->t_startc != _POSIX_VDISABLE
			&& putc(tp->t_startc, &tp->t_outq) == 0) {
		CLR(tp->t_state, TS_TBLOCK);
		ttstart(tp);
	}
	if (ISSET(tp->t_flags, RTSCTS)
			&& (*cdev->d_ioctl)(tp->t_dev, TIOCMBIS, (char *)rts, 0, u.u_procp) == 0) {
		CLR(tp->t_state, TS_TBLOCK);
	}
}

/*
 * Restart typewriter output following a delay timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
void
_ttrstrt(tp)
	register struct tty *tp;
{
	int		s;
	s = spltty();
	TTY_LOCK(tp);
	tp->t_state &= ~TS_TIMEOUT;
	ttstart(tp);
	TTY_UNLOCK(tp);
	splx(s);
}

void
ttrstrt(tp_arg)
	void *tp_arg;
{
	register struct tty *tp;

#ifdef DIAGNOSTIC
	if (tp_arg == NULL) {
		panic("ttrstrt");
	}
#endif
	tp = tp_arg;
	_ttrstrt(tp);
}

/*
 * Start output on the typewriter. It is used from the top half
 * after some characters have been put on the output queue,
 * from the interrupt routine to transmit the next
 * character, and after a timeout has finished.
 *
 * The spl calls were removed because the priority should already be spltty.
 */
int
ttstart(tp)
	register struct tty *tp;
{
	if (tp->t_oproc) /* kludge for pty */
		(*tp->t_oproc)(tp);
	return (0);
}

/*
 * Common code for tty ioctls.
 */
/*ARGSUSED*/
int
ttioctl_sc(tp, com, data, flag)
	register struct tty *tp;
	u_long com;
	caddr_t data;
	int flag;
{
	extern struct tty *constty; /* Temporary virtual console. */
	const struct cdevsw *cdev;
	const struct linesw *line;
	struct nameidata nd;
	int s, error;
	long newflags;
	int dev;
	
	dev = tp->t_dev;
	/*
	 * If the ioctl involves modification,
	 * hang if in the background.
	 */
	switch (com) {
	case TIOCSETA:
    case TIOCSETAF:
    case TIOCSETAW:
	case TIOCSETD:
	case TIOCSETP:
	case TIOCSETN:
	case TIOCFLUSH:
	case TIOCSETC:
	case TIOCSLTC:
		while (isbackground(curproc, tp) && u.u_procp->p_pgrp->pg_jobc
				&& (u.u_procp->p_flag & P_PPWAIT) == 0
				&& (u.u_procp->p_sigignore & sigmask(SIGTTOU)) == 0
				&& (u.u_procp->p_sigmask & sigmask(SIGTTOU)) == 0) {
			pgsignal(u.u_procp->p_pgrp, SIGTTOU, 1);
			s = spltty();
			TTY_LOCK(tp);
			error = ttysleep(tp, &lbolt, TTOPRI | PCATCH, ttybg, 0);
			splx(s);
			if (error) {
				return (error);
			}
		}
		break;
	case TIOCSPGRP:
	case TIOCLBIS:
	case TIOCSTI:
	case TIOCSWINSZ:
		while (tp->t_line == NTTYDISC && u.u_procp->p_pgrp != tp->t_pgrp
				&& tp == u.u_ttyp && (u.u_procp->p_flag & P_SVFORK) == 0
				&& !(u.u_procp->p_sigignore & sigmask(SIGTTOU))
				&& !(u.u_procp->p_sigmask & sigmask(SIGTTOU))) {
			gsignal(u.u_procp->p_pgrp->pg_id, SIGTTOU);
			error = ttysleep(tp, &lbolt, TTOPRI, ttybg, 0);
			if (error) {
				return (error);
			}
		}
		break;
	}

	/*
	 * Process the ioctl.
	 */
	switch (com) { 			/* Process the ioctl. */
	case FIOASYNC: 			/* set/clear async i/o */
		s = spltty();
		TTY_LOCK(tp);
		if (*(int *)data)
			tp->t_state |= TS_ASYNC;
		else
			tp->t_state &= ~TS_ASYNC;
		TTY_UNLOCK(tp);
		splx(s);
		break;

	case FIONBIO: 			/* set/clear non-blocking i/o */
	case FIONREAD: 			/* get # bytes to read */
		s = spltty();
		TTY_LOCK(tp);
		*(off_t*) data = ttnread(tp);
		TTY_UNLOCK(tp);
		splx(s);
		break;

	case TIOCEXCL: 			/* set exclusive use of tty */
		s = spltty();
		TTY_LOCK(tp);
		tp->t_state |= TS_XCLUDE;
		splx(s);
		TTY_UNLOCK(tp);
		break;

	case TIOCFLUSH: { 		/* flush buffers */
		register int flags = *(int *)data;

		if (flags == 0)
			flags = FREAD|FWRITE;
		else
			flags &= FREAD|FWRITE;
		s = spltty();
		TTY_LOCK(tp);
		ttyflush(tp, flags);
		TTY_UNLOCK(tp);
		splx(s);
		break;
	}

	case TIOCCONS: 			/* become virtual console */
		if (*(int*) data) {
			if (constty && constty != tp
					&& ISSET(constty->t_state, TS_CARR_ON | TS_ISOPEN)
							== (TS_CARR_ON | TS_ISOPEN))
				return (EBUSY);
			NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE, "/dev/console", u.u_procp);
			if ((error = namei(&nd)) != 0)
				return error;
			error = VOP_ACCESS(nd.ni_vp, VREAD, u.u_ucred, u.u_procp);
			vput(nd.ni_vp);
			if (error)
				return error;

#ifndef	UCONSOLE
			if (error = suser())
				return (error);
#endif
			constty = tp;
		} else if (tp == constty)
			constty = NULL;
		break;

	case TIOCDRAIN: 		/* wait till output drained */
		if (error = ttywait(tp))
			return (error);
		break;

	case TIOCGETA: { 		/* get termios struct */
		struct termios *t = (struct termios*) data;

		bcopy(&tp->t_termios, t, sizeof(struct termios));
		break;
	}

	case TIOCGETD: 			/* get line discipline */
		*(int *)data = tp->t_line;
		break;

	case TIOCGETP: {		/* send current parameters to user */
		register struct sgttyb *sg = (struct sgttyb *)data;

		sg->sg_ispeed = tp->t_ispeed;
		sg->sg_ospeed = tp->t_ospeed;
		sg->sg_erase = tp->t_erase;
		sg->sg_kill = tp->t_kill;
		sg->sg_flags = tp->t_flags;
		break;
	}

	case TIOCGWINSZ:	 	/* get window size */
		*(struct winsize *)data = tp->t_winsize;
		break;

	case FIOGETOWN:
		if (!isctty(u.u_procp, tp))
			return (ENOTTY);
		*(int*) data = tp->t_pgrp ? -tp->t_pgrp->pg_id : 0;
		break;

	case TIOCGPGRP: 		/* get pgrp of tty */
		if (!isctty(u.u_procp, tp))
			return (ENOTTY);
		*(int*) data = tp->t_pgrp ? tp->t_pgrp->pg_id : NO_PID;
		break;

	case TIOCGSID:			/* get sid of tty */
		if (!isctty(u.u_procp, tp))
			return (ENOTTY);
		*(int *)data = tp->t_session->s_leader->p_pid;
		break;

#ifdef TIOCHPCL
	case TIOCHPCL: 			/* hang up on last close */
		s = spltty();
		TTY_LOCK(tp);
		tp->t_state |= TS_HUPCLS;
		TTY_UNLOCK(tp);
		splx(s);
		break;
#endif
	case TIOCNXCL: 			/* reset exclusive use of tty */
		s = spltty();
		TTY_LOCK(tp);
		tp->t_state &= ~TS_XCLUDE;
		TTY_UNLOCK(tp);
		splx(s);
		break;

	case TIOCOUTQ: 			/* output queue size */
		*(int *)data = tp->t_outq.c_cc;
		break;

	case TIOCSETA: 			/* set termios struct */
	case TIOCSETAW: 		/* drain output, set */
	case TIOCSETAF: { 		/* drn out, fls in, set */
		register struct termios *t = (struct termios*) data;

		s = spltty();
		if (com == TIOCSETAW || com == TIOCSETAF) {
			if ((error = ttywait(tp))) {
				splx(s);
				return (error);
			}
			if (com == TIOCSETAF)
				s = spltty();
			TTY_LOCK(tp);
			ttyflush(tp, FREAD);
			TTY_UNLOCK(tp);
			splx(s);
		}
		if (!ISSET(t->c_cflag, CIGNORE)) {
			/*
			 * Set device hardware.
			 */
			if (tp->t_param && (error = (*tp->t_param)(tp, t))) {
				splx(s);
				return (error);
			} else {
				if (!ISSET(tp->t_state, TS_CARR_ON)
						&& ISSET(tp->t_cflag, CLOCAL)
						&& !ISSET(t->c_cflag, CLOCAL)) {
					CLR(tp->t_state, TS_ISOPEN);
					SET(tp->t_state, TS_WOPEN);
					ttwakeup(tp);
				}
				tp->t_cflag = t->c_cflag;
				tp->t_ispeed = t->c_ispeed;
				tp->t_ospeed = t->c_ospeed;
			}
			ttsetwater(tp);
		}
		TTY_LOCK(tp);
		if (com != TIOCSETAF) {
			if (ISSET(t->c_lflag, ICANON) != ISSET(tp->t_lflag, ICANON))
				if (ISSET(t->c_lflag, ICANON)) {
					SET(tp->t_lflag, PENDIN);
					ttwakeup(tp);
				} else {
					struct clist tq;

					catq(&tp->t_rawq, &tp->t_canq);
					tq = tp->t_rawq;
					tp->t_rawq = tp->t_canq;
					tp->t_canq = tq;
					CLR(tp->t_lflag, PENDIN);
				}
		}
		tp->t_iflag = t->c_iflag;
		tp->t_oflag = t->c_oflag;
		/*
		 * Make the EXTPROC bit read only.
		 */
		if (ISSET(tp->t_lflag, EXTPROC))
			SET(t->c_lflag, EXTPROC);
		else
			CLR(t->c_lflag, EXTPROC);
		tp->t_lflag = t->c_lflag | ISSET(tp->t_lflag, PENDIN);
		bcopy(t->c_cc, tp->t_cc, sizeof(t->c_cc));
		TTY_UNLOCK(tp);
		splx(s);
		break;
	}

	case TIOCSETD: { 		/* set line discipline */
		register int t = *(int *)data;
		int ndisp = devnum(LINETYPE);
		int error = 0;

		line = linesw_lookup(dev);
		if ((unsigned) t >= ndisp)
			return (ENXIO);
		if (t != tp->t_line) {
			s = spltty();
			(*line->l_close)(tp, flag);
			error = (*line->l_open)(dev, tp);
			if (error) {
				(void) (*line->l_open)(dev, tp);
				splx(s);
				return (error);
			}
			tp->t_line = t;
			splx(s);
		}
		break;
	}

	case TIOCSETN: {
		register struct sgttyb *sg = (struct sgttyb *)data;

		tp->t_erase = sg->sg_erase;
		tp->t_kill = sg->sg_kill;
		tp->t_ispeed = sg->sg_ispeed;
		tp->t_ospeed = sg->sg_ospeed;
		newflags = (tp->t_flags & 0xffff0000) | (sg->sg_flags & 0xffffL);

		s = spltty();
		TTY_LOCK(tp);
		if ((tp->t_flags & RAW) || (newflags & RAW) || com == TIOCSETP) {
			ttywait(tp);
			ttyflush(tp, FREAD);
		} else if ((tp->t_flags & CBREAK) != (newflags & CBREAK)) {
			if (newflags & CBREAK) {
				struct clist tq;

				catq(&tp->t_rawq, &tp->t_canq);
				tq = tp->t_rawq;
				tp->t_rawq = tp->t_canq;
				tp->t_canq = tq;
			} else {
				tp->t_flags |= PENDIN;
				newflags |= PENDIN;
				ttwakeup(tp);
			}
		}
		tp->t_flags = newflags;
		if (tp->t_flags&RAW) {
			tp->t_state &= ~TS_TTSTOP;
			ttstart(tp);
		}
		TTY_UNLOCK(tp);
		splx(s);
		break;
	}

	case TIOCSTART: 		/* start output, like ^Q */
		s = spltty();
		TTY_LOCK(tp);
		if ((tp->t_state & TS_TTSTOP) || (tp->t_flags & FLUSHO)) {
			tp->t_state &= ~TS_TTSTOP;
			tp->t_flags &= ~FLUSHO;
			ttstart(tp);
		}
		TTY_UNLOCK(tp);
		splx(s);
		break;

	case TIOCSTI: 			/* simulate terminal input */
    	line = linesw_lookup(dev);
		if (u.u_uid && (flag & FREAD) == 0)
			return (EPERM);
		if (u.u_uid && u.u_ttyp != tp)
			return (EACCES);
		if(u.u_uid && !isctty(u.u_procp, tp)) {
			return (EACCES);
		}
		(*line->l_rint)(*(char *)data, tp);
		break;

	case TIOCSTOP: 			/* stop output, like ^S */
		s = spltty();
		cdev = cdevsw_lookup(dev);
		TTY_LOCK(tp);
		if ((tp->t_state&TS_TTSTOP) == 0) {
			tp->t_state |= TS_TTSTOP;
			(*cdev->d_stop)(tp, 0);
		}
		TTY_UNLOCK(tp);
		splx(s);
		break;

	case TIOCSCTTY: 		/* become controlling tty */
		/* Session ctty vnode pointer set in vnode layer. */
		if (!SESS_LEADER(u.u_procp)
				|| ((u.u_procp->p_session->s_ttyvp || tp->t_session)
						&& (tp->t_session != u.u_procp->p_session)))
			return (EPERM);
		tp->t_session = u.u_procp->p_session;
		tp->t_pgrp = u.u_procp->p_pgrp;
		u.u_procp->p_session->s_ttyp = tp;
		u.u_procp->p_flag |= P_CONTROLT;
		break;

	case FIOSETOWN: {
		pid_t pgid = *(int*) data;
		struct pgrp *pgrp;

		if (!isctty(u.u_procp, tp))
			return (ENOTTY);

		if (pgid < 0)
			pgrp = pgfind(-pgid);
		else {
			struct proc *p1 = pfind(pgid);
			if (!p1)
				return (ESRCH);
			pgrp = p1->p_pgrp;
		}

		if (pgrp == NULL)
			return (EINVAL);
		else if (pgrp->pg_session != u.u_procp->p_session)
			return (EPERM);
		tp->t_pgrp = pgrp;
		break;
	}

	case TIOCSPGRP: { 		/* set pgrp of tty */
		struct pgrp *pgrp = pgfind(*(int *)data);

		if (!isctty(u.u_procp, tp))
			return (ENOTTY);
		else if (pgrp == NULL)
			return (EINVAL);
		else if (pgrp->pg_session != u.u_procp->p_session)
			return (EPERM);
		else if (u.u_procp && u.u_procp->p_pgrp == pgrp && u.u_procp->p_uid != u.u_uid && u.u_uid && !inferior(u.u_procp))
			return (EPERM);
		else if (u.u_uid && (flag & FREAD) == 0)
			return (EPERM);
		tp->t_pgrp = pgrp;
		break;
	}

	case TIOCSTAT:			/* get load avg stats */
		s = spltty();
		TTY_LOCK(tp);
		ttyinfo(tp);
		TTY_UNLOCK(tp);
		splx(s);
		break;

	case TIOCSWINSZ: 		/* set window size */
		if (bcmp((caddr_t)&tp->t_winsize, data,
		    sizeof (struct winsize))) {
			tp->t_winsize = *(struct winsize *)data;
			gsignal(tp->t_pgrp->pg_id, SIGWINCH);
		}
		break;

	case TIOCGETC:
		bcopy((caddr_t)&tp->t_intrc, data, sizeof (struct tchars));
		break;

	case TIOCSETC:
		bcopy(data, (caddr_t)&tp->t_intrc, sizeof (struct tchars));
		break;

	/* set/get local special characters */
	case TIOCSLTC:
		bcopy(data, (caddr_t)&tp->t_suspc, sizeof (struct ltchars));
		break;

	case TIOCGLTC:
		bcopy((caddr_t)&tp->t_suspc, data, sizeof (struct ltchars));
		break;
		
	default:
		return (-1);
	}

	/*
	 * Modify local mode word.
	 */
	switch(com) {
	case TIOCLBIS:
		tp->t_flags |= (long)*(u_int *)data << 16;
		break;

	case TIOCLBIC:
		tp->t_flags &= ~((long)*(u_int *)data << 16);
		break;

	case TIOCLSET:
		tp->t_flags &= 0xffffL;
		tp->t_flags |= (long)*(u_int *)data << 16;
		break;

	case TIOCLGET:
		*(int *)data = tp->t_flags >> 16;
		break;
	}

	return (0);
}

int
ttioctl(tp, cmd, data, flag, p)
	register struct tty *tp;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	if (p == pfind(u.u_procp->p_pid)) {
		return (ttioctl_sc(tp, cmd, data, flag));
	}
	return (-1);
}

int
ttpoll(tp, events, p)
	struct tty *tp;
	int events;
	struct proc *p;
{
	int revents, s;

	revents = 0;
	s = spltty();
	TTY_LOCK(tp);
	if (events & (POLLIN | POLLRDNORM))
		if (ttnread(tp) > 0)
			revents |= events & (POLLIN | POLLRDNORM);

	if (events & (POLLOUT | POLLWRNORM))
		if (tp->t_outq.c_cc <= tp->t_lowat)
			revents |= events & (POLLOUT | POLLWRNORM);

	if (events & POLLHUP)
		if (!CONNECTED(tp))
			revents |= POLLHUP;

	if (revents == 0) {
		if (events & (POLLIN | POLLHUP | POLLRDNORM))
			selrecord(p, &tp->t_rsel);

		if (events & (POLLOUT | POLLWRNORM))
			selrecord(p, &tp->t_wsel);
	}
	TTY_LOCK(tp);
	splx(s);
	return (revents);
}

static void
filt_ttyrdetach(struct knote *kn)
{
	struct tty	*tp;
	int		s;

	tp = kn->kn_hook;
	s = spltty();
	SIMPLEQ_REMOVE(&tp->t_rsel.si_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_ttyread(struct knote *kn, long hint)
{
	struct tty	*tp;
	int		s;

	tp = kn->kn_hook;
	s = spltty();
	if ((hint & NOTE_SUBMIT) == 0)
		TTY_LOCK(tp);
	kn->kn_data = ttnread(tp);
	if ((hint & NOTE_SUBMIT) == 0)
		TTY_UNLOCK(tp);
	splx(s);
	return (kn->kn_data > 0);
}

static void
filt_ttywdetach(struct knote *kn)
{
	struct tty	*tp;
	int		s;

	tp = kn->kn_hook;
	s = spltty();
	TTY_LOCK(tp);
	SIMPLEQ_REMOVE(&tp->t_wsel.si_klist, kn, knote, kn_selnext);
	TTY_UNLOCK(tp);
	splx(s);
}

static int
filt_ttywrite(struct knote *kn, long hint)
{
	struct tty	*tp;
	int		canwrite, s;

	tp = kn->kn_hook;
	s = spltty();
	if ((hint & NOTE_SUBMIT) == 0)
		TTY_LOCK(tp);
	kn->kn_data = tp->t_outq.c_cn - tp->t_outq.c_cc;
	canwrite = (tp->t_outq.c_cc <= tp->t_lowat) && CONNECTED(tp);
	if ((hint & NOTE_SUBMIT) == 0)
		TTY_UNLOCK(tp);
	splx(s);
	return (canwrite);
}

static const struct filterops ttyread_filtops =
	{ 1, NULL, filt_ttyrdetach, filt_ttyread };
static const struct filterops ttywrite_filtops =
	{ 1, NULL, filt_ttywdetach, filt_ttywrite };

int
ttykqfilter(dev_t dev, struct knote *kn)
{
	struct tty	*tp;
	struct klist	*klist;
	int		s;
	const struct cdevsw	*cdev;

        if (((cdev = cdevsw_lookup(dev)) == NULL) ||
	    (cdev->d_tty == NULL) ||
	    ((tp = (*cdev->d_tty)(dev)) == NULL))
		return (ENXIO);

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &tp->t_rsel.si_klist;
		kn->kn_fop = &ttyread_filtops;
		break;
	case EVFILT_WRITE:
		klist = &tp->t_wsel.si_klist;
		kn->kn_fop = &ttywrite_filtops;
		break;
	default:
		return (1);
	}

	kn->kn_hook = tp;

	s = spltty();
	TTY_LOCK(tp);
	SIMPLEQ_INSERT_HEAD(klist, kn, kn_selnext);
	TTY_UNLOCK(tp);
	splx(s);

	return (0);
}

static int
ttnread(tp)
	register struct tty *tp;
{
	register int nread = 0;

	if (tp->t_flags & PENDIN)
		ttypend(tp);
	nread = tp->t_canq.c_cc;
	if (tp->t_flags & (RAW|CBREAK))
		nread += tp->t_rawq.c_cc;
	return (nread);
}

/*
 * Initial open of tty, or (re)entry to line discipline.
 * Establish a process group for distribution of
 * quits and interrupts from the tty.
 */
int
ttyopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	int s;
	
	s = spltty();
	TTY_LOCK(tp);
	tp->t_dev = dev;
	tp->t_state &= ~TS_WOPEN;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_ISOPEN;
		bzero((caddr_t) & tp->t_winsize, sizeof(tp->t_winsize));
		if (tp->t_line != NTTYDISC) {
			ttywflush(tp);
		}
	}
	TTY_UNLOCK(tp);
	splx(s);
	return (0);
}

int
ttylopen(tp, dialout, nonblock)
	struct tty *tp;
	int dialout, nonblock;
{
	int	s, error;

	error = 0;
	s = spltty();
	TTY_LOCK(tp);

	if (dialout) {
		/*
		 * If the device is already open for non-dialout, fail.
		 * Otherwise, set TS_DIALOUT to block any pending non-dialout
		 * opens.
		 */
		if (ISSET(tp->t_state, TS_ISOPEN) && !ISSET(tp->t_state, TS_DIALOUT)) {
			error = EBUSY;
			goto out;
		}
		SET(tp->t_state, TS_DIALOUT);
	} else {
		if (!nonblock) {
			/*
			 * Wait for carrier.  Also wait for any dialout
			 * processes to close the tty first.
			 */
			while (ISSET(tp->t_state, TS_DIALOUT) || !CONNECTED(tp)) {
				tp->t_wopen++;
				error = ttysleep(tp, &tp->t_rawq, TTIPRI | PCATCH, ttopen, 0);
				tp->t_wopen--;
				if (error) {
					goto out;
				}
			}
		} else {
			/*
			 * Don't allow a non-blocking non-dialout open if the
			 * device is already open for dialout.
			 */
			if (ISSET(tp->t_state, TS_DIALOUT)) {
				error = EBUSY;
				goto out;
			}
		}
	}

out:
	TTY_UNLOCK(tp);
	splx(s);
	return (error);
}

/*
 * clean tp on last close
 */
int
ttyclose(tp)
	register struct tty *tp;
{
	extern struct tty *constty; /* Temporary virtual console. */
	int s;

	s = spltty();
	TTY_LOCK(tp);

	if (constty == tp) {
		constty = NULL;
	}

	ttyflush(tp, FREAD | FWRITE);

	tp->t_gen++;
	tp->t_pgrp = NULL;
	if (tp->t_session != NULL) {
		SESSRELE(tp->t_session);
		tp->t_session = NULL;
	}
	tp->t_state = 0;

	TTY_UNLOCK(tp);
	splx(s);
	return (0);
}

/*
 * "close" a line discipline
 */
int
ttylclose(tp, flag)
	register struct tty *tp;
	int flag;
{
	int s;

	/*
	 * 4.4 has IO_NDELAY but I think that is a mistake because the upper level
	 * 'close' routines all pass 'fp->f_flags' down.  This was verified with a
	 * printf here - the F* flags are received rather than the IO_* flags!
	 */

	if (flag & (IO_NDELAY | FNONBLOCK)) {
		s = spltty();
		TTY_LOCK(tp);
		ttyflush(tp, FREAD | FWRITE);
		TTY_UNLOCK(tp);
		splx(s);
	} else {
		ttywflush(tp);
	}
	tp->t_line = 0;
	return (0);
}

/*
 * Handle modem control transition on a tty.
 * Flag indicates new state of carrier.
 * Returns 0 if the line should be turned off, otherwise 1.
 */
int
ttymodem(tp, flag)
	register struct tty *tp;
	int flag;
{
	const struct cdevsw *cdev;
	int s;
	
	cdev = cdevsw_lookup(tp->t_dev);
	if ((tp->t_state & TS_WOPEN) == 0 && (tp->t_flags & MDMBUF)) {
		/*
		 * MDMBUF: do flow control according to carrier flag
		 */
		if (flag) {
			tp->t_state &= ~TS_TTSTOP;
			ttstart(tp);
		} else if ((tp->t_state & TS_TTSTOP) == 0) {
			tp->t_state |= TS_TTSTOP;
			(*cdev->d_stop)(tp, 0);
		}
	} else if (flag == 0) {
		/*
		 * Lost carrier.
		 */

		s = spltty();
		TTY_LOCK(tp);
		tp->t_state &= ~TS_CARR_ON;
		if (tp->t_state & TS_ISOPEN) {
			if ((tp->t_flags & NOHANG) == 0) {
				if (tp->t_session && tp->t_session->s_leader) {
					psignal(tp->t_session->s_leader, SIGHUP);
				}
				gsignal(tp->t_pgrp->pg_id, SIGHUP);
				gsignal(tp->t_pgrp->pg_id, SIGCONT);
				ttyflush(tp, FREAD | FWRITE);
				TTY_UNLOCK(tp);
				splx(s);
				return (0);
			}
		}
	} else {
		/*
		 * Carrier now on.
		 */
		tp->t_state |= TS_CARR_ON;
		wakeup((caddr_t) & tp->t_rawq);
	}
	TTY_UNLOCK(tp);
	splx(s);
	return (1);
}

/*
 * Default modem control routine (for other line disciplines).
 * Return argument flag, to turn off device on carrier drop.
 */
int
nullmodem(tp, flag)
	register struct tty *tp;
	int flag;
{
	int s;

	s = spltty();
	TTY_LOCK(tp);
	if (flag)
		tp->t_state |= TS_CARR_ON;
	else
		tp->t_state &= ~TS_CARR_ON;
	TTY_UNLOCK(tp);
	splx(s);
	return (flag);
}

/*
 * reinput pending characters after state switch
 * call at spltty().
 */
void
ttypend(tp)
	register struct tty *tp;
{
	struct clist tq;
	register int c;

	tp->t_flags &= ~PENDIN;
	tp->t_state |= TS_TYPEN;
	tq = tp->t_rawq;
	tp->t_rawq.c_cc = 0;
	tp->t_rawq.c_cf = tp->t_rawq.c_cl = 0;
	while ((c = getc(&tq)) >= 0)
		ttyinput(c, tp);
	tp->t_state &= ~TS_TYPEN;
}

/*
 * Place a character on raw TTY input queue,
 * putting in delimiters and waking up top
 * half as needed.  Also echo if required.
 * The arguments are the character and the
 * appropriate tty structure.
 */
int
ttyinput_wlock(c, tp)
	register int c;
	register struct tty *tp;
{
	const struct cdevsw *cdev;
	long t_flags;
	int i;

	t_flags = tp->t_flags;
	/*
	 * If input is pending take it first.
	 */
	if (t_flags & PENDIN)
		ttypend(tp);

	c &= 0377;

	/*
	 * In tandem mode, check high water mark.
	 */
	if (t_flags & (TANDEM | RTSCTS))
		ttyblock(tp);

	if (t_flags & RAW) {
		/*
		 * Raw mode, just put character
		 * in input q w/o interpretation.
		 */
		if (tp->t_rawq.c_cc > TTYHOG)
			ttyflush(tp, FREAD | FWRITE);
		else {
			if (putc(c, &tp->t_rawq) == 0)
				ttwakeup(tp);
			ttyecho(c, tp);
		}
		goto endcase;
	}

	/*
	 * Ignore any high bit added during
	 * previous ttyinput processing.
	 */
	if ((tp->t_state & TS_TYPEN) == 0 && (t_flags & PASS8) == 0)
		c &= 0177;

	/*
	 * Check for literal nexting very first.  This is the _ONLY_ place
	 * left which ORs in 0200.  Handling literal nexting this way is
	 * what keeps the tty subsystem from being 8 bit clean.  The fix is
	 * horrendous though and is put off for now.  And to think that ALL
	 * of this is made necessary by ttyrubout() - it's the only place that
	 * actually _checks_ the 0200 bit and only for newline and tab chars
	 * at that!
	 *
	 * If we had 9 bit bytes life would be a lot simpler ;)
	 *
	 * The basic idea is to flag the character as "special" and also
	 * modify it so that the character does not match any of the special
	 * editing or control characters.  We could just as simply jump directly
	 * to the test for 'cbreak' below.
	 */
	if (tp->t_state & TS_LNCH) {
		c |= 0200;
		tp->t_state &= ~TS_LNCH;
	}

	/*
	 * Scan for special characters.  This code
	 * is really just a big case statement with
	 * non-constant cases.  The bottom of the
	 * case statement is labeled ``endcase'', so goto
	 * it after a case match, or similar.
	 */
	if (tp->t_line == NTTYDISC) {
		if (CCEQ(tp->t_lnextc, c)) {
			if (t_flags & ECHO)
				ttyout1("^\b", tp);
			tp->t_state |= TS_LNCH;
			goto endcase;
		}
		if (CCEQ(tp->t_flushc, c)) {
			if (t_flags & FLUSHO)
				tp->t_flags &= ~FLUSHO;
			else {
				ttyflush(tp, FWRITE);
				ttyecho(c, tp);
				if (tp->t_rawq.c_cc + tp->t_canq.c_cc)
					ttyretype(tp);
				tp->t_flags |= FLUSHO;
			}
			goto startoutput;
		}
		if (CCEQ(tp->t_suspc, c)) {
			if ((t_flags & NOFLSH) == 0)
				ttyflush(tp, FREAD);
			ttyecho(c, tp);
			gsignal(tp->t_pgrp->pg_id, SIGTSTP);
			goto endcase;
		}
	}

	cdev = cdevsw_lookup(tp->t_dev);

	/*
	 * Handle start/stop characters.
	 */
	if (CCEQ(tp->t_stopc, c)) {
		if ((tp->t_state & TS_TTSTOP) == 0) {
			tp->t_state |= TS_TTSTOP;
			(*cdev->d_stop)(tp, 0);
			return (0);
		}
		if (CCEQ(tp->t_startc, c))
			return (0);
		goto endcase;
	}
	if (CCEQ(tp->t_startc, c))
		goto restartoutput;

	/*
	 * Look for interrupt/quit chars.
	 */
	if (CCEQ(tp->t_intrc,c) || CCEQ(tp->t_quitc, c)) {
		if ((t_flags & NOFLSH) == 0)
			ttyflush(tp, FREAD | FWRITE);
		ttyecho(c, tp);
		gsignal(tp->t_pgrp->pg_id, CCEQ(tp->t_intrc,c) ? SIGINT : SIGQUIT);
		goto endcase;
	}

	/*
	 * Cbreak mode, don't process line editing
	 * characters; check high water mark for wakeup.
	 */
	if (t_flags & CBREAK) {
		if (tp->t_rawq.c_cc > TTYHOG) {
			if (tp->t_outq.c_cc < TTHIWAT(tp) && tp->t_line == NTTYDISC)
				(void) ttyoutput(CTRL('g'), tp);
		} else if (putc(c, &tp->t_rawq) == 0) {
			ttwakeup(tp);
			ttyecho(c, tp);
		}
		goto endcase;
	}

	/*
	 * From here on down cooked mode character
	 * processing takes place.
	 */
	if (CCEQ(tp->t_erase, c)) {
		if (tp->t_rawq.c_cc)
			ttyrub(unputc(&tp->t_rawq), tp);
		goto endcase;
	}
	if (CCEQ(tp->t_kill, c)) {
		if ((t_flags & CRTKIL) && tp->t_rawq.c_cc == tp->t_rocount) {
			while (tp->t_rawq.c_cc)
				ttyrub(unputc(&tp->t_rawq), tp);
		} else {
			ttyecho(c, tp);
			ttyecho('\n', tp);
			while (getc(&tp->t_rawq) > 0)
				;
			tp->t_rocount = 0;
		}
		tp->t_state &= ~TS_LOCAL;
		goto endcase;
	}

	/*
	 * New line discipline,
	 * check word erase/reprint line.
	 */
	if (tp->t_line == NTTYDISC) {
		if (CCEQ(tp->t_werasc, c)) {
			if (tp->t_rawq.c_cc == 0)
				goto endcase;
			do {
				c = unputc(&tp->t_rawq);
				if (c != ' ' && c != '\t')
					goto erasenb;
				ttyrub(c, tp);
			} while (tp->t_rawq.c_cc);
			goto endcase;
	erasenb:
			do {
				ttyrub(c, tp);
				if (tp->t_rawq.c_cc == 0)
					goto endcase;
				c = unputc(&tp->t_rawq);
			} while (c != ' ' && c != '\t');
			(void) putc(c, &tp->t_rawq);
			goto endcase;
		}
		if (CCEQ(tp->t_rprntc, c)) {
			ttyretype(tp);
			goto endcase;
		}
	}

	/*
	 * Check for input buffer overflow
	 */
	if (tp->t_rawq.c_cc + tp->t_canq.c_cc >= TTYHOG) {
		if (tp->t_line == NTTYDISC)
			(void) ttyoutput(CTRL('g'), tp);
		goto endcase;
	}

	/*
	 * Put data char in q for user and
	 * wakeup on seeing a line delimiter.
	 */
	if (putc(c, &tp->t_rawq) == 0) {
		if (ttbreakc(c, tp)) {
			tp->t_rocount = 0;
			catq(&tp->t_rawq, &tp->t_canq);
			ttwakeup(tp);
		} else if (tp->t_rocount++ == 0)
			tp->t_rocol = tp->t_col;
		if (tp->t_state & TS_ERASE) {
			tp->t_state &= ~TS_ERASE;
			(void) ttyoutput('/', tp);
		}
		i = tp->t_col;
		ttyecho(c, tp);
		if (CCEQ(tp->t_eofc,c) && (t_flags & ECHO)) {
			i = MIN(2, tp->t_col - i);
			while (i > 0) {
				(void) ttyoutput('\b', tp);
				i--;
			}
		}
	}
endcase:
	/*
	 * If DEC-style start/stop is enabled don't restart
	 * output until seeing the start character.
	 */
	if ((t_flags&DECCTQ) && (tp->t_state&TS_TTSTOP) &&
	    tp->t_startc != tp->t_stopc)
		return (0);
restartoutput:
	tp->t_state &= ~TS_TTSTOP;
	tp->t_flags &= ~FLUSHO;
startoutput:
	return (ttstart(tp));
}

int
ttyinput(c, tp)
	int c;
	struct tty *tp;
{
	int s, error;
	s = spltty();
	TTY_LOCK(tp);
	error = ttyinput_wlock(c, tp);
	TTY_UNLOCK(tp);
	splx(s);
	return (error);
}

/*
 * Put character on TTY output queue, adding delays,
 * expanding tabs, and handling the CR/NL bit.
 * This is called both from the top half for output,
 * and from interrupt level for echoing.
 * The arguments are the character and the tty structure.
 * Returns < 0 if putc succeeds, otherwise returns char to resend
 */
int
ttyoutput(c, tp)
	register int c;
	register struct tty *tp;
{
	register int col;

	if (tp->t_flags & (RAW|LITOUT)) {
		if (tp->t_flags&FLUSHO)
			return (-1);
		if (putc(c, &tp->t_outq))
			return(c);
		tk_nout++;
		return(-1);
	}

	c &= 0177;

	/*
	 * Ignore EOT in normal mode to avoid
	 * hanging up certain terminals.
	 */
	if (c == CEOT && (tp->t_flags & CBREAK) == 0)
		return (-1);
	/*
	 * Turn tabs to spaces as required
	 */
	if (c == '\t' && (tp->t_flags & XTABS)) {
		register int s;

		c = 8 - (tp->t_col & 7);
		if ((tp->t_flags & FLUSHO) == 0) {
			s = spltty(); /* don't interrupt tabs */
			c -= b_to_q("        ", c, &tp->t_outq);
			tk_nout += c;
			splx(s);
		}
		tp->t_col += c;
		return (c ? -1 : '\t');
	}

	/*
	 * turn <nl> to <cr><lf> if desired.
	 */
	if (c == '\n' && (tp->t_flags & CRMOD)) {
		if (putc('\r', &tp->t_outq))
			return (c);
	}
	tk_nout++;
	tp->t_outcc++;
	if ((tp->t_flags & FLUSHO) == 0 && putc(c, &tp->t_outq))
		return (c);

	col = tp->t_col;
	switch (CCLASS(c)) {

	case ORDINARY:
		col++;
		break;
	case CONTROL:
		break;
	case BACKSPACE:
		if (col)
			col--;
		break;
	case NEWLINE:
	case RETURN:
		col = 0;
		break;
	case TAB:
		col = (col | 07) + 1;
		break;
	}
	tp->t_col = col;
	return (-1);
}

/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 */
int
ttread(tp, uio, flag)
	register struct tty *tp;
	struct uio *uio;
	int flag;
{
	register struct clist *qp;
	register int c;
	long t_flags;
	int s, first, error = 0, carrier;

loop:

	/*
	 * Take any pending input first.
	 */
	s = spltty();
	TTY_LOCK(tp);
	if (tp->t_flags & PENDIN)
		ttypend(tp);

	/*
	 * Hang process if it's in the background.
	 */
	if (tp == u.u_ttyp && u.u_procp->p_pgrp != tp->t_pgrp) {
		if ((u.u_procp->p_sigignore & sigmask(SIGTTIN))
				|| (u.u_procp->p_sigmask & sigmask(SIGTTIN))
				|| (u.u_procp->p_flag & P_SVFORK))
			TTY_UNLOCK(tp);
			splx(s);
			return (EIO);
		gsignal(u.u_procp->p_pgrp->pg_id, SIGTTIN);
		error = ttysleep(tp, &lbolt, TTIPRI | PCATCH, ttybg, 0);
		splx(s);
		if (error)
			return (error);
		goto loop;
	}
	t_flags = tp->t_flags;

	/*
	 * In raw mode take characters directly from the
	 * raw queue w/o processing.  Interlock against
	 * device interrupts when interrogating rawq.
	 */
	if (t_flags & RAW) {
		s = spltty();
		TTY_LOCK(tp);
		if (tp->t_rawq.c_cc <= 0) {
			carrier = ISSET(tp->t_state, TS_CARR_ON);
			if (!carrier && ISSET(tp->t_state, TS_ISOPEN)) {
				TTY_UNLOCK(tp);
				splx(s);
				return (0); /* EOF */
			}
			if (flag & IO_NDELAY) {
				TTY_UNLOCK(tp);
				splx(s);
				return (EWOULDBLOCK);
			}
			sleep((caddr_t) & tp->t_rawq, TTIPRI);
			TTY_UNLOCK(tp);
			splx(s);
			goto loop;
		}
		TTY_UNLOCK(tp);
		splx(s);
		while (!error && tp->t_rawq.c_cc && uio->uio_resid)
			error = ureadc(getc(&tp->t_rawq), uio);
		goto checktandem;
	}

	/*
	 * In cbreak mode use the rawq, otherwise
	 * take characters from the canonicalized q.
	 */
	qp = t_flags & CBREAK ? &tp->t_rawq : &tp->t_canq;

	/*
	 * No input, sleep on rawq awaiting hardware
	 * receipt and notification.
	 */
	s = spltty();
	TTY_LOCK(tp);
	if (qp->c_cc <= 0) {
		carrier = ISSET(tp->t_state, TS_CARR_ON);
		if (!carrier && ISSET(tp->t_state, TS_ISOPEN)) {
			TTY_UNLOCK(tp);
			splx(s);
			return (0); /* EOF */
		}
		if (flag & IO_NDELAY) {
			TTY_UNLOCK(tp);
			splx(s);
			return (EWOULDBLOCK);
		}
		sleep((caddr_t) & tp->t_rawq, TTIPRI);
		TTY_UNLOCK(tp);
		splx(s);
		goto loop;
	}
	TTY_UNLOCK(tp);
	splx(s);

	/*
	 * Input present, perform input mapping
	 * and processing (we're not in raw mode).
	 */
	first = 1;
	while ((c = getc(qp)) >= 0) {
		if ((t_flags & CRMOD) && c == '\r')
			c = '\n';
		/*
		 * Check for delayed suspend character.
		 */
		if (tp->t_line == NTTYDISC && CCEQ(tp->t_dsuspc, c)) {
			gsignal(tp->t_pgrp->pg_id, SIGTSTP);
			if (first) {
				s = spltty();
				TTY_LOCK(tp);
				error = ttysleep(tp, &lbolt, TTIPRI | PCATCH, ttybg, 0);
				splx(s);
				if (error)
					break;
				goto loop;
			}
			break;
		}
		/*
		 * Interpret EOF only in cooked mode.
		 */
		if (CCEQ(tp->t_eofc,c) && (t_flags & CBREAK) == 0)
			break;
		/*
		 * Give user character.
		 */
		error = ureadc(t_flags & PASS8 ? c : c & 0177, uio);
		if (error)
			break;
		if (uio->uio_resid == 0)
			break;
		/*
		 * In cooked mode check for a "break character"
		 * marking the end of a "line of input".
		 */
		if ((t_flags & CBREAK) == 0 && ttbreakc(c, tp))
			break;
		first = 0;
	}

checktandem:
	/*
	 * Look to unblock output now that (presumably)
	 * the input queue has gone down.
	 */
	s = spltty();
	TTY_LOCK(tp);
	if (ISSET(tp->t_state,TS_TBLOCK) && tp->t_rawq.c_cc < TTYUNBLOCK)
		ttyunblock(tp);
	TTY_UNLOCK(tp);
	splx(s);
	return (error);
}

/*
 * Check the output queue on tp for space for a kernel message
 * (from uprintf/tprintf).  Allow some space over the normal
 * hiwater mark so we don't lose messages due to normal flow
 * control, but don't let the tty run amok.
 * Sleeps here are not interruptible, but we return prematurely
 * if new signals come in.
 */
 
int
ttycheckoutq_wlock(tp, wait)
	register struct tty *tp;
	int wait;
{
	int hiwat, s, error;

	hiwat = TTHIWAT(tp);
	s = spltty();
	if (tp->t_outq.c_cc > hiwat + 200)
		while (tp->t_outq.c_cc > hiwat) {
			ttstart(tp);
			if (wait == 0) {
    			splx(s);
				return (0);
			}
			tp->t_state |= TS_ASLEEP;
			error = ttysleep(tp, &tp->t_outq, (PZERO - 1) | PCATCH, "ttckoutq", hz);
			if (error == EINTR) {
			    wait = 0;
			}
		}
	splx(s);
	return (1);
}

int
ttycheckoutq(tp, wait)
	register struct tty *tp;
	int wait;
{
	int	r, s;

	s = spltty();
	TTY_LOCK(tp);
	r = ttycheckoutq_wlock(tp, wait);
	TTY_UNLOCK(tp);
	splx(s);
	return (r);
}

/*
 * Called from the device's write routine after it has
 * calculated the tty-structure given as argument.
 */
int
ttwrite(tp, uio, flag)
	register struct tty *tp;
	register struct uio *uio;
	int flag;
{
	char *cp;
	register int cc, ce, c;
	int i, hiwat, cnt, error, s;
	char obuf[OBUFSIZ];

	hiwat = TTHIWAT(tp);
	cnt = uio->uio_resid;
	error = 0;
	cc = 0;
loop:
	s = spltty();
	TTY_LOCK(tp);
	if (!(tp->t_state&TS_CARR_ON)) {
		if (tp->t_state & TS_ISOPEN) {
			TTY_UNLOCK(tp);
			splx(s);
			return(EIO);
		} else if (flag & IO_NDELAY) {
			TTY_UNLOCK(tp);
			splx(s);
			error = EWOULDBLOCK;
			goto out;
		} else {
			/* Sleep awaiting carrier. */
			error = ttysleep(tp, &tp->t_rawq, TTIPRI | PCATCH, ttopen, 0);
			if (error)
				goto out;
			goto loop;
		}
	}
	TTY_UNLOCK(tp);
	splx(s);
	/*
	 * Hang the process if it's in the background.
	 */
	if (u.u_procp->p_pgrp != tp->t_pgrp && tp == u.u_ttyp
			&& (tp->t_flags & TOSTOP) && (u.u_procp->p_flag & P_SVFORK) == 0
			&& !(u.u_procp->p_sigignore & sigmask(SIGTTOU))
			&& !(u.u_procp->p_sigmask & sigmask(SIGTTOU))) {
		gsignal(u.u_procp->p_pgrp->pg_id, SIGTTOU);
		s = spltty();
		TTY_LOCK(tp);
		error = ttysleep(tp, &lbolt, TTIPRI | PCATCH, ttybg, 0);
		splx(s);
		if (error)
			goto out;
		goto loop;
	}

	/*
	 * Process the user's data in at most OBUFSIZ
	 * chunks.  Perform lower case simulation and
	 * similar hacks.  Keep track of high water
	 * mark, sleep on overflow awaiting device aid
	 * in acquiring new space.
	 */
	while (uio->uio_resid || cc > 0) {
		if (tp->t_flags&FLUSHO) {
			TTY_UNLOCK(tp);
			uio->uio_resid = 0;
			return(0);
		}
		if (tp->t_outq.c_cc > hiwat)
			goto ovhiwat;
		/*
		 * Grab a hunk of data from the user, unless we have some
		 * leftover from last time.
		*/
		if (cc == 0) {
			cc = MIN(uio->uio_resid, OBUFSIZ);
			cp = obuf;
			error = uiomove(cp, cc, uio);
			if (error) {
				cc = 0;
				break;
			}
		}
		/*
		 * If nothing fancy need be done, grab those characters we
		 * can handle without any of ttyoutput's processing and
		 * just transfer them to the output q.  For those chars
		 * which require special processing (as indicated by the
		 * bits in partab), call ttyoutput.  After processing
		 * a hunk of data, look for FLUSHO so ^O's will take effect
		 * immediately.
		 */
		s = spltty();
		TTY_LOCK(tp);
		while (cc > 0) {
			if (tp->t_flags & (RAW|LITOUT))
				ce = cc;
			else {
				ce = cc - scanc((u_int)cc, (u_char *)cp, (u_char *)char_type, CCLASSMASK);
				/*
				 * If ce is zero, then we're processing
				 * a special character through ttyoutput.
				 */
				if (ce == 0) {
					tp->t_rocount = 0;
					if (ttyoutput(*cp, tp) >= 0) {
					    /* no c-lists, wait a bit */
					    ttstart(tp);
					    sleep((caddr_t)&lbolt, TTOPRI);
						TTY_UNLOCK(tp);
						splx(s);
					    goto loop;
					}
					cp++, cc--;
					if ((tp->t_flags & FLUSHO) || tp->t_outq.c_cc > hiwat)
						TTY_UNLOCK(tp);
						splx(s);
						goto ovhiwat;
					continue;
				}
			}
			/*
			 * A bunch of normal characters have been found,
			 * transfer them en masse to the output queue and
			 * continue processing at the top of the loop.
			 * If there are any further characters in this
			 * <= OBUFSIZ chunk, the first should be a character
			 * requiring special handling by ttyoutput.
			 */
			tp->t_rocount = 0;
			i = b_to_q(cp, ce, &tp->t_outq);
			ce -= i;
			tp->t_col += ce;
			cp += ce, cc -= ce;
			tp->t_outcc += ce;
			if (i > 0) {
				/* out of c-lists, wait a bit */
				ttstart(tp);
				sleep((caddr_t)&lbolt, TTOPRI);
				TTY_UNLOCK(tp);
				splx(s);
				goto loop;
			}
			if ((tp->t_flags&FLUSHO) || tp->t_outq.c_cc > hiwat)
				break;
		}	/* while (cc > 0) */
		TTY_UNLOCK(tp);
		splx(s);
		ttstart(tp);
	}	/* while (uio->uio_resid || cc > 0) */
out:
	/*
	 * If cc is nonzero, we leave the uio structure inconsistent, as the
	 * offset and iov pointers have moved forward, but it doesn't matter
	 * (the call will either return short or restart with a new uio).
	*/
	uio->uio_resid += cc;
	return (error);

ovhiwat:
	s = spltty();
	TTY_LOCK(tp);
	/*
	 * This can only occur if FLUSHO is also set in t_flags,
	 * or if ttstart/oproc is synchronous (or very fast).
	 */
	if (tp->t_outq.c_cc <= hiwat) {
		TTY_UNLOCK(tp);
		splx(s);
		goto loop;
	}
	ttstart(tp);
	if (flag & IO_NDELAY) {
		TTY_UNLOCK(tp);
		splx(s);
		uio->uio_resid += cc;
		return (uio->uio_resid == cnt ? EWOULDBLOCK : 0);
	}
	tp->t_state |= TS_ASLEEP;
	error = ttysleep(tp, &tp->t_outq, TTOPRI | PCATCH, ttyout, 0);
	splx(s);
	if (error)
		goto out;
	goto loop;
}

/*
 * Rubout one character from the rawq of tp
 * as cleanly as possible.
 */
void
ttyrub(c, tp)
	register int c;
	register struct tty *tp;
{
	register char *cp;
	int savecol;
	int s;

	if ((tp->t_flags & ECHO) == 0)
		return;
	tp->t_flags &= ~FLUSHO;
	c &= 0377;
	if (tp->t_flags & CRTBS) {
		if (tp->t_rocount == 0) {
			/*
			 * Screwed by ttwrite; retype
			 */
			ttyretype(tp);
			return;
		}
		/*
		 * Out of the ENTIRE tty subsystem would believe this is the ONLY place
		 * that the "9th" bit (quoted chars) is tested?
		 */
		if (c == ('\t' | TTY_QUOTE) || c == ('\n' | TTY_QUOTE))
			ttyrubo(tp, 2);
		else {
			CLR(c, ~TTY_CHARMASK);
			switch (CCLASS(c)) {
			case ORDINARY:
				ttyrubo(tp, 1);
				break;
			case BACKSPACE:
			case CONTROL:
			case NEWLINE:
			case RETURN:
			case VTAB:
				if (tp->t_flags & CTLECH)
					ttyrubo(tp, 2);
				break;
			case TAB:
				if (tp->t_rocount < tp->t_rawq.c_cc) {
					ttyretype(tp);
					return;
				}
				s = spltty();
				savecol = tp->t_col;
				tp->t_state |= TS_CNTTB;
				tp->t_flags |= FLUSHO;
				tp->t_col = tp->t_rocol;
				cp = tp->t_rawq.c_cf;
				for (; cp; cp = nextc(&tp->t_rawq, cp))
					ttyecho(*cp, tp);
				tp->t_flags &= ~FLUSHO;
				tp->t_state &= ~TS_CNTTB;
				splx(s);
				/* savecol will now be length of the tab */
				savecol -= tp->t_col;
				tp->t_col += savecol;
				if (savecol > 8)
					savecol = 8; /* overflow screw */
				while (--savecol >= 0)
					(void) ttyoutput('\b', tp);
				break;
			default:
				panic("ttyrub");
			}
		}
	} else if (tp->t_flags & PRTERA) {
		if ((tp->t_state & TS_ERASE) == 0) {
			(void) ttyoutput('\\', tp);
			tp->t_state |= TS_ERASE;
		}
		ttyecho(c, tp);
	} else
		ttyecho(tp->t_erase, tp);
	tp->t_rocount--;
}

/*
 * Crt back over cnt chars perhaps
 * erasing them.
 */
static void
ttyrubo(tp, cnt)
	register struct tty *tp;
	register int cnt;
{
	register char *rubostring = tp->t_flags & CRTERA ? "\b \b" : "\b";

	while (--cnt >= 0)
		ttyout1(rubostring, tp);
}

/*
 * Reprint the rawq line.
 * We assume c_cc has already been checked.
 */
void
ttyretype(tp)
	register struct tty *tp;
{
	register char *cp;
	int s;

	if (tp->t_rprntc != _POSIX_VDISABLE)
		ttyecho(tp->t_rprntc, tp);
	(void) ttyoutput('\n', tp);
	s = spltty();
	for (cp = tp->t_canq.c_cf; cp; cp = nextc(&tp->t_canq, cp))
		ttyecho(*cp, tp);
	for (cp = tp->t_rawq.c_cf; cp; cp = nextc(&tp->t_rawq, cp))
		ttyecho(*cp, tp);
	tp->t_state &= ~TS_ERASE;
	splx(s);
	tp->t_rocount = tp->t_rawq.c_cc;
	tp->t_rocol = 0;
}

/*
 * Echo a typed character to the terminal
 */
static void
ttyecho(c, tp)
	register int c;
	register struct tty *tp;
{
	register int c7;

	if ((tp->t_state & TS_CNTTB) == 0)
		tp->t_flags &= ~FLUSHO;
	if ((tp->t_flags & ECHO) == 0)
		return;
	c &= 0377;

	if (tp->t_flags & RAW) {
		(void) ttyoutput(c, tp);
		return;
	}
	if (c == '\r' && (tp->t_flags & CRMOD))
		c = '\n';
	c7 = c & 0177;
	if (tp->t_flags & CTLECH) {
		if ((c7 <= 037 && c != '\t' && c != '\n') || c7 == 0177) {
			(void) ttyoutput('^', tp);
			if (c7 == 0177)
				c7 = '?';
			else
				c7 += 'A' - 1;
		}
	}
	(void) ttyoutput(c7, tp);
}

/*
 * Is c a break char for tp?
 */
int
ttbreakc(c, tp)
	register int c;
	register struct tty *tp;
{
	return (c == '\n' || CCEQ(tp->t_eofc, c) || CCEQ(tp->t_brkc, c)
			|| (c == '\r' && (tp->t_flags & CRMOD)));
}

/*
 * send string cp to tp
 */
void
ttyout1(cp, tp)
	register char *cp;
	register struct tty *tp;
{
	register int c;

	while (c == *cp++)
		(void) ttyoutput(c, tp);
}

/*
 * Wake up any readers on a tty.
 */
void
ttwakeup(tp)
	register struct tty *tp;
{

	if (&tp->t_rsel != NULL) {
		selnotify(&tp->t_rsel, NOTE_SUBMIT);
		tp->t_state &= ~TS_RCOLL;
	}
	if (tp->t_state & TS_ASYNC)
		pgsignal(tp->t_pgrp, SIGIO, 1);
	wakeup((caddr_t)&tp->t_rawq);
}

/*
 * Look up a code for a specified speed in a conversion table;
 * used by drivers to map software speed values to hardware parameters.
 */
int
ttspeedtab(speed, table)
	int speed;
	register struct speedtab *table;
{

	for (; table->sp_speed != -1; table++)
		if (table->sp_speed == speed)
			return (table->sp_code);
	return (-1);
}

/*
 * Set tty hi and low water marks.
 *
 * Try to arrange the dynamics so there's about one second
 * from hi to low water.
 *
 */
void
ttsetwater(tp)
	struct tty *tp;
{
	register int cps, x;

#define CLAMP(x, h, l)	((x) > h ? h : ((x) < l) ? l : (x))

	cps = tp->t_ospeed / 10;
	tp->t_lowat = x = CLAMP(cps / 2, TTMAXLOWAT, TTMINLOWAT);
	x += cps;
	x = CLAMP(x, TTMAXHIWAT, TTMINHIWAT);
	tp->t_hiwat = roundup(x, CBSIZE);
#undef	CLAMP
}

/*
 * Report on state of foreground process group.
 */
void
ttyinfo(tp)
	register struct tty *tp;
{
	register struct proc *p, *pick;
	struct timeval utime, stime;
	int tmp;

	if (ttycheckoutq(tp, 0) == 0)
		return;

	/* Print load average. */
	tmp = (averunnable.ldavg[0] * 100 + FSCALE / 2) >> FSHIFT;
	ttyprintf(tp, "load: %d.%02d ", tmp / 100, tmp % 100);

	if (tp->t_session == NULL)
		ttyprintf(tp, "not a controlling terminal\n");
	else if (tp->t_pgrp == NULL)
		ttyprintf(tp, "no foreground process group\n");
	else if ((p = LIST_FIRST(&tp->t_pgrp->pg_mem)) == NULL)
		ttyprintf(tp, "empty foreground process group\n");
	else {
		/* Pick interesting process. */
		for (pick = NULL; p != NULL; p = LIST_NEXT(p, p_pglist))
			if (proc_compare(pick, p))
				pick = p;

		ttyprintf(tp, " cmd: %s %d [%s] ", pick->p_comm, pick->p_pid,
				pick->p_stat == SRUN ? "running" :
				pick->p_wmesg ? pick->p_wmesg : "iowait");

		calcru(pick, &utime, &stime, NULL);

		/* Print user time. */
		ttyprintf(tp, "%d.%02du ", utime.tv_sec, utime.tv_usec / 10000);

		/* Print system time. */
		ttyprintf(tp, "%d.%02ds ", stime.tv_sec, stime.tv_usec / 10000);

#define	pgtok(a)	(((a) * NBPG) / 1024)
		/* Print percentage cpu, resident set size. */
		tmp = (pick->p_pctcpu * 10000 + FSCALE / 2) >> FSHIFT;
		ttyprintf(tp, "%d%% %dk\n", tmp / 100,
				pick->p_stat == SIDL || pick->p_stat == SZOMB ?
						0 :
#ifdef pmap_resident_count
						pgtok(pmap_resident_count(&pick->p_vmspace->vm_pmap))
#else
						pgtok(pick->p_vmspace->vm_rssize)
#endif
								);
	}
	tp->t_rocount = 0; /* so pending input will be retyped if BS */
}


/*
 * Output char to tty; console putchar style.
 */
int
tputchar(c, tp)
	int c;
	struct tty *tp;
{
	register int s;

	s = spltty();
	if (!ISSET(tp->t_state, TS_CARR_ON | TS_ISOPEN) != (TS_CARR_ON | TS_ISOPEN)) {
		splx(s);
		return (-1);
	}
	if (c == '\n')
		(void)ttyoutput('\r', tp);
	(void)ttyoutput(c, tp);
	ttstart(tp);
	splx(s);
	return (0);
}

int
ttysleep(tp, chan, pri, wmesg, timo)
	struct tty *tp;
	void *chan;
	int pri, timo;
	char *wmesg;
{
	int error;
	int gen;

	gen = tp->t_gen;
	error = tsleep(chan, pri, wmesg, timo);
	if (error)
		return (error);
	return (tp->t_gen == gen ? 0 : ERESTART);
}

/*
 * Allocate a tty struct.  Clists in the struct will be allocated by
 * ttyopen().
 */
struct tty *
ttymalloc(void)
{
	struct tty *tp;
	tp = malloc(sizeof(*tp), M_TTY, M_WAITOK);
	bzero(tp, sizeof(*tp));
	tty_lock_init(tp);
	callout_init(&tp->t_rstrt_ch);
	/* XXX: default to 1024 chars for now */
	clist_alloc_cblocks(&tp->t_rawq, 1024, 1);
	clist_alloc_cblocks(&tp->t_canq, 1024, 1);
	/* output queue doesn't need quoting */
	clist_alloc_cblocks(&tp->t_outq, 1024, 1);

	tty_init_termios(tp);

	return (tp);
}

/*
 * Free a tty struct.  Clists in the struct should have been freed by
 * ttyclose().
 */
void
ttyfree(tp)
	struct tty *tp;
{
	callout_stop(&tp->t_rstrt_ch);
	clist_free_cblocks(&tp->t_rawq);
	clist_free_cblocks(&tp->t_canq);
	clist_free_cblocks(&tp->t_outq);
	free(tp, M_TTY);
}

static void
tty_init_termios(tp)
	struct tty *tp;
{
	struct termios *t = &tp->t_termios;

	t->c_cflag = TTYDEF_CFLAG;
	t->c_iflag = TTYDEF_IFLAG;
	t->c_lflag = TTYDEF_LFLAG;
	t->c_oflag = TTYDEF_OFLAG;
	t->c_ispeed = TTYDEF_SPEED;
	t->c_ospeed = TTYDEF_SPEED;
	bcopy(ttydefchars, &t->c_cc, sizeof(ttydefchars));
	tp->t_termios = *t;
}

void
tty_init_console(tp, s)
	struct tty *tp;
	speed_t 	s;
{
	struct termios *ti = &tp->t_termios;
	struct termios *to = &tp->t_termios;

	if (s != 0) {
		ti->c_ispeed = ti->c_ospeed = s;
		to->c_ispeed = to->c_ospeed = s;
	}

	ti->c_cflag |= CLOCAL;
	to->c_cflag |= CLOCAL;
}

/*
 * Returns 1 if p2 is "better" than p1
 *
 * The algorithm for picking the "interesting" process is thus:
 *
 *	1) Only foreground processes are eligible - implied.
 *	2) Runnable processes are favored over anything else.  The runner
 *	   with the highest cpu utilization is picked (p_estcpu).  Ties are
 *	   broken by picking the highest pid.
 *	3) The sleeper with the shortest sleep time is next.  With ties,
 *	   we pick out just "short-term" sleepers (P_SINTR == 0).
 *	4) Further ties are broken by picking the highest pid.
 */
#define ISRUN(p)		(((p)->p_stat == SRUN) || ((p)->p_stat == SIDL))
#define TESTAB(a, b)    ((a)<<1 | (b))
#define ONLYA   		2
#define ONLYB   		1
#define BOTH    		3

static int
proc_compare(p1, p2)
	register struct proc *p1, *p2;
{

	if (p1 == NULL)
		return (1);
	/*
	 * see if at least one of them is runnable
	 */
	switch (TESTAB(ISRUN(p1), ISRUN(p2))) {
	case ONLYA:
		return (0);
	case ONLYB:
		return (1);
	case BOTH:
		/*
		 * tie - favor one with highest recent cpu utilization
		 */
		if (p2->p_estcpu > p1->p_estcpu)
			return (1);
		if (p1->p_estcpu > p2->p_estcpu)
			return (0);
		return (p2->p_pid > p1->p_pid);	/* tie - return highest pid */
	}
	/*
 	 * weed out zombies
	 */
	switch (TESTAB(p1->p_stat == SZOMB, p2->p_stat == SZOMB)) {
	case ONLYA:
		return (1);
	case ONLYB:
		return (0);
	case BOTH:
		return (p2->p_pid > p1->p_pid); /* tie - return highest pid */
	}
	/*
	 * pick the one with the smallest sleep time
	 */
	if (p2->p_slptime > p1->p_slptime)
		return (0);
	if (p1->p_slptime > p2->p_slptime)
		return (1);
	/*
	 * favor one sleeping in a non-interruptible sleep
	 */
	if ((p1->p_flag & P_SINTR) && (p2->p_flag & P_SINTR) == 0)
		return (1);
	if ((p2->p_flag & P_SINTR) && (p1->p_flag & P_SINTR) == 0)
		return (0);
	return (p2->p_pid > p1->p_pid);		/* tie - return highest pid */
}
