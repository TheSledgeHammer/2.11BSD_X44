/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_xxx.c	1.2 (2.11BSD) 2000/2/20
 */

#include "signal.h"

#include "../sys/fs.h"
#include "../sys/kernel.h"
#include "../sys/param.h"
#include "../sys/reboot.h"
#include "../sys/systm.h"
#include "../sys/user.h"

reboot()
{
	register struct a {
		int	opt;
	};

	if (suser())
		boot(rootdev, ((struct a *)u.u_ap)->opt);
}
