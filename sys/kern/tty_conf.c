/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_conf.c	1.2 (2.11BSD GTE) 11/30/94
 */

#include <sys/param.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/user.h>

int	nodev();
int	nulldev();

int	ttyopen(),ttylclose(),ttread(),ttwrite(),nullioctl(),ttstart();
int	ttymodem(), nullmodem(), ttyinput();

/* #include "bk.h" */
#if NBK > 0
int	bkopen(),bkclose(),bkread(),bkinput(),bkioctl();
#endif

/* #include "tb.h" */
#if NTB > 0
int	tbopen(),tbclose(),tbread(),tbinput(),tbioctl();
#endif
/* #include "sl.h" */
#if NSL > 0
int	slopen(),slclose(),slinput(),slioctl(),slstart();
#endif

/* 0- TTYDISC (Termios) */
struct linesw ttydisc = {
	.l_open = ttyopen,
	.l_close = ttylclose,
	.l_read = ttread,
	.l_write = ttwrite,
	.l_ioctl = nullioctl,
	.l_rint = ttyinput,
	.l_rend = nodev,
	.l_meta = nulldev,
	.l_start = ttstart,
	.l_modem = ttymodem,
	.l_poll = nopoll
};

/* 1- NTTYDISC */
struct linesw nttydisc = {
	.l_open = ttyopen,
	.l_close = ttylclose,
	.l_read = ttread,
	.l_write = ttwrite,
	.l_ioctl = nullioctl,
	.l_rint = ttyinput,
	.l_rend = nodev,
	.l_meta = nulldev,
	.l_start = ttstart,
	.l_modem = ttymodem,
	.l_poll = nopoll
};

/* 2- OTTYDISC */
struct linesw ottydisc = {
	.l_open = ttyopen,
	.l_close = ttylclose,
	.l_read = ttread,
	.l_write = ttwrite,
	.l_ioctl = nullioctl,
	.l_rint = ttyinput,
	.l_rend = nodev,
	.l_meta = nulldev,
	.l_start = ttstart,
	.l_modem = ttymodem,
	.l_poll = nopoll
};

/* 3- NETLDISC */
struct linesw netldisc = {
	.l_open = bkopen,
	.l_close = bkclose,
	.l_read = bkread,
	.l_write = ttwrite,
	.l_ioctl = bkioctl,
	.l_rint = bkinput,
	.l_rend = nodev,
	.l_meta = nulldev,
	.l_start = ttstart,
	.l_modem = nullmodem,
	.l_poll = nopoll
};

/* 4- TABLDISC */
struct linesw tabldisc = {
	.l_open = tbopen,
	.l_close = tbclose,
	.l_read = tbread,
	.l_write = nodev,
	.l_ioctl = tbioctl,
	.l_rint = tbinput,
	.l_rend = nodev,
	.l_meta = nulldev,
	.l_start = ttstart,
	.l_modem = nullmodem,
	.l_poll = nopoll
};

/* 5- SLIPDISC */
struct linesw slipdisc = {
	.l_open = slopen,
	.l_close = slclose,
	.l_read = noread,
	.l_write = nowrite,
	.l_ioctl = slioctl,
	.l_rint = slinput,
	.l_rend = nodev,
	.l_meta = nodev,
	.l_start = slstart,
	.l_modem = nullmodem,
	.l_poll = nopoll
};

/* 6- PPPDISC */
struct linesw pppdisc = {
	.l_open = noopen,
	.l_close = noclose,
	.l_read = noread,
	.l_write = nowrite,
	.l_ioctl = noioctl,
	.l_rint = norint,
	.l_rend = nodev,
	.l_meta = nodev,
	.l_start = nostart,
	.l_modem = nomodem,
	.l_poll = nopoll
};

/* initialize tty conf structures */
void
tty_conf_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &ttydisc);	/* 0- TTYDISC */
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &nttydisc);	/* 1- NTTYDISC */
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &ottydisc);	/* 2- OTTYDISC */
#if NBK > 0
	DEVSWIO_CONFIG_INIT(devsw, NBK, NULL, NULL, &netldisc);	/* 3- NETLDISC */
#endif
#if NTB > 0
	DEVSWIO_CONFIG_INIT(devsw, NTB, NULL, NULL, &tabldisc);	/* 4- TABLDISC */
#endif
#if NSL > 0
	DEVSWIO_CONFIG_INIT(devsw, NSL, NULL, NULL, &slipdisc);	/* 5- SLIPDISC */
#endif
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &pppdisc);	/* 6- PPPDISC */
}

/*
 * Do nothing specific version of line
 * discipline specific ioctl command.
 */
/*ARGSUSED*/
int
nullioctl(tp, cmd, data, flags)
	struct tty *tp;
	u_int cmd;
	char *data;
	int flags;
{
#ifdef lint
	tp = tp; data = data; flags = flags;
#endif
	return (-1);
}
