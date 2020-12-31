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
int	SLOPEN(),SLCLOSE(),SLINPUT(),SLTIOCTL(),SLSTART();
#endif

/* 0- OTTYDISC */
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
	.l_poll = nodev			/* add poll */
};

/* 2- NTTYDISC */
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
	.l_poll = nodev			/* add poll */
};

/* 1- NETLDISC */
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
	.l_poll = nodev			/* add poll */
};

/* 3- TABLDISC */
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
	.l_poll = nodev			/* add poll */
};

/* 4- SLIPDISC */
struct linesw slipdisc = {
	.l_open = SLOPEN,
	.l_close = SLCLOSE,
	.l_read = nodev,
	.l_write = nodev,
	.l_ioctl = SLTIOCTL,
	.l_rint = SLINPUT,
	.l_rend = nodev,
	.l_meta = nulldev,
	.l_start = SLSTART,
	.l_modem = nulldev,
	.l_poll = nodev			/* add poll */
};

struct linesw linesw[] =
{
	ttyopen, ttylclose, ttread, ttwrite, nullioctl,	/* 0- OTTYDISC */
	ttyinput, nodev, nulldev, ttstart, ttymodem,
#if NBK > 0
	bkopen, bkclose, bkread, ttwrite, bkioctl,		/* 1- NETLDISC */
	bkinput, nodev, nulldev, ttstart, nullmodem,
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
	ttyopen, ttylclose, ttread, ttwrite, nullioctl,	/* 2- NTTYDISC */
	ttyinput, nodev, nulldev, ttstart, ttymodem,
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nullmodem,	/* 3- TABLDISC */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
#if NSL > 0
	SLOPEN, SLCLOSE, nodev, nodev, SLTIOCTL,
	SLINPUT, nodev, nulldev, SLSTART, nulldev,		/* 4- SLIPDISC */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
};
int	nldisp = sizeof (linesw) / sizeof (linesw[0]);

/* New devswio conf initialization for tty */
void
tty_conf_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &ottydisc);	/* 0- OTTYDISC */
#if NBK > 0
	DEVSWIO_CONFIG_INIT(devsw, NBK, NULL, NULL, &netldisc);	/* 1- NETLDISC */
#endif
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &nttydisc);	/* 2- NTTYDISC */
#if NTB > 0
	DEVSWIO_CONFIG_INIT(devsw, NTB, NULL, NULL, &tabldisc);	/* 3- TABLDISC */
#endif
#if NSL > 0
	DEVSWIO_CONFIG_INIT(devsw, NSL, NULL, NULL, &slipdisc);	/* 4- SLIPDISC */
#endif
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

#if NSL > 0
SLOPEN(dev, tp)
	dev_t dev;
	struct tty *tp;
{
	int error, slopen();

	if (!suser())
		return (EPERM);
	if (tp->t_line == SLIPDISC)
		return (EBUSY);
	error = KScall(slopen, sizeof(dev_t) + sizeof(struct tty *), dev, tp);
	if (!error)
		ttyflush(tp, FREAD | FWRITE);
	return(error);
}

SLCLOSE(tp, flag)
	struct tty *tp;
	int flag;
{
	int slclose();

	ttywflush(tp);
	tp->t_line = 0;
	KScall(slclose, sizeof(struct tty *), tp);
}

SLTIOCTL(tp, cmd, data, flag)
	struct tty *tp;
	int cmd, flag;
	caddr_t data;
{
	int sltioctl();

	return(KScall(sltioctl, sizeof(struct tty *) + sizeof(int) +
	    sizeof(caddr_t) + sizeof(int), tp, cmd, data, flag));
}

SLSTART(tp)
	struct tty *tp;
{
	void slstart();

	KScall(slstart, sizeof(struct tty *), tp);
}

SLINPUT(c, tp)
	int c;
	struct tty *tp;
{
	void slinput();

	KScall(slinput, sizeof(int) + sizeof(struct tty *), c, tp);
}
#endif
