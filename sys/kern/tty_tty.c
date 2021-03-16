/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_tty.c	1.2 (2.11BSD GTE) 11/29/94
 */

/*
 * Indirect driver for controlling tty compatibility. Pass through to ctty
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>

/*ARGSUSED*/
int
syopen(dev, flag, type)
	dev_t dev;
	int flag, type;
{
	if (u->u_ttyp == NULL)
		return (ENXIO);
	return (cttyopen(u->u_ttyd, flag, type, u->u_procp));
}

/*ARGSUSED*/
int
syread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	if (u->u_ttyp == NULL)
		return (ENXIO);
	return (cttyread(u->u_ttyd, uio, flag));
}

/*ARGSUSED*/
int
sywrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	if (u->u_ttyp == NULL)
		return (ENXIO);
	return (cttywrite(u->u_ttyd, uio, flag));
}

/*ARGSUSED*/
int
syioctl(dev, cmd, addr, flag)
	dev_t dev;
	u_int cmd;
	caddr_t addr;
	int flag;
{
	if (cmd == TIOCNOTTY) {
		u->u_ttyp = 0;
		u->u_ttyd = 0;
		u->u_procp->p_pgrp = 0;
		return (0);
	}
	if (u->u_ttyp == NULL)
		return (ENXIO);
	return (cttyioctl(u->u_ttyd, cmd, addr, flag, u->u_procp));
}

/*ARGSUSED*/
int
syselect(dev, flag)
	dev_t dev;
	int flag;
{

	if (u->u_ttyp == NULL) {
		u->u_error = ENXIO;
		return (0);
	}
	return (cttyselect(u->u_ttyd, flag, u->u_procp));
}

int
sypoll(dev, events)
	dev_t dev;
	int events;
{

	if (u->u_ttyp == NULL) {
		u->u_error = ENXIO;
		return (0);
	}
	return (cttypoll(u->u_ttyd, events, u->u_procp));
}
