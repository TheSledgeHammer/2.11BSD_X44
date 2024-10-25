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

#include "namespace.h"

#include <sys/param.h>
#include <stddef.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(telldir,_telldir)
#endif

/*
 * return a pointer into a directory
 */
long
telldir(dirp)
	const DIR *dirp;
{
    return (lseek(dirp->dd_fd, 0L, SEEK_CUR) - dirp->dd_size + dirp->dd_loc);
}
