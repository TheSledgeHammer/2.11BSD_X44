/*
 *	DM-BB fake driver
 */

/*
 *	SCCS id	@(#)dhfdm.c	2.1	(Berkeley)	8/5/83
 */

#include "../../sys/conf.h"
#include "../../sys/tty.h"
#include "param.h"

struct	tty	dh11[];

dmopen(dev)
{
	register struct tty *tp;

	tp = &dh11[minor(dev)];
	tp->t_state |= CARR_ON;
}

dmctl(dev, bits)
{
}
