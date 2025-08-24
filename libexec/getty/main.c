/*	$NetBSD: main.c,v 1.68 2021/10/12 23:40:38 jmcneill Exp $	*/

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
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#if 0
static char sccsid[] = "@(#)main.c	5.5.1 (2.11BSD GTE) 12/9/94";
#endif
#endif

/*
 * getty -- adapt to terminal speed on dialup, and call login
 *
 * Melbourne getty, June 83, kre.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/file.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
//#include <sgtty.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <term.h>
#include <termios.h>
#include <time.h>
#include <ttyent.h>
#include <unistd.h>
#include <util.h>

#include "gettytab.h"
#include "pathnames.h"
#include "extern.h"

extern char **environ;

/*
 * Set the amount of running time that getty should accumulate
 * before deciding that something is wrong and exit.
 */
#define GETTY_TIMEOUT	60 /* seconds */

/* defines for auto detection of incoming PPP calls (->PAP/CHAP) */

#define PPP_FRAME           0x7e  /* PPP Framing character */
#define PPP_STATION         0xff  /* "All Station" character */
#define PPP_ESCAPE          0x7d  /* Escape Character */
#define PPP_CONTROL         0x03  /* PPP Control Field */
#define PPP_CONTROL_ESCAPED 0x23  /* PPP Control Field, escaped */
#define PPP_LCP_HI          0xc0  /* LCP protocol - high byte */
#define PPP_LCP_LOW         0x21  /* LCP protocol - low byte */

struct termios tmode, omode;

/*
struct sgttyb tmode = {
		.sg_ispeed = 0,
		.sg_ospeed = 0,
		.sg_erase = CERASE,
		.sg_kill = CKILL,
		.sg_flags = 0,
};
struct tchars tc = {
		.t_intrc = CINTR,
		.t_quitc = CQUIT,
		.t_startc = CSTART,
		.t_stopc = CSTOP,
		.t_eofc = CEOF,
		.t_brkc = CBRK,
};
struct ltchars ltc = {
		.t_suspc = CSUSP,
		.t_dsuspc = CDSUSP,
		.t_rprntc = CRPRNT,
		.t_flushc = CFLUSH,
		.t_werasc = CWERASE,
		.t_lnextc = CLNEXT,
};
*/

int	crmod;
int	upper;
int	lower;
int	digit;

char	hostname[MAXHOSTNAMELEN + 1];
char	name[LOGIN_NAME_MAX];
char	dev[] = _PATH_DEV;
char	ctty[] = _PATH_CONSOLE;
char	ttyn[32];
char	*rawttyn;

#define	OBUFSIZ		128
#define	TABBUFSIZ	512

char	defent[TABBUFSIZ];
char	defstrs[TABBUFSIZ];
char	tabent[TABBUFSIZ];
char	tabstrs[TABBUFSIZ];

char	*env[128];

char partab[] = {
		0001,0201,0201,0001,0201,0001,0001,0201,
		0202,0004,0003,0205,0005,0206,0201,0001,
		0201,0001,0001,0201,0001,0201,0201,0001,
		0001,0201,0201,0001,0201,0001,0001,0201,
		0200,0000,0000,0200,0000,0200,0200,0000,
		0000,0200,0200,0000,0200,0000,0000,0200,
		0000,0200,0200,0000,0200,0000,0000,0200,
		0200,0000,0000,0200,0000,0200,0200,0000,
		0200,0000,0000,0200,0000,0200,0200,0000,
		0000,0200,0200,0000,0200,0000,0000,0200,
		0000,0200,0200,0000,0200,0000,0000,0200,
		0200,0000,0000,0200,0000,0200,0200,0000,
		0000,0200,0200,0000,0200,0000,0000,0200,
		0200,0000,0000,0200,0000,0200,0200,0000,
		0200,0000,0000,0200,0000,0200,0200,0000,
		0000,0200,0200,0000,0200,0000,0000,0201
};

