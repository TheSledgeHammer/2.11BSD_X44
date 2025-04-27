/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)lastlog.h	5.1 (Berkeley) 5/30/85
 */


#ifndef	_LASTLOG_H_
#define	_LASTLOG_H_

#include <utmp.h>

#ifndef _PATH_LASTLOG
#define	_PATH_LASTLOG	"/var/log/lastlog"
#endif

struct lastlog {
	time_t	ll_time;
	char	ll_line[UT_LINESIZE];
	char	ll_host[UT_HOSTSIZE];		/* same as in utmp */
};

#endif /* !_LASTLOG_H_ */
