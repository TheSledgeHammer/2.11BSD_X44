/* $NetBSD: kill.c,v 1.32 2020/08/30 19:35:09 kre Exp $ */

/*
 * Copyright (c) 1988, 1993, 1994
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
#if !defined(lint) && !defined(SHELL)
__COPYRIGHT("@(#) Copyright (c) 1988, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)kill.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: kill.c,v 1.32 2020/08/30 19:35:09 kre Exp $");
#endif
#endif /* not lint */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <locale.h>
#include <sys/ioctl.h>

#ifdef SHELL            /* sh (aka ash) builtin */
int killcmd(int, char *argv[]);
#define main killcmd
#include "../../bin/sh/bltin/bltin.h"
#endif /* SHELL */ 

__dead static void nosig(const char *);
void printsignals(FILE *fp);
static int signame_to_signum(char *);
__dead static void usage(void);

int
main(int argc, char *argv[])
{
	pid_t pid;
	int errors, numsig;
	char *ep;

	if (argc < 2)
		usage();

	numsig = SIGTERM;

	argc--, argv++;
	if (!strcmp(*argv, "-l")) {
		argc--, argv++;
		if (argc > 1)
			usage();
		if (argc == 1) {
			if (!isdigit(**argv))
				usage();
			numsig = strtol(*argv, &ep, 10);
			if (!*argv || *ep)
				errx(1, "illegal signal number: %s", *argv);
			if (numsig >= 128)
				numsig -= 128;
			if (numsig <= 0 || numsig >= NSIG)
				nosig(*argv);
			printf("%s\n", sys_signame[numsig]);
			exit(0);
		}
		printsignals(stdout);
		exit(0);
	}

	if (!strcmp(*argv, "-s")) {
		argc--, argv++;
		if (argc < 1) {
			warnx("option requires an argument -- s");
			usage();
		}
		if (strcmp(*argv, "0")) {
			if ((numsig = signame_to_signum(*argv)) < 0)
				nosig(*argv);
		} else
			numsig = 0;
		argc--, argv++;
	} else if (**argv == '-') {
		++*argv;
		if (isalpha(**argv)) {
			if ((numsig = signame_to_signum(*argv)) < 0)
				nosig(*argv);
		} else if (isdigit(**argv)) {
			numsig = strtol(*argv, &ep, 10);
			if (!*argv || *ep)
				errx(1, "illegal signal number: %s", *argv);
			if (numsig <= 0 || numsig >= NSIG)
				nosig(*argv);
		} else
			nosig(*argv);
		argc--, argv++;
	}

	if (argc == 0)
		usage();

	for (errors = 0; argc; argc--, argv++) {
#ifdef SHELL
        extern int getjobpgrp(const char *);

        if (**argv == '%') {
            pid = getjobpgrp(*argv);
        } else
#endif
        {
            pid = (pid_t)strtoimax(*argv, &ep, 10);
        }
		if (!*argv || *ep) {
			warnx("illegal process id: %s", *argv);
			errors = 1;
		} else if (kill(pid, numsig) == -1) {
			warn("%s", *argv);
			errors = 1;
		}
	}
	exit(errors);
}

static int
signame_to_signum(char *sig)
{
	int n;

	if (!strncasecmp(sig, "sig", 3))
		sig += 3;
	for (n = 1; n < NSIG; n++) {
		if (!strcasecmp(sys_signame[n], sig))
			return (n);
	}
	return (-1);
}

static void
nosig(const char *name)
{

	warnx("unknown signal %s; valid signals:", name);
	printsignals(stderr);
    exit(1);
	/* NOTREACHED */
}

void
printsignals(FILE *fp)
{
    int n;

    for (n = 1; n < NSIG; n++) {
        (void)fprintf(fp, "%s", sys_signame[n]);
        if ((n == (NSIG / 2)) || (n == (NSIG - 1))) {
            (void)fprintf(fp, "\n");
        } else {
            (void)fprintf(fp, " ");
        }
    }
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: kill [-s signal_name] pid ...\n");
	(void)fprintf(stderr, "       kill -l [exit_status]\n");
	(void)fprintf(stderr, "       kill -signal_name pid ...\n");
	(void)fprintf(stderr, "       kill -signal_number pid ...\n");
	exit(1);
	/* NOTREACHED */
}