#define	ERASE	tmode.c_verase
#define	KILL	tmode.c_vkill
#define	EOT		tmode.c_veof

static void	clearscreen(void);

sigjmp_buf timeout;

static void
dingdong(int signo)
{

	(void)alarm(0);
	(void)signal(SIGALRM, SIG_DFL);
	siglongjmp(timeout, 1);
}

sigjmp_buf	intrupt;

static void
interrupt(int signo)
{

	(void)signal(SIGINT, interrupt);
	siglongjmp(intrupt, 1);
}

/*
 * Action to take when getty is running too long.
 */
static void
timeoverrun(int signo)
{
	syslog(LOG_ERR, "getty exiting due to excessive running time\n");
	exit(1);
}

static int	getname(void);
static void	oflush(void);
static void	prompt(void);
static int	putchr(int);
static void	putf(const char *);
static void	putpad(const char *);
static void	xputs(const char *);

int
main(int argc, char *argv[])
{
	register const char *tname;
	int repcnt = 0, failopenlogged = 0;
	volatile int first_time = 1;
	struct rlimit limit;
	int rval;

	(void)signal(SIGINT, SIG_IGN);
/*
	signal(SIGQUIT, SIG_DFL);
*/
	openlog("getty", LOG_PID|LOG_ODELAY|LOG_CONS, LOG_AUTH);
	(void)gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	if (hostname[0] == '\0') {
		(void)strlcpy(hostname, "Amnesiac", sizeof(hostname));
	}

	/*
	 * Limit running time to deal with broken or dead lines.
	 */
	(void)signal(SIGXCPU, timeoverrun);
	limit.rlim_max = RLIM_INFINITY;
	limit.rlim_cur = GETTY_TIMEOUT;
	(void)setrlimit(RLIMIT_CPU, &limit);
	/*
	 * The following is a work around for vhangup interactions
	 * which cause great problems getting window systems started.
	 * If the tty line is "-", we do the old style getty presuming
	 * that the file descriptors are already set up for us. 
	 * J. Gettys - MIT Project Athena.
	 */
	if (argc <= 2 || strcmp(argv[2], "-") == 0) {
		(void)strlcpy(ttyn, ttyname(0), sizeof(ttyn));
	} else {
		int i;

		rawttyn = argv[2];
		(void)strlcpy(ttyn, dev, sizeof(ttyn));
		(void)strlcat(ttyn, argv[2], sizeof(ttyn));
		if (strcmp(argv[0], "+") != 0) {
			chown(ttyn, 0, 0);
			chmod(ttyn, 0600);
			revoke(ttyn);
			/*
			 * Delay the open so DTR stays down long enough to be detected.
			 */
			sleep(2);
			if (ttyaction(ttyn, "getty", "root")) {
				syslog(LOG_WARNING, "%s: ttyaction failed", ttyn);
			}
			while ((i = open(ttyn, O_RDWR)) == -1) {
				if ((repcnt % 10 == 0) && (errno != ENXIO || !failopenlogged)) {
					syslog(LOG_ERR, "%s: %m", ttyn);
					closelog();
					failopenlogged = 1;
				}
				repcnt++;
				(void)sleep(60);
			}
			(void)login_tty(i);
		}
	}

	/* Start with default tty settings */
	if (tcgetattr(0, &tmode) < 0) {
		syslog(LOG_ERR, "%s: %m", ttyn);
		exit(1);
	}
	omode = tmode;

	gettable("default", defent, defstrs);
	gendefaults();
	tname = "default";
	if (argc > 1)
		tname = argv[1];
	for (;;) {
		int off;

		rval = 0;
		gettable(tname, tabent, tabstrs);
		if (OPset || EPset || APset)
			APset++, OPset++, EPset++;
		setdefaults();
		off = 0;
		(void)tcflush(0, TCIOFLUSH);	/* clear out the crap */
		(void)ioctl(0, FIONBIO, &off);	/* turn off non-blocking mode */
		(void)ioctl(0, FIOASYNC, &off);	/* ditto for async mode */
		if (IS)
			(void)cfsetispeed(&tmode, (speed_t)IS);
		else if (SP)
			(void)cfsetispeed(&tmode, (speed_t)SP);
		if (OS)
			(void)cfsetospeed(&tmode, (speed_t)OS);
		else if (SP)
			(void)cfsetospeed(&tmode, (speed_t)SP);
		setflags(0);
		setchars();
		if (tcsetattr(0, TCSANOW, &tmode) < 0) {
			syslog(LOG_ERR, "%s: %m", ttyn);
			exit(1);
		}
		if (AB) {
			tname = autobaud();
			continue;
		}
		if (PS) {
			tname = portselector();
			continue;
		}
		if (CS) {
			clearscreen();
		}
		if (CL && *CL) {
			putpad(CL);
		}
		edithost(HE);

		/*
		 * If this is the first time through this, and an
		 * issue file has been given, then send it.
		 */
		if (first_time != 0 && IF != NULL) {
			char buf[_POSIX2_LINE_MAX];
			FILE *fp;

			if ((fp = fopen(IF, "r")) != NULL) {
				while (fgets(buf, sizeof(buf) - 1, fp) != NULL)
					putf(buf);
				(void)fclose(fp);
			}
		}
		first_time = 0;

		if (IM && *IM) {
			putf(IM);
		}
		oflush();
		if (sigsetjmp(timeout, 1)) {
			tmode.c_ispeed = 0;
			tmode.c_ospeed = 0;
			(void)tcsetattr(0, TCSANOW, &tmode);
			exit(1);
		}
		if (TO) {
			(void)signal(SIGALRM, dingdong);
			(void)alarm((int)TO);
		}
		if (NN) {
			name[0] = '\0';
			lower = 1;
			upper = 0;
			digit = 0;
		} else if (AL) {
			const char *p = AL;
			char *q = name;

			while (*p && q < &name[sizeof name - 1]) {
				if (isupper((unsigned char)*p)) {
					upper = 1;
				} else if (islower((unsigned char)*p)) {
					lower = 1;
				} else if (isdigit((unsigned char)*p)) {
					digit = 1;
				}
				*q++ = *p++;
			}
		} else if ((rval = getname()) == 2) {
			setflags(2);
			(void)execle(PP, "ppplogin", ttyn, (char *) 0, env);
			syslog(LOG_ERR, "%s: %m", PP);
			exit(1);
		}

		if (rval || AL|| NN) {
			register int i;

			oflush();
			(void)alarm(0);
			(void)signal(SIGALRM, SIG_DFL);
			if (name[0] == '-') {
				xputs("user names may not start with '-'.");
				continue;
			}
			if (!(upper || lower || digit)) {
				continue;
			}
			setflags(2);
			if (crmod) {
				tmode.c_iflag |= ICRNL;
				tmode.c_oflag |= ONLCR;
			}
#if XXX
			if (upper || UC)
				tmode.sg_flags |= LCASE;
			if (lower || LC)
				tmode.sg_flags &= ~LCASE;
#endif
			if (tcsetattr(0, TCSANOW, &tmode) < 0) {
				syslog(LOG_ERR, "%s: %m", ttyn);
				exit(1);
			}
			(void)signal(SIGINT, SIG_DFL);
			for (i = 0; environ[i] != (char *)0; i++)
				env[i] = environ[i];
			makeenv(&env[i]);
			limit.rlim_max = RLIM_INFINITY;
			limit.rlim_cur = RLIM_INFINITY;
			(void)setrlimit(RLIMIT_CPU, &limit);
			if (NN)
				(void)execle(LO, "login", AL ? "-fp" : "-p",
				    (char *) 0, env);
			else
				(void)execle(LO, "login", AL ? "-fp" : "-p",
				    "--", name, (char *) 0, env);
			syslog(LOG_ERR, "%s: %m", LO);
			exit(1);
		}
		(void)alarm(0);
		(void)signal(SIGALRM, SIG_DFL);
		(void)signal(SIGINT, SIG_IGN);
		if (NX && *NX) {
			tname = NX;
		}
	}
}

