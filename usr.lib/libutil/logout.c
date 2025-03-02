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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "%W% (Berkeley) %G%";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <utmp.h>

typedef struct utmp UTMP;

int
logout(const char *line)
{
	register int fd;
	UTMP ut;
	int rval;

	if ((fd = open(_PATH_UTMP, O_RDWR)) < 0)
		return(0);
	rval = 0;
	while (read(fd, (char *)&ut, sizeof(UTMP)) == sizeof(UTMP)) {
		if (!ut.ut_name[0] || strncmp(ut.ut_line, line, UT_LINESIZE))
			continue;
		bzero(ut.ut_name, UT_NAMESIZE);
		bzero(ut.ut_host, UT_HOSTSIZE);
		(void)time(&ut.ut_time);
		(void)lseek(fd, -(long)sizeof(UTMP), SEEK_CUR);
		(void)write(fd, (char *)&ut, sizeof(UTMP));
		rval = 1;
	}
	(void)close(fd);
	return(rval);
}
