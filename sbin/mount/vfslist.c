/*	$NetBSD: vfslist.c,v 1.4 2003/08/07 10:04:26 agc Exp $	*/

/*
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)vfslist.c	8.1 (Berkeley) 5/8/95";
#else
__RCSID("$NetBSD: vfslist.c,v 1.4 2003/08/07 10:04:26 agc Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vfslist.h"

static int skipvfs, *typelist;
static enum { IN_LIST, NOT_IN_LIST } which;
static const char *vfsnames[] = INITMOUNTNAMES;

int
checkvfsname(const char *vfsname, const char **vfslist)
{
	if (vfslist == NULL) {
		return (0);
	}
	while (*vfslist != NULL) {
		if (strcmp(vfsname, *vfslist) == 0) {
			return (skipvfs);
		}
		++vfslist;
	}
	return (!skipvfs);
}

int
checkvfstype(int vfstype, const char **vfslist)
{
	if ((vfstype < 0) || (vfstype > MOUNT_MAXTYPE)) {
		return (0);
	}
	return (checkvfsname(vfsnames[vfstype], vfslist));
}

int
selected(int type)
{
	/* If no type specified, it's always selected. */
	if (typelist == NULL) {
		return (1);
	}

	for (; *typelist != 0; ++typelist) {
		if (type == *typelist) {
			return (which == IN_LIST ? 1 : 0);
		}
	}
	return (which == IN_LIST ? 0 : 1);
}

int
fsnametotype(const char *name)
{
	const char **cp;

	for (cp = vfsnames; *cp; ++cp) {
		if (strcmp(name, *cp) == 0) {
			return (cp - vfsnames);
		}
	}
	return (0);
}

const char **
makevfslist(const char *fslist)
{
	const char **av;
	int i;
	char *nextcp;

	if (fslist == NULL) {
		return (NULL);
	}
	if (fslist[0] == 'n' && fslist[1] == 'o') {
		fslist += 2;
		skipvfs = 1;
	}
	for (i = 0, nextcp = fslist; *nextcp; nextcp++) {
		if (*nextcp == ',') {
			i++;
		}
	}
	if ((av = malloc((size_t)(i + 2) * sizeof(char *))) == NULL) {
		warn("malloc");
		return (NULL);
	}
	nextcp = fslist;
	i = 0;
	av[i++] = nextcp;
	while ((nextcp = strchr(nextcp, ',')) != NULL) {
		*nextcp++ = '\0';
		av[i++] = nextcp;
	}
	av[i++] = NULL;
	return (av);
}

void
maketypelist(const char *fslist)
{
	int *av, i;
	char *nextcp;

	if ((fslist == NULL) || (fslist[0] == '\0')) {
		errx(1, "empty type list");
	}

	/*
	 * XXX
	 * Note: the syntax is "noxxx,yyy" for no xxx's and
	 * no yyy's, not the more intuitive "noyyy,noyyy".
	 */
	if (fslist[0] == 'n' && fslist[1] == 'o') {
		fslist += 2;
		which = NOT_IN_LIST;
	} else {
		which = IN_LIST;
	}

	/* Count the number of types. */
	for (i = 0, nextcp = fslist; *nextcp != NULL; ++nextcp) {
		if (*nextcp == ',') {
			i++;
		}
	}

	/* Build an array of that many types. */
	if ((av = typelist = (int *)malloc((i + 2) * sizeof(int))) == NULL) {
		err(1, NULL);
	}
	for (i = 0; fslist != NULL; fslist = nextcp, ++i) {
		if ((nextcp = strchr(fslist, ',')) != NULL) {
			*nextcp++ = '\0';
		}
		av[i] = fsnametotype(fslist);
		if (av[i] == 0) {
			errx(1, "%s: unknown mount type", fslist);
		}
	}
	/* Terminate the array. */
	av[i++] = 0;
}
