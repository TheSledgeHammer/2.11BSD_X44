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

static int use_crypto = 1;

static void passwd_conf(const char *arg, int flags);
static void usage(void);

static const char *crypto_type = NULL;
static const char *crypto_option = NULL;

int
main(int argc, char **argv)
{
//	struct passwd *pw;
	const char *uname, *username;
	char *arg;
	int ch, eval;

	uname = NULL;
	arg = NULL;
	while ((ch = getopt(argc, argv, "a:l:")) != EOF) {
		switch (ch) {
		case 'a':
			use_crypto = 0;
			arg = optarg;
			break;
		case 'l':
		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	username = getlogin();
	if (username == NULL) {
		errx(1, "who are you ??");
	}

	passwd_conf(arg, use_crypto);

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

	eval = local_passwd(uname, username, crypto_type, crypto_option);
#ifdef USE_NDBM
	pw_dirpag_rename(eval);
#else
	exit(eval);
#endif
}

char *
getnewpasswd(struct passwd *pw, uid_t uid, const char *msg, int min_pw_len, const char *temp, const char *type, const char *option)
{
	register char *p, *t;
	int rval, tries;
	char buf[_PASSWORD_LEN + 1], salt[_PASSWORD_LEN + 1];

	(void)printf("%s %s.\n", msg, pw->pw_name);

	if (uid && pw->pw_passwd[0]
			&& strcmp(crypt(getpass("Old password:"), pw->pw_passwd),
					pw->pw_passwd)) {
		(void)printf("passwd: %s.\n", strerror(EACCES));
		pw_error(temp, 0, 1);
	}

	for (buf[0] = '\0', tries = 0;;) {
		p = getpass("New password:");
		if (!*p) {
			(void)printf("Password unchanged.\n");
			pw_error(temp, 0, 0);
		}
		if (min_pw_len > 0 && (int) strlen(p) < min_pw_len) {
			(void)printf("Password is too short.\n");
			continue;
		}
		if (strlen(p) <= 5 && (uid != 0 || ++tries < 2)) {
			(void)printf("Please enter a longer password.\n");
			continue;
		}
		for (t = p; *t && islower(*t); ++t)
			;
		if (!*t && (uid != 0 || ++tries < 2)) {
			(void)printf(
					"Please don't use an all-lower case password.\nUnusual capitalization, control characters or digits are suggested.\n");
			continue;
		}
		(void)strcpy(buf, p);
		if (!strcmp(buf, getpass("Retype new password:"))) {
			break;
		}
		(void)printf("Mismatch; try again, EOF to quit.\n");
	}
	rval = pwd_gensalt(salt, _PASSWORD_LEN, type, option);
	if (!rval) {
		(void)printf("Couldn't generate salt.\n");
	}
	return (crypt(buf, salt));
}

static void
passwd_conf(const char *arg, int flags)
{
	const char *type, *option;
	int rval;

	type = NULL;
	option = NULL;
	rval = pwd_conf(&type, &option);
	if (rval == 0) {
		if (flags != 0) {
			crypto_type = type;
			crypto_option = option;
		} else {
			if (!strcasecmp(arg, "old")) {
				type = "old";
			} else if (!strcasecmp(arg, "new")) {
				type = "new";
			} else if (!strcasecmp(arg, "md5")) {
				type = "md5";
			} else if (!strcasecmp(arg, "blowfish")) {
				type = "blowfish";
			} else if (!strcasecmp(arg, "sha1")) {
				type = "sha1";
			} else {
				warnx("illegal argument to -a option\n");
				usage();
			}
			crypto_type = type;
			crypto_option = option;
		}
	} else {
		errx(1,
				"passwd: No gensalt algorithm and/or cipher has been specified\n");
	}
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: passwd [-a algorithm] [-l] [user]\n");
	(void)fprintf(stderr, "       algorithms: old new md5 blowfish sha1\n");
	exit(1);
}
