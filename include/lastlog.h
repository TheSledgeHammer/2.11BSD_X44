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

#define	_PATH_LASTLOG	"/var/log/lastlog"

struct lastlog {
	time_t	ll_time;
	char	ll_line[8];
	char	ll_host[16];		/* same as in utmp */
};

#endif /* !_LASTLOG_H_ */
