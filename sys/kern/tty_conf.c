/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_conf.c	1.2 (2.11BSD GTE) 11/30/94
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/user.h>

//#include "tb.h"
#if NTB > 0
int	tbopen(dev_t, struct tty *);
int	tbclose(struct tty *, int);
int	tbread(struct tty *, struct uio *, int);
int	tbtioctl(struct tty *, u_long, caddr_t, int, struct proc *);
int	tbinput(int, struct tty *);
#endif

//#include "sl.h"
#if NSL > 0
int	slopen(dev_t, struct tty *);
int	slclose(struct tty *, int);
int	sltioctl(struct tty *, u_long, caddr_t, int, struct proc *);
int	slinput(int, struct tty *);
int	slstart(struct tty *);
#endif

//#include "ppp.h"
#if NPPP > 0
int	pppopen(dev_t, struct tty *);
int	pppclose(struct tty *, int);
int	ppptioctl(struct tty *, u_long, caddr_t, int, struct proc *);
int	pppinput(int, struct tty *);
int	pppstart(struct tty *);
int	pppread(struct tty *, struct uio *, int);
int	pppwrite(struct tty *, struct uio *, int);
#endif

//#include "strip.h"
#if NSTRIP > 0
int	stripopen(dev_t, struct tty *);
int	stripclose(struct tty *, int);
int	striptioctl(struct tty *, u_long, caddr_t, int, struct proc *);
int	stripinput(int, struct tty *);
int	stripstart(struct tty *);
#endif

/* 0- TTYDISC (Termios) */
const struct linesw ttydisc = {
	.l_open = ttyopen,
	.l_close = ttylclose,
	.l_read = ttread,
	.l_write = ttwrite,
	.l_ioctl = nullttyioctl,
	.l_rint = ttyinput,
	.l_start = ttstart,
	.l_modem = ttymodem,
	.l_poll = ttpoll
};

/* 1- NTTYDISC */
const struct linesw nttydisc = {
	.l_open = ttyopen,
	.l_close = ttylclose,
	.l_read = ttread,
	.l_write = ttwrite,
	.l_ioctl = nullttyioctl,
	.l_rint = ttyinput,
	.l_rend = norend,
	.l_meta = nullmeta,
	.l_start = ttstart,
	.l_modem = ttymodem,
	.l_poll = ttpoll
};

/* 2- OTTYDISC */
const struct linesw ottydisc = {
	.l_open = ttyopen,
	.l_close = ttylclose,
	.l_read = ttread,
	.l_write = ttwrite,
	.l_ioctl = nullttyioctl,
	.l_rint = ttyinput,
	.l_rend = norend,
	.l_meta = nullmeta,
	.l_start = ttstart,
	.l_modem = ttymodem,
	.l_poll = ttpoll
};

#if NBK > 0
/* 3- NETLDISC */
const struct linesw netldisc = {
	.l_open = bkopen,
	.l_close = bkclose,
	.l_read = bkread,
	.l_write = ttwrite,
	.l_ioctl = bkioctl,
	.l_rint = bkinput,
	.l_rend = norend,
	.l_meta = nullmeta,
	.l_start = ttstart,
	.l_modem = nullmodem,
	.l_poll = ttpoll
};
#endif

#if NTB > 0
/* 4- TABLDISC */
const struct linesw tabldisc = {
	.l_open = tbopen,
	.l_close = tbclose,
	.l_read = tbread,
	.l_write = nottywrite,
	.l_ioctl = tbioctl,
	.l_rint = tbinput,
	.l_rend = norend,
	.l_meta = nullmeta,
	.l_start = ttstart,
	.l_modem = nullmodem,
	.l_poll = nottypoll
};
#endif

#if NSL > 0
/* 5- SLIPDISC */
const struct linesw slipdisc = {
	.l_open = slopen,
	.l_close = slclose,
	.l_read = nottyread,
	.l_write = nottywrite,
	.l_ioctl = slioctl,
	.l_rint = slinput,
	.l_rend = norend,
	.l_meta = nullmeta,
	.l_start = slstart,
	.l_modem = nomodem,
	.l_poll = nottypoll
};
#endif

#if NSTRIP > 0
/* 6- STRIPDISC */
const struct linesw strip_disc = {
	.l_open = stripopen,
	.l_close = stripclose,
	.l_read = nottyread,
	.l_write = nottywrite,
	.l_ioctl = striptioctl,
	.l_rint = stripinput,
	.l_rend = norend,
	.l_meta = nometa,
	.l_start = stripstart,
	.l_modem = nullmodem,
	.l_poll = nottypoll
};
#endif

#if NPPP > 0
/* 7- PPPDISC */
const struct linesw pppdisc = {
	.l_open = pppopen,
	.l_close = pppclose,
	.l_read = pppread,
	.l_write = pppwrite,
	.l_ioctl = ppptioctl,
	.l_rint = pppinput,
	.l_rend = norend,
	.l_meta = nometa,
	.l_start = pppstart,
	.l_modem = ttymodem,
	.l_poll = ttpoll
};
#endif
/*
 * Do nothing specific version of line
 * discipline specific ioctl command.
 */
/*ARGSUSED*/
/*
int
nullttyioctl(tp, cmd, data, flags, p)
	struct tty *tp;
	u_long cmd;
	char *data;
	int flags;
	struct proc *p;
{
#ifdef lint
	tp = tp; data = data; flags = flags;
#endif
	return (-1);
}
*/
