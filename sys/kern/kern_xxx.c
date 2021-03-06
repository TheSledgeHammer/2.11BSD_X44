/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_xxx.c	1.2 (2.11BSD) 2000/2/20
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/signal.h>
#include <sys/reboot.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <vm/include/vm.h>
#include <sys/sysctl.h>
#include <sys/proc.h>

void
reboot()
{
	register struct a {
		syscallarg(int)	opt;
	} *uap = (struct a *)u->u_ap;

	if (suser())
		boot(rootdev, uap->opt);
}
