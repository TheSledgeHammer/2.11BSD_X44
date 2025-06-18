/*
 * Copyright (c) 1980, 1993
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char copyright[] =
"@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)comsat.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <paths.h>
#include <pwd.h>
#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)
#include <sgtty.h>
#else
#include <termios.h>
#endif
#include <time.h>
#include <vis.h>
#include <unistd.h>
#include <utmp.h>
#include <utmpx.h>

int	debug = 0;
#define	dsyslog	if (debug) syslog

#define MAXIDLE	120

static char	hostname[MAXHOSTNAMELEN + 1];
static int	nutmp, uf;

static struct utmp  *utmp = NULL;
static struct utmpx *utmpx = NULL;

static time_t	lastmsgtime;

static void reapchildren(int);
static void utmpx_read(struct utmpx *);
static void utmp_read(struct utmp *);
static void onalrm(int);
static void utmp_notify(struct utmp *, char *, off_t);
static void utmpx_notify(struct utmpx *, char *, off_t);
static void mailfor(char *);
static void notify(const char *, size_t, const char *, size_t, off_t);
static void jkfprintf(FILE *, const char *, off_t);
#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)
static void notify_sgtty(FILE *, struct sgttyb *, char *);
#else
static void notify_termios(FILE *, struct termios *, char *);
#endif

int
main(int argc, char *argv[])
{
	struct sockaddr_storage from;
	int cc;
	socklen_t fromlen;
	char msgbuf[100];

	/* verify proper invocation */
	fromlen = sizeof(from);
	if (getsockname(0, (struct sockaddr *)(void *)&from, &fromlen) == -1) {
		err(1, "getsockname");
	}
	openlog("comsat", LOG_PID, LOG_DAEMON);
	if (chdir(_PATH_MAILDIR)) {
		syslog(LOG_ERR, "chdir: %s: %m", _PATH_MAILDIR);
		(void) recv(0, msgbuf, sizeof(msgbuf) - 1, 0);
		exit(1);
	}
	if ((uf = open(_PATH_UTMP, O_RDONLY, 0)) < 0) {
		syslog(LOG_ERR, "open: %s: %m", _PATH_UTMP);
		(void) recv(0, msgbuf, sizeof(msgbuf) - 1, 0);
		exit(1);
	}
	(void)time(&lastmsgtime);
	(void)gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	onalrm(0);
	(void)signal(SIGALRM, onalrm);
	(void)signal(SIGTTOU, SIG_IGN);
	(void)signal(SIGCHLD, reapchildren);
	for (;;) {
		cc = recv(0, msgbuf, sizeof(msgbuf) - 1, 0);
		if (cc <= 0) {
			if (errno != EINTR) {
				sleep(1);
			}
			errno = 0;
			continue;
		}
		if (!nutmp) {		/* no one has logged in yet */
			continue;
		}
		sigblock(sigmask(SIGALRM));
		msgbuf[cc] = '\0';
		(void)time(&lastmsgtime);
		mailfor(msgbuf);
		sigsetmask(0L);
	}
}

static void
reapchildren(int signo)
{
	while (wait3(NULL, WNOHANG, NULL) > 0);
}

static void
utmpx_read(struct utmpx *utpx)
{
	setutxent();
	utpx = getutxent();
	if (utpx == NULL) {
		syslog(LOG_ERR, "%s", strerror(errno));
		exit(1);
	}
}

static void
utmp_read(struct utmp *utp)
{
	static u_int utmpsize;		/* last malloced size for utmp */
	static u_int utmpmtime;		/* last modification time for utmp */
	struct stat statbf;

	(void)fstat(uf, &statbf);
	if (statbf.st_mtime > utmpmtime) {
		utmpmtime = statbf.st_mtime;
		if (statbf.st_size > utmpsize) {
			utmpsize = statbf.st_size + 10 * sizeof(struct utmp);
			utp = realloc(utp , utmpsize);
			if (utp == NULL) {
				syslog(LOG_ERR, "%s", strerror(errno));
				exit(1);
			}
		}
		(void)lseek(uf, (off_t)0, SEEK_SET);
		nutmp = read(uf, utp, (int)statbf.st_size)/sizeof(struct utmp);
	}
}

static void
onalrm(int signo)
{
	if (time(NULL) - lastmsgtime >= MAXIDLE) {
		exit(0);
	}
	(void)alarm((u_int)15);
	/* Read utmp */
	utmp_read(utmp);
	/* Read utmpx */
	utmpx_read(utmpx);
}

static void
utmp_notify(struct utmp *utp, char *name, off_t offset)
{
	while (utp != NULL) {
		if (!strcmp(utp->ut_name, name)) {
			notify(utp->ut_line, sizeof(utp->ut_line), utp->ut_name, sizeof(utp->ut_name), offset);
		}
	}
}

static void
utmpx_notify(struct utmpx *utpx, char *name, off_t offset)
{
	while (utpx != NULL) {
		if (!strcmp(utpx->ut_name, name)) {
			notify(utpx->ut_line, sizeof(utpx->ut_line), utpx->ut_name, sizeof(utpx->ut_name), offset);
		}
	}
	endutxent();
}

