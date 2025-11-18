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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
char copyright[] =
"@(#) Copyright (c) 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
"@(#) Copyright (c) 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)passwd.c	4.35 (Berkeley) 3/16/89";
static char sccsid[] = "@(#)passwd.c	8.3 (Berkeley) 4/2/94";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <util.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"

#ifdef USE_KERBEROS
static int use_kerberos = 1;
#endif
#ifdef USE_PAM
static int use_pam = 1;
#endif
#ifdef USE_YP
static int use_yp = 1;
#endif

static char tempname[] = "/tmp/passwd.XXXXXX";
static const char *default_option = "blowfish";
static int use_default = 0;

void usage(void);

int
main(int argc, char **argv)
{
	struct passwd *pw;
	const char *uname, *option;
	char *arg;
	int ch, eval;

	option = default_option;
	while ((ch = getopt(argc, argv, "a:l:")) != EOF) {
		switch (ch) {
		case 'a':
			use_default = 1;
			pwd_algorithm(option, use_default);
			arg = optarg;
			break;
#ifdef USE_KERBEROS
		case 'k':
			use_kerberos = 0;
			arg = optarg;
			break;
#endif
		case 'l':
			arg = optarg;
			break;
#ifdef USE_PAM
		case 'p':
			use_pam = 0;
			arg = optarg;
			break;
#endif
#ifdef USE_YP
		case 'y':
			use_yp = 0;
			arg = optarg;
			break;
#endif
		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	switch (argc) {
	case 0:
		break;
	case 1:
		uname = argv[0];
		break;
	default:
		usage();
	}

	switch (fork()) {
	case 0:
		break;
	case -1:
		pw_error("passwd: can't fork", 1, 1);
		/* NOTREACHED */
	default:
		exit(0);
		/* NOTREACHED */
	}

	eval = local_passwd(uname, tempname, option);
#ifdef USE_NDBM
	pw_dirpag_rename(eval);
#else
	exit(eval);
#endif
}

void
usage(void)
{
#ifdef USE_KERBEROS
	fprintf(stderr, "usage: passwd [-k] [user]\n");
#endif
	fprintf(stderr, "usage: passwd [-l] [user]\n");
#ifdef USE_PAM
	fprintf(stderr, "usage: passwd [-p] [user]\n");
#endif
#ifdef USE_YP
	fprintf(stderr, "usage: passwd [-y] [user]\n");
#endif
	exit(1);
}
