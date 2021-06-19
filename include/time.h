/*
 * Copyright (c) 1983, 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)time.h	1.2 (Berkeley) 3/4/87
 */

#ifndef _TIME_H_
#define	_TIME_H_

#include <sys/cdefs.h>
#include <machine/ansi.h>

#include <sys/null.h>

#ifdef	_BSD_CLOCK_T_
typedef	_BSD_CLOCK_T_	clock_t;
#undef	_BSD_CLOCK_T_
#endif

#ifdef	_BSD_TIME_T_
typedef	_BSD_TIME_T_	time_t;
#undef	_BSD_TIME_T_
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_CLOCKID_T_
typedef	_BSD_CLOCKID_T_	clockid_t;
#undef	_BSD_CLOCKID_T_
#endif

#ifdef	_BSD_TIMER_T_
typedef	_BSD_TIMER_T_	timer_t;
#undef	_BSD_TIMER_T_
#endif

#define CLOCKS_PER_SEC	100
/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	int		tm_sec;
	int		tm_min;
	int		tm_hour;
	int		tm_mday;
	int		tm_mon;
	int		tm_year;
	int		tm_wday;
	int		tm_yday;
	int		tm_isdst;
	long	tm_gmtoff;
	char	*tm_zone;
};

__BEGIN_DECLS

extern	struct tm *gmtime(), *localtime();
extern	char *asctime(), *ctime();
/*
 * CLK_TCK uses libc's internal __sysconf() to retrieve the machine's
 * HZ. The value of _SC_CLK_TCK is 39 -- we hard code it so we do not
 * need to include unistd.h
 */
long __sysconf(int);
#define CLK_TCK		(__sysconf(39))

__END_DECLS

#endif /* !_TIME_H_ */
