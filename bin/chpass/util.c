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
#if !defined(lint) && defined(DOSCCS)
#if 0
static char sccsid[] = "@(#)util.c	5.9.1 (2.11BSD) 1996/1/12";
static char sccsid[] = "@(#)util.c	8.4 (Berkeley) 4/2/94";
#endif
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#include "chpass.h"

static char dmsize[] =
	{ -1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static const char *months[] =
	{ "January", "February", "March", "April", "May", "June",
	  "July", "August", "September", "October", "November",
	  "December", NULL };

char *
ttoa(time_t tval)
{
	register struct tm *tp;
	static char tbuf[50];

	if (tval) {
		tp = localtime(&tval);
		(void) sprintf(tbuf, "%s %d, 19%d", months[tp->tm_mon], tp->tm_mday,
				tp->tm_year);
	} else {
		*tbuf = '\0';
	}
	return (tbuf);
} 

int
atot(const char *p, time_t *store)
{
	register char *t;
	const char **mp;
	static struct tm *lt;
	time_t tval;
	int day, month, year;

	if (!*p) {
		*store = 0;
		return (0);
	}
	if (!lt) {
		unsetenv("TZ");
		(void) time(&tval);
		lt = localtime(&tval);
	}
	if (!(t = strtok(__UNCONST(p), " \t"))) {
		goto bad;
	}
	for (mp = months;; ++mp) {
		if (!*mp) {
			goto bad;
		}
		if (!strncasecmp(*mp, t, 3)) {
			month = mp - months + 1;
			break;
		}
	}
	if (!(t = strtok((char*) NULL, " \t,")) || !isdigit(*t)) {
		goto bad;
	}
	day = atoi(t);
	if (!(t = strtok((char*) NULL, " \t,")) || !isdigit(*t)) {
		goto bad;
	}
	year = atoi(t);
	if (day < 1 || day > 31 || month < 1 || month > 12 || !year) {
		goto bad;
	}
/*
#define	TM_YEAR_BASE	1900
#define	EPOCH_YEAR		1970
#define	DAYSPERNYEAR	365
#define	DAYSPERLYEAR	366
#define	HOURSPERDAY		24
#define	MINSPERHOUR		60
#define	SECSPERMIN		60
#define	isleap(y) 		(((((y) % 4) == 0 && ((y) % 100) != 0 ) || (((y) % 400) == 0)))
*/

	if (year < 100) {
		year += TM_YEAR_BASE;
	}
	if (year <= EPOCH_YEAR) {
bad:
		return (1);
	}
	tval = (isleap(year) && month > 2);
	for (--year; year >= EPOCH_YEAR; --year) {
		tval += isleap(year) ?
		DAYSPERLYEAR : DAYSPERNYEAR;
	}
	while (--month) {
		tval += dmsize[month];
	}
	tval += day;
	tval = tval * HOURSPERDAY * MINSPERHOUR * SECSPERMIN;
	tval -= lt->tm_gmtoff;
	*store = tval;
	return (0);
}

void
display(FILE *fp, struct passwd *pw, struct entry list[])
{
	register char *p;
	char *bp;

	fprintf(fp, "#Changing user database information for %s.\n", pw->pw_name);
	if (!uid) {
		fprintf(fp, "Login: %s\n", pw->pw_name);
		fprintf(fp, "Password: %s\n", pw->pw_passwd);
		fprintf(fp, "Uid [#]: %d\n", pw->pw_uid);
		fprintf(fp, "Gid [# or name]: %d\n", pw->pw_gid);
		fprintf(fp, "Change [month day year]: %s\n", ttoa(pw->pw_change));
		fprintf(fp, "Expire [month day year]: %s\n", ttoa(pw->pw_expire));
		fprintf(fp, "Class: %s\n", pw->pw_class);
		fprintf(fp, "Home directory: %s\n", pw->pw_dir);
		fprintf(fp, "Shell: %s\n", *pw->pw_shell ? pw->pw_shell : _PATH_BSHELL);
	}
	/* only admin can change "restricted" shells */
	else if (ok_shell(pw->pw_shell)) {
		fprintf(fp, "Shell: %s\n", *pw->pw_shell ? pw->pw_shell : _PATH_BSHELL);
	} else {
		list[E_SHELL].restricted = 1;
		/*
		setusershell();
		for (;;) {
			if (!(p = getusershell())) {
				break;
			} else if (!strcmp(pw->pw_shell, p)) {
				fprintf(fp, "Shell: %s\n",
						*pw->pw_shell ? pw->pw_shell : _PATH_BSHELL);
				break;
			}
		}
		*/
	}
	bp = pw->pw_gecos;
	p = strsep(&bp, ",");
	fprintf(fp, "Full Name: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	fprintf(fp, "Location: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	fprintf(fp, "Office Phone: %s\n", p ? p : "");
	p = strsep(&bp, ",");
	fprintf(fp, "Home Phone: %s\n", p ? p : "");
}

const char *
ok_shell(const char *name)
{
	char *p;
	const char *sh;

	setusershell();
	while ((sh = getusershell())) {
		if (!strcmp(name, sh)) {
			return (name);
		}
		/* allow just shell name, but use "real" path */
		if ((p = strrchr(sh, '/')) && strcmp(name, p + 1) == 0) {
			return (sh);
		}
	}
	return (NULL);
}