static int
getname(void)
{
	register char *np;
	register int c;
	char cs;
	int ppp_state, ppp_connection;

	/*
	 * Interrupt may happen if we use CBREAK mode
	 */
	if (sigsetjmp(intrupt, 1)) {
		(void)signal(SIGINT, SIG_IGN);
		return (0);
	}
	(void)signal(SIGINT, interrupt);
	setflags(1);
	prompt();
	if (PF > 0) {
		oflush();
		(void)sleep((int)PF);
		PF = 0;
	}
	if (tcsetattr(0, TCSANOW, &tmode) < 0) {
		syslog(LOG_ERR, "%s: %m", ttyn);
		exit(1);
	}
	crmod = 0;
	upper = 0;
	lower = 0;
	digit = 0;
	ppp_state = 0;
	ppp_connection = 0;
	np = name;
	for (;;) {
		oflush();
		if (read(STDIN_FILENO, &cs, 1) <= 0)
			exit(0);
		if ((c = cs&0177) == 0)
			return (0);

		/*
		 * PPP detection state machine..
		 * Look for sequences:
		 * PPP_FRAME, PPP_STATION, PPP_ESCAPE, PPP_CONTROL_ESCAPED or
		 * PPP_FRAME, PPP_STATION, PPP_CONTROL (deviant from RFC)
		 * See RFC1662.
		 * Derived from code from Michael Hancock <michaelh@cet.co.jp>
		 * and Erik 'PPP' Olson <eriko@wrq.com>
		 */
		if (PP && cs == PPP_FRAME) {
			ppp_state = 1;
		} else if (ppp_state == 1 && cs == PPP_STATION) {
			ppp_state = 2;
		} else if (ppp_state == 2 && cs == PPP_ESCAPE) {
			ppp_state = 3;
		} else if ((ppp_state == 2 && cs == PPP_CONTROL) ||
		    (ppp_state == 3 && cs == PPP_CONTROL_ESCAPED)) {
			ppp_state = 4;
		} else if (ppp_state == 4 && cs == PPP_LCP_HI) {
			ppp_state = 5;
		} else if (ppp_state == 5 && cs == PPP_LCP_LOW) {
			ppp_connection = 1;
			break;
		} else {
			ppp_state = 0;
		}

		if (c == EOT)
			exit(1);
		if (c == '\r' || c == '\n' || np >= &name[LOGIN_NAME_MAX - 1]) {
			*np = '\0';
			putf("\r\n");
			break;
		}
		if (islower(c))
			lower++;
		else if (isupper(c))
			upper++;
		else if (c == ERASE || c == '#' || c == '\b') {
			if (np > name) {
				np--;
				if (cfgetospeed(&tmode) >= 1200)
					xputs("\b \b");
				else
					putchr(cs);
			}
			continue;
		} else if (c == KILL || c == '@') {
			putchr(cs);
			putchr('\r');
			if (cfgetospeed(&tmode) < 1200)
				putchr('\n');
			/* this is the way they do it down under ... */
			else if (np > name)
				xputs("                                     \r");
			prompt();
			np = name;
			continue;
		} else if (isdigit(c) || c == '_')
			digit = 1;
		if (IG && (c <= ' ' || c > 0176))
			continue;
		*np++ = c;
		putchr(cs);

		/*
		 * An MS-Windows direct connect PPP "client" won't send its
		 * first PPP packet until we respond to its "CLIENT" poll
		 * with a CRLF sequence.  We cater to yet another broken
		 * implementation of a previously-standard protocol...
		 */
		*np = '\0';
		if (strstr(name, "CLIENT"))
			putf("\r\n");
	}
	(void)signal(SIGINT, SIG_IGN);
	*np = 0;
	if (c == '\r')
		crmod = 1;
	if ((upper && !lower && !LC) || UC)
		for (np = name; *np; np++)
			*np = tolower((unsigned char)*np);
	return (1 + ppp_connection);
}