static void
mailfor(char *name)
{
	char *cp, *fn;
	off_t offset;
	intmax_t val;

	if (!(cp = strchr(name, '@'))) {
		return;
	}
	*cp = '\0';
	errno = 0;
	offset = val = strtoimax(cp + 1, &fn, 10);
	if (errno == ERANGE || offset != val) {
		return;
	}
	if (fn && *fn && *fn != '\n') {
		/*
		 * Procmail sends messages to comsat with a trailing colon
		 * and a pathname to the folder where the new message was
		 * deposited.  Since we can't reliably open only regular
		 * files, we need to ignore these.  With one exception:
		 * if it mentions the user's system mailbox.
		 */
		char maildir[MAXPATHLEN];
		int l = snprintf(maildir, sizeof(maildir), ":%s/%s",
		    _PATH_MAILDIR, name);
		if (l >= (int)sizeof(maildir) || strcmp(maildir, fn) != 0) {
			return;
		}
	}
	utmp_notify(utmp, name, offset);
	utmpx_notify(utmpx, name, offset);
}

static const char *cr;

#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)

static void
notify_sgtty(FILE *tp, struct sgttyb *gttybuf, char *tty)
{
	if (ioctl(fileno(tp), TIOCGETP, gttybuf) == -1) {
		dsyslog(LOG_ERR, "tcgetattr `%s' (%s)", tty, strerror(errno));
		_exit(1);
	}
	cr = (gttybuf->sg_flags & CRMOD) && !(gttybuf->sg_flags & RAW) ?
			"\n" : "\n\r";
}

#else

static void
notify_termios(FILE *tp, struct termios *ttybuf, char *tty)
{
	if (tcgetattr(fileno(tp), ttybuf) == -1) {
		dsyslog(LOG_ERR, "tcgetattr `%s' (%s)", tty, strerror(errno));
		_exit(1);
	}
	cr = (ttybuf->c_oflag & ONLCR) && (ttybuf->c_oflag & OPOST) ? "\n" : "\n\r";
}

#endif

static void
notify(const char *ut_line, size_t ut_linesize, const char *ut_name, size_t ut_namesize, off_t offset)
{
	FILE *tp;
	struct stat stb;
#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)
	struct sgttyb gttybuf;
#else
	struct termios ttybuf;
#endif
	char tty[20], name[ut_namesize + 1];
	const char *s;

	s = ut_line;
	if (strncmp(s, "pts/", 4) == 0) {
		s += 4;
	}
	if (strchr(s, '/')) {
		/* A slash is an attempt to break security... */
		syslog(LOG_AUTH | LOG_NOTICE, "Unexpected `/' in `%s'", ut_line);
		return;
	}
	(void)snprintf(tty, sizeof(tty), "%s%.*s",
	    _PATH_DEV, (int)ut_linesize, ut_line);
	if (strchr(tty + sizeof(_PATH_DEV) - 1, '/')) {
		/* A slash is an attempt to break security... */
		syslog(LOG_AUTH | LOG_NOTICE, "'/' in \"%s\"", tty);
		return;
	}
	if (stat(tty, &stb) || !(stb.st_mode & S_IEXEC)) {
		dsyslog(LOG_DEBUG, "%s: wrong mode on %s", ut_name, tty);
		return;
	}
	dsyslog(LOG_DEBUG, "notify %s on %s\n", ut_name, tty);
	if (fork())
		return;
	(void)signal(SIGALRM, SIG_DFL);
	(void)alarm((u_int)30);
	if ((tp = fopen(tty, "w")) == NULL) {
		dsyslog(LOG_ERR, "%s: %s", tty, strerror(errno));
		_exit(-1);
	}
#if defined(RUN_SGTTY) && (RUN_SGTTY == 0)
	notify_sgtty(tp, &gttybuf, tty);
#else
	notify_termios(tp, &ttybuf, tty);
#endif
	(void)strncpy(name, ut_name, ut_namesize);
	name[ut_namesize - 1] = '\0';
	(void)fprintf(tp, "%s\007New mail for %s@%.*s\007 has arrived:%s----%s",
	    cr, name, (int)sizeof(hostname), hostname, cr, cr);
	jkfprintf(tp, name, offset);
	(void)fclose(tp);
	_exit(0);
}

void
jkfprintf(FILE *tp, const char *name, off_t offset)
{
	register char *cp, ch;
	register FILE *fi;
	register int linecnt, charcnt, inheader;
	register struct passwd *p;
	char line[BUFSIZ];

	/* Set effective uid to user in case mail drop is on nfs */
	if ((p = getpwnam(name)) != NULL) {
		(void)setuid(p->pw_uid);
	}

	if ((fi = fopen(name, "r")) == NULL) {
		return;
	}

	(void)fseeko(fi, offset, SEEK_SET);
	/*
	 * Print the first 7 lines or 560 characters of the new mail
	 * (whichever comes first).  Skip header crap other than
	 * From, Subject, To, and Date.
	 */
	linecnt = 7;
	charcnt = 560;
	inheader = 1;
	while (fgets(line, sizeof(line), fi) != NULL) {
		if (inheader) {
			if (line[0] == '\n') {
				inheader = 0;
				continue;
			}
			if (line[0] == ' ' || line[0] == '\t' ||
					(strncmp(line, "From:", 5) &&
					strncmp(line, "Subject:", 8))) {
					continue;
			}
		}
		if (linecnt <= 0 || charcnt <= 0) {
			(void) fprintf(tp, "...more...%s", cr);
			(void) fclose(fi);
			return;
		}
		/* strip weird stuff so can't trojan horse stupid terminals */
		for (cp = line; (ch = *cp) && ch != '\n'; ++cp, --charcnt) {
			ch = toascii(ch);
			if (!isprint(ch) && !isspace(ch)) {
				ch |= 0x40;
			}
			(void)fputc(ch, tp);
		}
		(void) fputs(cr, tp);
		--linecnt;
	}
	(void) fprintf(tp, "----%s\n", cr);
	(void) fclose(fi);
}
