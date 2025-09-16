/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)timeb.h	7.1 (Berkeley) 6/4/86
 */

#ifndef _SYS_TIMEB_H_
#define _SYS_TIMEB_H_

//#include <sys/time.h>

/*
 * Structure returned by ftime system call
 */
struct timeb {
	time_t			time;			/* seconds since the Epoch */
	unsigned short 	millitm;		/* + milliseconds since the Epoch */
	short			timezone;		/* minutes west of CUT */
	short			dstflag;		/* DST == non-zero */
};

#ifndef _KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
int ftime(struct timeb *);
__END_DECLS
#endif /* _KERNEL */

#endif /* _SYS_TIMEB_H_ */