static void
putpad(const char *s)
{
	tputs(s, 1, putchr);
}

#ifdef OLD_GETTY
static short tmspc10[] = {
		0, 2000, 1333, 909, 743, 666, 500, 333, 166, 83, 55, 41, 20, 10, 5, 15
};

static void
putpad(char *s)
{
	register int pad = 0;
	register int mspc10;

	if (isdigit(*s)) {
		while (isdigit(*s)) {
			pad *= 10;
			pad += *s++ - '0';
		}
		pad *= 10;
		if (*s == '.' && isdigit(s[1])) {
			pad += s[1] - '0';
			s += 2;
		}
	}

	xputs(s);
	/*
	 * If no delay needed, or output speed is
	 * not comprehensible, then don't try to delay.
	 */
	if (pad == 0)
		return;
	if (tmode.sg_ospeed <= 0 ||
	    tmode.sg_ospeed >= (sizeof tmspc10 / sizeof tmspc10[0]))
		return;

	/*
	 * Round up by a half a character frame,
	 * and then do the delay.
	 * Too bad there are no user program accessible programmed delays.
	 * Transmitting pad characters slows many
	 * terminals down and also loads the system.
	 */
	mspc10 = tmspc10[(short)tmode.sg_ospeed];
	pad += mspc10 / 2;
	for (pad /= mspc10; pad > 0; pad--)
		putchr(*PC);
}
#endif

