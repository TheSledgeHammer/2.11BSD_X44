/*-
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

#ifndef lint
static char sccsid[] = "@(#)dumprmt.c	8.3 (Berkeley) 4/28/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <protocols/dumprestor.h>

#include <ctype.h>
#include <err.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"
#include "dump.h"

#define	TS_CLOSED	0
#define	TS_OPEN		1

static	int rmtstate = TS_CLOSED;
static	int rmtape;
static	char *rmtpeer;

static	int okname(char *);
static	int rmtcall(char *, char *);
static	void rmtconnaborted(/* int, int */);
static	int rmtgetb(void);
static	void rmtgetconn(void);
static	void rmtgets(char *, int);
static	int rmtreply(char *);

extern	int ntrec;		/* blocking factor on tape */

int
rmthost(host)
	char *host;
{

	rmtpeer = malloc(strlen(host) + 1);
	if (rmtpeer)
		strcpy(rmtpeer, host);
	else
		rmtpeer = host;
	signal(SIGPIPE, rmtconnaborted);
	rmtgetconn();
	if (rmtape < 0)
		return (0);
	return (1);
}

static void
rmtconnaborted()
{

	errx(1, "Lost connection to remote host.");
}

void
rmtgetconn()
{
	register char *cp;
	static struct servent *sp = NULL;
	static struct passwd *pwd = NULL;
#ifdef notdef
	static int on = 1;
#endif
	char *tuser;
	int size;
	int maxseg;

	if (sp == NULL) {
		sp = getservbyname("shell", "tcp");
		if (sp == NULL)
			errx(1, "shell/tcp: unknown service");
		pwd = getpwuid(getuid());
		if (pwd == NULL)
			errx(1, "who are you?");
	}
	if ((cp = strchr(rmtpeer, '@')) != NULL) {
		tuser = rmtpeer;
		*cp = '\0';
		if (!okname(tuser))
			exit(1);
		rmtpeer = ++cp;
	} else
		tuser = pwd->pw_name;
	rmtape = rcmd(&rmtpeer, (u_short)sp->s_port, pwd->pw_name, tuser,
	    _PATH_RMT, (int *)0);
	size = ntrec * TP_BSIZE;
	if (size > 60 * 1024)		/* XXX */
		size = 60 * 1024;
	/* Leave some space for rmt request/response protocol */
	size += 2 * 1024;
	while (size > TP_BSIZE &&
	    setsockopt(rmtape, SOL_SOCKET, SO_SNDBUF, &size, sizeof (size)) < 0)
		    size -= TP_BSIZE;
	(void)setsockopt(rmtape, SOL_SOCKET, SO_RCVBUF, &size, sizeof (size));
	maxseg = 1024;
	if (setsockopt(rmtape, IPPROTO_TCP, TCP_MAXSEG,
	    &maxseg, sizeof (maxseg)) < 0)
		perror("TCP_MAXSEG setsockopt");

#ifdef notdef
	if (setsockopt(rmtape, IPPROTO_TCP, TCP_NODELAY, &on, sizeof (on)) < 0)
		perror("TCP_NODELAY setsockopt");
#endif
}

static int
okname(cp0)
	char *cp0;
{
	register char *cp;
	register int c;

	for (cp = cp0; *cp; cp++) {
		c = *cp;
		if (!isascii(c) || !(isalnum(c) || c == '_' || c == '-')) {
			warnx("invalid user name: %s", cp0);
			return (0);
		}
	}
	return (1);
}

int
rmtopen(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

	(void)sprintf(buf, "O%s\n%d\n", tape, mode);
	rmtstate = TS_OPEN;
	return (rmtcall(tape, buf));
}

void
rmtclose()
{

	if (rmtstate != TS_OPEN)
		return;
	rmtcall("close", "C\n");
	rmtstate = TS_CLOSED;
}

int
rmtread(buf, count)
	char *buf;
	int count;
{
	char line[30];
	int n, i, cc;
	extern errno;

	(void)sprintf(line, "R%d\n", count);
	n = rmtcall("read", line);
	if (n < 0) {
		errno = n;
		return (-1);
	}
	for (i = 0; i < n; i += cc) {
		cc = read(rmtape, buf+i, n - i);
		if (cc <= 0) {
			rmtconnaborted();
		}
	}
	return (n);
}

int
rmtwrite(buf, count)
	char *buf;
	int count;
{
	char line[30];

	(void)sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
	write(rmtape, buf, count);
	return (rmtreply("write"));
}

void
rmtwrite0(count)
	int count;
{
	char line[30];

	(void)sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
}

void
rmtwrite1(buf, count)
	char *buf;
	int count;
{

	write(rmtape, buf, count);
}

int
rmtwrite2()
{

	return (rmtreply("write"));
}

int
rmtseek(offset, pos)
	int offset, pos;
{
	char line[80];

	(void)sprintf(line, "L%d\n%d\n", offset, pos);
	return (rmtcall("seek", line));
}

struct	mtget mts;

struct mtget *
rmtstatus()
{
	register int i;
	register char *cp;

	if (rmtstate != TS_OPEN)
		return (NULL);
	rmtcall("status", "S\n");
	for (i = 0, cp = (char *)&mts; i < sizeof(mts); i++)
		*cp++ = rmtgetb();
	return (&mts);
}

int
rmtioctl(cmd, count)
	int cmd, count;
{
	char buf[256];

	if (count < 0)
		return (-1);
	(void)sprintf(buf, "I%d\n%d\n", cmd, count);
	return (rmtcall("ioctl", buf));
}

static int
rmtcall(cmd, buf)
	char *cmd, *buf;
{

	if (write(rmtape, buf, strlen(buf)) != strlen(buf))
		rmtconnaborted();
	return (rmtreply(cmd));
}

static int
rmtreply(cmd)
	char *cmd;
{
	register char *cp;
	char code[30], emsg[BUFSIZ];

	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		msg("%s: %s", cmd, emsg);
		if (*code == 'F') {
			rmtstate = TS_CLOSED;
			return (-1);
		}
		return (-1);
	}
	if (*code != 'A') {
		/* Kill trailing newline */
		cp = code + strlen(code);
		if (cp > code && *--cp == '\n')
			*cp = '\0';

		msg("Protocol to remote tape server botched (code \"%s\").\n",
		    code);
		rmtconnaborted();
	}
	return (atoi(code + 1));
}

int
rmtgetb()
{
	char c;

	if (read(rmtape, &c, 1) != 1)
		rmtconnaborted();
	return (c);
}

/* Get a line (guaranteed to have a trailing newline). */
void
rmtgets(line, len)
	char *line;
	int len;
{
	register char *cp = line;

	while (len > 1) {
		*cp = rmtgetb();
		if (*cp == '\n') {
			cp[1] = '\0';
			return;
		}
		cp++;
		len--;
	}
	*cp = '\0';
	msg("Protocol to remote tape server botched.\n");
	msg("(rmtgets got \"%s\").\n", line);
	rmtconnaborted();
}
