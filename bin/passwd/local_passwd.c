/*-
 * Copyright (c) 1990, 1993, 1994
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
#endif
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)passwd.c	4.35 (Berkeley) 3/16/89";
static char sccsid[] = "@(#)local_passwd.c	8.3 (Berkeley) 4/2/94";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/signal.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#ifdef LOGIN_CAP
#include <login_cap.h>
#endif
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"

static const char *local_msg = "Changing local password for";

uid_t uid;

int
local_passwd(const char *uname, const char *temp, const char *type, const char *option)
{
	struct passwd *pw;
	int pfd, tfd;
	int min_pw_len;
	int pw_expiry;
#ifdef LOGIN_CAP
	login_cap_t *lc;
#endif

	min_pw_len = 0;
	pw_expiry = 0;
	uid = getuid();
	if (!(pw = getpwuid(uid))) {
		(void)fprintf(stderr, "passwd: unknown user: uid %u\n", uid);
		exit(1);
	}

	if (!(pw = getpwnam(uname))) {
		(void)fprintf(stderr, "passwd: unknown user %s.\n", uname);
		exit(1);
	}

	if (uid && uid != pw->pw_uid) {
		(void)fprintf(stderr, "passwd: %s\n", strerror(EACCES));
		exit(1);
	}

	pw_init();
	pfd = pw_lockpw(pw, 0);
	tfd = pw_tmp(temp);
	if (pfd < 0) {
		warnx("The passwd file is busy, waiting...");
		pfd = pw_lockpw(pw, 10);
		if (pfd < 0) {
			errx(1, "The passwd file is still busy, "
					"try again later.");
		}
	}

	/*
	 * Get class restrictions for this user, then get the new password.
	 */
#ifdef LOGIN_CAP
	if ((lc = login_getclass(pw->pw_class))) {
		min_pw_len = (int)login_getcapnum(lc, "minpasswordlen", 0, 0);
		pw_expiry = (int)login_getcaptime(lc, "passwordtime", 0, 0);
		login_close(lc);
	}
#endif

	/*
	 * Get the new password.  Reset passwd change time to zero; when
	 * classes are implemented, go and get the "offset" value for this
	 * class and reset the timer.
	 */
	pw->pw_passwd = getnewpasswd(pw, uid, local_msg, min_pw_len, temp, type, option);
	pw->pw_change = pw_expiry ? pw_expiry + time((time_t *) NULL) : 0;
	pw_copy(pfd, tfd, pw);

	if (!pw_mkdb(temp)) {
		pw_error("passwd: passwd failed", 1, 1);
	}
	return (0);
}