static void
xputs(const char *s)
{

	while (*s)
		putchr(*s++);
}

char outbuf[OBUFSIZ];
int	obufcnt = 0;

static int
putchr(int cc)
{
	char c;
    int rval;
    
    rval = 0;
	c = cc;
	c |= partab[c&0177] & 0200;
	if (OP)
		c ^= 0200;
	if (!UB) {
		outbuf[obufcnt++] = c;
		if (obufcnt >= OBUFSIZ) {
			oflush();
		}
        rval = 1;
	} else {
		rval = write(STDOUT_FILENO, &c, 1);
	}
    return (rval);
}

static void
oflush(void)
{
	if (obufcnt) {
		(void)write(STDOUT_FILENO, outbuf, obufcnt);
	}
	obufcnt = 0;
}

static void
prompt(void)
{

	putf(LM);
	if (CO)
		putchr('\n');
}

static void
putf(const char *cp)
{
	char *ttyns, *slash;
	char datebuffer[60];
	extern char editedhost[];

	while (*cp) {
		if (*cp != '%') {
			putchr(*cp++);
			continue;
		}
		switch (*++cp) {

		case 't':
			ttyns = ttyname(0);
			slash = rindex(ttyn, '/');
			if (slash == (char *) 0)
				xputs(ttyns);
			else
				xputs(&slash[1]);
			break;

		case 'h':
			xputs(editedhost);
			break;

		case 'd':
			get_date(datebuffer);
			xputs(datebuffer);
			break;

		case '%':
			putchr('%');
			break;
		}
		if (*cp) {
			cp++;
		}
	}
}

static void
clearscreen(void)
{
	struct ttyent *typ;
	int err;

	if (rawttyn == NULL)
		return;

	typ = getttynam(rawttyn);

	if ((typ == NULL) || (typ->ty_type == NULL) ||
	    (typ->ty_type[0] == 0))
		return;

	if (setupterm(typ->ty_type, 0, &err) == ERR)
		return;

	if (clear_screen)
		putpad(clear_screen);

	del_curterm(cur_term);
	cur_term = NULL;
}
