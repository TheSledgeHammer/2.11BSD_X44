/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)telldir.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <stddef.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>


/*
 * return a pointer into a directory
 */
long
telldir(dirp)
	DIR *dirp;
{
	extern long lseek();

	return (lseek(dirp->dd_fd, 0L, 1) - dirp->dd_size + dirp->dd_loc);
}
