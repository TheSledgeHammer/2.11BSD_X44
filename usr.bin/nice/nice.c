/*	$NetBSD: nice.c,v 1.12 2003/08/07 11:15:24 agc Exp $	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
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
__COPYRIGHT(
    "@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)nice.c	5.4 (Berkeley) 6/1/90";
#endif
__RCSID("$NetBSD: nice.c,v 1.12 2003/08/07 11:15:24 agc Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>

#define	DEFNICE	10

int	main(int, char **);
static void usage(void);

int
main(argc, argv)
	int argc;
	char **argv;
{
	int niceness = DEFNICE;
	int c;

	setlocale(LC_ALL, "");

	/* handle obsolete -number syntax */
	if (argc > 1 && argv[1][0] == '-' && isdigit((unsigned char) argv[1][1])) {
		niceness = atoi(argv[1] + 1);
		argc--;
		argv++;
	}

	while ((c = getopt (argc, argv, "n:")) != -1) {
		switch (c) {
		case 'n':
			niceness = atoi (optarg);
			break;

		case '?':
		default:
			usage();
			break;
		}
	}
	argc -= optind; argv += optind;

	if (argc == 0)
		usage();

	errno = 0;
	niceness += getpriority(PRIO_PROCESS, 0);
	if (errno) {
		err (1, "getpriority");
		/* NOTREACHED */
	}
	if (setpriority(PRIO_PROCESS, 0, niceness)) {
		warn ("setpriority");
	}

	execvp(argv[0], &argv[0]);
	err ((errno == ENOENT) ? 127 : 126, "%s", argv[0]);
	/* NOTREACHED */
}

static void
usage()
{
	(void)fprintf(stderr,
	    "usage: nice [ -n increment ] utility [ argument ...]\n");
	
	exit(1);
}
