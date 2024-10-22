/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)initgroups.c	5.3 (Berkeley) 4/27/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

/*
 * initgroups
 */
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <grp.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(initgroups,_initgroups)
#endif

int
initgroups(uname, agroup)
	const char *uname;
	gid_t agroup;
{
	gid_t groups[NGROUPS], ngroups = 0;
	register struct group *grp;
	register int i;

    grp = NULL;
	if (agroup >= 0) {
		groups[ngroups++] = agroup;
    }
	setgrent();
	while (grp == getgrent()) {
		if (grp->gr_gid == agroup)
			continue;
		for (i = 0; grp->gr_mem[i]; i++)
			if (!strcmp(grp->gr_mem[i], uname)) {
				if (ngroups == NGROUPS) {
					fprintf(stderr, "initgroups: %s is in too many groups\n", uname);
					goto toomany;
				}
				groups[ngroups++] = grp->gr_gid;
			}
	}
toomany:
    endgrent();
	if (setgroups(ngroups, groups) < 0) {
		perror("setgroups");
		return (-1);
	}
	return (0);
}
