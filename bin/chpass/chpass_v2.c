/*-
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

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <libgen.h>

#include "chpass.h"

char e1[] = ": ";
char e2[] = ":,";

struct entry list[] = {
	{ "Login",		p_login,  1,   5, e1,   },
	{ "Password",		p_passwd, 1,   8, e1,   },
	{ "Uid",		p_uid,    1,   3, e1,   },
	{ "Gid",		p_gid,    1,   3, e1,   },
	{ "Class",		p_class,  1,   5, e1,   },
	{ "Change",		p_change, 1,   6, NULL, },
	{ "Expire",		p_expire, 1,   6, NULL, },
	{ "Full Name",		p_gecos,  0,   9, e2,   },
	{ "Office Phone",	p_gecos,  0,  12, e2,   },
	{ "Home Phone",		p_gecos,  0,  10, e2,   },
	{ "Location",		p_gecos,  0,   8, e2,   },
	{ "Home directory",	p_hdir,   1,  14, e1,   },
	{ "Shell",		p_shell,  0,   5, e1,   },
	{ NULL, 0, },
};

static char tempname[] = "/tmp/passwd.XXXXXX";
uid_t uid;

static void usage(void);

int
main(int argc, char **argv)
{
	enum { NEWSH, LOADENTRY, EDITENTRY } op;
    struct passwd lpw, *pw;
    int ch, pfd, tfd;
    char *arg;

	uid = getuid();
	while ((ch = getopt(argc, argv, "a:s:")) != EOF) {
		switch (ch) {
		case 'a':
			op = LOADENTRY;
			arg = optarg;
			break;
		case 's':
			op = NEWSH;
			arg = optarg;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (op == EDITENTRY || op == NEWSH) {
		switch (argc) {
		case 0:
			if (!(pw = getpwuid(uid))) {
				errx(1, "chpass: unknown user: uid %u\n", uid);
			}
			break;
		case 1:
			if (!(pw = getpwnam(*argv))) {
				errx(1, "chpass: unknown user %s.\n", *argv);
			}
			if (uid && uid != pw->pw_uid) {
				errx(1, "chpass: %s\n", strerror(EACCES));
			}
			break;
		default:
			usage();
		}
	}

	if (op == NEWSH) {
		if (!arg[0]) {
			usage();
		}
		if (p_shell(arg, pw, (struct entry *)NULL)) {
			pw_error((char *)NULL, 0, 1);
		}
	}

	if (op == LOADENTRY) {
		if (uid) {
			errx(1, "chpass: %s\n", strerror(EACCES));
		}
		pw = &lpw;
		if (!pw_scan(arg, pw)) {
			exit(1);
		}
	}

	/*
	 * The temporary file/file descriptor usage is a little tricky here.
	 * 1:	We start off with two fd's, one for the master password
	 *	file (used to lock everything), and one for a temporary file.
	 * 2:	Display() gets an fp for the temporary file, and copies the
	 *	user's information into it.  It then gives the temporary file
	 *	to the user and closes the fp, closing the underlying fd.
	 * 3:	The user edits the temporary file some number of times.
	 * 4:	Verify() gets an fp for the temporary file, and verifies the
	 *	contents.  It can't use an fp derived from the step #2 fd,
	 *	because the user's editor may have created a new instance of
	 *	the file.  Once the file is verified, its contents are stored
	 *	in a password structure.  The verify routine closes the fp,
	 *	closing the underlying fd.
	 * 5:	Delete the temporary file.
	 * 6:	Get a new temporary file/fd.  Pw_copy() gets an fp for it
	 *	file and copies the master password file into it, replacing
	 *	the user record with a new one.  We can't use the first
	 *	temporary file for this because it was owned by the user.
	 *	Pw_copy() closes its fp, flushing the data and closing the
	 *	underlying file descriptor.  We can't close the master
	 *	password fp, or we'd lose the lock.
	 * 7:	Call pw_mkdb() (which renames the temporary file) and exit.
	 *	The exit closes the master passwd fp/fd.
	 */
	pw_init();
	pfd = pw_lockpw(pw, 0);
	tfd = pw_tmp(tempname);
	if (pfd < 0) {
		warnx("The passwd file is busy, waiting...");
		pfd = pw_lockpw(pw, 10);
		if (pfd < 0) {
			errx(1, "The passwd file is still busy, "
					"try again later.");
		}
	}

	if (op == EDITENTRY) {
		if (!info(tempname, tfd, pw, list)) {
			pw_error(tempname, 1, 1);
		}
		tfd = pw_tmp();
	}

	pw_copy(pfd, tfd, pw);

	switch (fork()) {
	case 0:
		break;
	case -1:
		pw_error("chpass: can't fork; ", 1, 1);
		break;
		/* NOTREACHED */
	default:
		exit(0);
		/* NOTREACHED */
	}

	if (!pw_mkdb(tempname)) {
		pw_error("chpass: passwd failed; ", 1, 1);
	}
#ifdef USE_NDBM
	pw_dirpag_rename();
#else
	exit(0);
#endif
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: chpass [-a list] [-s shell] [user]\n");
	exit(1);
}
