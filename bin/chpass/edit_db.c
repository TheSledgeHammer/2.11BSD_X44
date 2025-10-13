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
static void loadpw(char *arg, struct passwd *pw);


void
chpass_db(struct passwd *pw, int pfd, int tfd)
{
	if (!pw_mkdb(arg)) {
		pw_error(NULL, 0, 1);
	}
	exit(0);
}

static int check(FILE *fp, struct passwd *pw, struct entry list[]);


void
edit(char *tempname, struct passwd *pw, struct entry list[])
{
	struct stat begin, end;

	for (rval = 0;;) {
		if (stat(tempname, &begin)) {
			pw_error(tempname, 1, 1);
		}
		if (pw_edit(tempname, 1)) {
			(void)fprintf(stderr, "chpass: edit failed; ");
			break;
		}
		if (begin.st_mtime == end.st_mtime) {
			(void)fprintf(stderr, "chpass: no changes made; ");
			break;
		}
		if (verify(tempname, pw, list)) {
			break;
		}
		pw_prompt();
	}
}

/*
 * display --
 *	print out the file for the user to edit; strange side-effect:
 *	set conditional flag if the user gets to edit the shell.
 */
void
display(char *tempname, int fd, struct passwd *pw,  struct entry list[])
{
	FILE *fp;
	char *bp, *p;

	if (!(fp = fdopen(fd, "w+"))) {
		pw_error(tempname, 1, 1);
	}

	(void)fprintf(fp,
	    "#Changing user database information for %s.\n", pw->pw_name);
	if (!uid) {
		(void)fprintf(fp, "Login: %s\n", pw->pw_name);
		(void)fprintf(fp, "Password: %s\n", pw->pw_passwd);
		(void)fprintf(fp, "Uid [#]: %d\n", pw->pw_uid);
		(void)fprintf(fp, "Gid [# or name]: %d\n", pw->pw_gid);
		(void)fprintf(fp, "Change [month day year]: %s\n",
		    ttoa(pw->pw_change));
		(void)fprintf(fp, "Expire [month day year]: %s\n",
		    ttoa(pw->pw_expire));
		(void)fprintf(fp, "Class: %s\n", pw->pw_class);
		(void)fprintf(fp, "Home directory: %s\n", pw->pw_dir);
		(void)fprintf(fp, "Shell: %s\n",
		    *pw->pw_shell ? pw->pw_shell : _PATH_BSHELL);
	}
	/* Only admin can change "restricted" shells. */
	else if (ok_shell(pw->pw_shell)) {
		/*
		 * Make shell a restricted field.  Ugly with a
		 * necklace, but there's not much else to do.
		 */
		(void)fprintf(fp, "Shell: %s\n",
		    *pw->pw_shell ? pw->pw_shell : _PATH_BSHELL);
	} else {
		list[E_SHELL].restricted = 1;
	}
	bp = pw->pw_gecos;
	p = strsep(&bp, ",");
	(void)fprintf(fp, "Full Name: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	(void)fprintf(fp, "Location: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	(void)fprintf(fp, "Office Phone: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	(void)fprintf(fp, "Home Phone: %s\n", p ? p : "");

	(void)fchown(fd, getuid(), getgid());
	(void)fclose(fp);
}


int
verify(char *tempname, struct passwd *pw, struct entry list[])
{
	struct stat sb;
	FILE *fp;
	int fd;

	fp = NULL;
	if ((fd = open(tempname, O_RDONLY|O_NOFOLLOW)) == -1 || !(fp = fdopen(fd, "r"))) {
		pw_error(tempname, 1, 1);
	}
	if (fstat(fd, &sb)) {
		pw_error(tempname, 1, 1);
	}
	if (sb.st_size == 0 || sb.st_nlink != 1) {
		(void)fclose(fp);
		return (0);
	}
	return (check(fp, pw, list));
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
