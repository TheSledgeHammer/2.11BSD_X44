/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)times.h	7.1 (Berkeley) 6/4/86
 */

#ifndef	_SYS_TIMES_H_
#define	_SYS_TIMES_H_

/*
 * Structure returned by times()
 */
struct tms {
	time_t	tms_utime;		/* user time */
	time_t	tms_stime;		/* system time */
	time_t	tms_cutime;		/* user time, children */
	time_t	tms_cstime;		/* system time, children */
};

#ifndef _KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
clock_t times(struct tms *);
__END_DECLS
#endif
#endif /* _SYS_TIMES_H_ */
