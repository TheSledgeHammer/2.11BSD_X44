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

#include <unistd.h>
#include <util.h>

#include "chpass.h"

static int check(FILE *fp, struct passwd *pw, struct entry list[]);
static int info(char *tempname, struct passwd *pw, struct entry list[]);
static void loadpw(char *arg, struct passwd *pw);

void
chpass_ndbm(struct passwd *pw, const char *passwd, const char *temp)
{
	int pfd, tfd;

	pw_init();

	if (info(tempname, pw, list)) {
		pw_error(NULL, 0, 1);
	}

	pfd = pw_lockpw(pw, 0);
	tfd = pw_tmp();

	switch (fork()) {
	case 0:
		break;
	case -1:
		(void)fprintf(stderr, "chpass: can't fork; ");
		goto bad;
		/* NOTREACHED */
	default:
		exit(0);
		/* NOTREACHED */
	}

	if (!pw_mkdb(arg)) {
bad:
		pw_error(NULL, 0, 1);
	}
	pw_dirpag(passwd, temp);
}

static int
info(char *tempname, struct passwd *pw, struct entry list[])
{
	struct stat sb, begin, end;
	FILE *fp;
	int fd, rval;

	if ((fd = open(tempname, O_RDONLY|O_NOFOLLOW)) == -1 || !(fp = fdopen(fd, "r"))) {
		(void)fprintf(stderr, "chpass: no temporary file: %s; ", tempname);
		return (0);
	}

	if (fstat(fd, &sb)) {
		pw_error(tempname, 1, 1);
	}

	if (sb.st_size == 0 || sb.st_nlink != 1) {
		(void)fclose(fp);
		return (0);
	}

	print(fp, pw);
	(void)fflush(fp);

	/*
	 * give the file to the real user; setuid permissions
	 * are discarded in edit()
	 */
	(void)fchown(fd, getuid(), getgid());

	for (rval = 0;;) {
		(void)fstat(fd, &begin);
		if (pw_edit(tempname, 1)) {
			(void)fprintf(stderr, "chpass: edit failed; ");
			break;
		}
		(void)fstat(fd, &end);
		if (begin.st_mtime == end.st_mtime) {
			(void)fprintf(stderr, "chpass: no changes made; ");
			break;
		}
		(void)rewind(fp);
		if (check(fp, pw, list)) {
			rval = 1;
			break;
		}
		(void)fflush(stderr);
		pw_prompt();
	}
	(void)fclose(fp);
	(void)unlink(tempname);
	return (rval);
}

static int
check(FILE *fp, struct passwd *pw, struct entry list[])
{
	register struct entry *ep;
	register char *p;
	int len;
	char buf[LINE_MAX];

	while (fgets(buf, sizeof(buf), fp)) {
		if (!buf[0] || buf[0] == '#')
			continue;
		if (!(p = strchr(buf, '\n'))) {
			warnx("line too long");
			return (0);
		}
		*p = '\0';
		for (ep = list;; ++ep) {
			if (!ep->prompt) {
				warnx("chpass: unrecognized field.\n");
				return (0);
			}
			if (!strncasecmp(buf, ep->prompt, ep->len)) {
				if (ep->restricted && uid) {
					warnx("chpass: you may not change the %s field.\n",
							ep->prompt);
					return (0);
				}
				if (!(p = strchr(buf, ':'))) {
					warnx("chpass: line corrupted.\n");
					return (0);
				}
				while (isspace(*++p));
				if (ep->except && strpbrk(p, ep->except)) {
					warnx("chpass: illegal character in the \"%s\" field.\n",
							ep->prompt);
					return (0);
				}
				if ((ep->func)(p, pw, ep)) {
					return (0);
				}
				break;
			}
		}
	}

	/*
	 * special checks...
	 *
	 * there has to be a limit on the size of the gecos fields,
	 * otherwise getpwent(3) can choke.
	 * ``if I swallow anything evil, put your fingers down my throat...''
	 *	-- The Who
	 */

	/* Build the gecos field. */
	len = strlen(list[E_NAME].save) + strlen(list[E_BPHONE].save) +
	    strlen(list[E_HPHONE].save) + strlen(list[E_LOCATE].save) + 4;
	if (!(p = malloc(len))) {
		err(1, NULL);
	}
	(void)sprintf(pw->pw_gecos = p, "%s,%s,%s,%s", list[E_NAME].save,
	    list[E_LOCATE].save, list[E_BPHONE].save, list[E_HPHONE].save);

	if (snprintf(buf, sizeof(buf),
	    "%s:%s:%d:%d:%s:%ld:%ld:%s:%s:%s",
	    pw->pw_name, pw->pw_passwd, pw->pw_uid, pw->pw_gid, pw->pw_class,
	    pw->pw_change, pw->pw_expire, pw->pw_gecos, pw->pw_dir,
	    pw->pw_shell) >= sizeof(buf)) {
		warnx("entries too long");
		return (0);
	}
	return (pw_scan(buf, pw));
}

static void
loadpw(char *arg, struct passwd *pw)
{
	register char *cp;
	char	*bp = arg;

	pw->pw_name = strsep(&bp, ":");
	pw->pw_passwd = strsep(&bp, ":");
	if (!(cp = strsep(&bp, ":"))) {
		goto bad;
	}
	pw->pw_uid = atoi(cp);
	if (!(cp = strsep(&bp, ":"))) {
		goto bad;
	}
	pw->pw_gid = atoi(cp);
	pw->pw_class = strsep(&bp, ":");
	if (!(cp = strsep(&bp, ":"))) {
		goto bad;
	}
	pw->pw_change = atol(cp);
	if (!(cp = strsep(&bp, ":"))) {
		goto bad;
	}
	pw->pw_expire = atol(cp);
	pw->pw_gecos = strsep(&bp, ":");
	pw->pw_dir = strsep(&bp, ":");
	pw->pw_shell = strsep(&bp, ":");
	if (!pw->pw_shell || strsep(&bp, ":")) {
bad:
		(void)fprintf(stderr, "chpass: bad password list.\n");
		exit(1);
	}
}
