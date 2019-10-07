/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rlauto.c	2.2 (2.11BSD GTE) 1997/7/20
 */

#include "../machine/autoconfig.h"
#include "../machine/machparam.h"
#include "../sys/param.h"
#include "rlreg.h"

extern	int	errno;

rlprobe(addr,vector)
	struct rldevice *addr;
	int vector;
{
	stuff(RL_CRDY, (&(addr->rlcs)));
	if	(errno)			/* paranoia */
		return(ACP_NXDEV);
	return(ACP_EXISTS);
}
