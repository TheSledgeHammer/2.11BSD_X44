/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)utmp.h	5.6.1 (2.11BSD) 1996/11/27
 */

#ifndef	_UTMP_H_
#define	_UTMP_H_

#include <sys/cdefs.h>
#include <time.h> /* for time_t */

#define	_PATH_UTMP		"/var/run/utmp"
//#define	_PATH_WTMP		"/usr/adm/wtmp"
#define	_PATH_WTMP		"/var/log/wtmp"

#define	UT_NAMESIZE	15
#define	UT_LINESIZE	8
#define	UT_HOSTSIZE	16

struct utmp {
	char	ut_line[UT_LINESIZE];
	char	ut_name[UT_NAMESIZE];
	char	ut_host[UT_HOSTSIZE];
	long	ut_time;
};

__BEGIN_DECLS
int 		utmpname(const char *);
void 		setutent(void);
struct utmp *getutent(void);
void 		endutent(void);
__END_DECLS
#endif /* !_UTMP_H_ */
