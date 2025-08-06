/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if	!defined(lint) && defined(DOSCCS)
#if 0
static char sccsid[] = "@(#)subr.c	5.4.2 (2.11BSD GTE) 1997/3/28";
#endif
#endif

#include <sys/termios.h>

/*
 * Melbourne getty.
 */
#include <sgtty.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <select.h>

#include "gettytab.h"
#include "extern.h"
#include "pathnames.h"

extern	struct sgttyb tmode;
extern	struct tchars tc;
extern	struct ltchars ltc;

/*
 * Get a table entry.
 */
void
gettable(char *name, char *buf, char *area)
{
	register struct gettystrs *sp;
	register struct gettynums *np;
	register struct gettyflags *fp;
	register n;

	hopcount = 0;		/* new lookup, start fresh */
	if (getent(buf, name) != 1)
		return;

	for (sp = gettystrs; sp->field; sp++) {
		sp->value = getstr(buf, sp->field, &area);
	}
	for (np = gettynums; np->field; np++) {
		n = getnum(buf, np->field);
		if (n == -1) {
			np->set = 0;
		} else {
			np->set = 1;
			np->value = n;
		}
	}
	for (fp = gettyflags; fp->field; fp++) {
		n = getflag(buf, fp->field);
		if (n == -1) {
			fp->set = 0;
		} else {
			fp->set = 1;
			fp->value = n ^ fp->invrt;
		}
	}
}

void
gendefaults(void)
{
	register struct gettystrs *sp;
	register struct gettynums *np;
	register struct gettyflags *fp;

	for (sp = gettystrs; sp->field; sp++)
		if (sp->value)
			sp->defalt = sp->value;
	for (np = gettynums; np->field; np++)
		if (np->set)
			np->defalt = np->value;
	for (fp = gettyflags; fp->field; fp++)
		if (fp->set)
			fp->defalt = fp->value;
		else
			fp->defalt = fp->invrt;
}

void
setdefaults(void)
{
	register struct gettystrs *sp;
	register struct gettynums *np;
	register struct gettyflags *fp;

	for (sp = gettystrs; sp->field; sp++)
		if (!sp->value)
			sp->value = sp->defalt;
	for (np = gettynums; np->field; np++)
		if (!np->set)
			np->value = np->defalt;
	for (fp = gettyflags; fp->field; fp++)
		if (!fp->set)
			fp->value = fp->defalt;
}

static char **charnames[] = {
		&ER, &KL, &IN, &QU, &XN, &XF, &ET, &BK,
		&SU, &DS, &RP, &FL, &WE, &LN, 0
};

static char *charvars[] = {
		&tmode.sg_erase, &tmode.sg_kill, &tc.t_intrc,
		&tc.t_quitc, &tc.t_startc, &tc.t_stopc,
		&tc.t_eofc, &tc.t_brkc, &ltc.t_suspc,
		&ltc.t_dsuspc, &ltc.t_rprntc, &ltc.t_flushc,
		&ltc.t_werasc, &ltc.t_lnextc, 0
};
/*
static char *charvars[] = {
		&tmode.c_cc[VERASE], &tmode.c_cc[VKILL], &tmode.c_cc[VINTR],
		&tmode.c_cc[VQUIT], &tmode.c_cc[VSTART], &tmode.c_cc[VSTOP],
		&tmode.c_cc[VEOF], &tmode.c_cc[VEOL], &tmode.c_cc[VSUSP],
		&tmode.c_cc[VDSUSP], &tmode.c_cc[VREPRINT], &tmode.c_cc[VDISCARD],
		&tmode.c_cc[VWERASE], &tmode.c_cc[VLNEXT], 0
};
*/
void
setchars(void)
{
	register int i;
	register char *p;

	for (i = 0; charnames[i]; i++) {
		p = *charnames[i];
		if (p && *p)
			*charvars[i] = *p;
		else
			*charvars[i] = '\377';
	}
}

long
setflags(int n)
{
	register long f;

	switch (n) {
	case 0:
		if (F0set)
			return(F0);
		break;
	case 1:
		if (F1set)
			return(F1);
		break;
	default:
		if (F2set)
			return(F2);
		break;
	}

	f = 0;

	if (AP)
		f |= ANYP;
	else if (OP)
		f |= ODDP;
	else if (EP)
		f |= EVENP;
	if (HF)
		f |= RTSCTS;
	if (NL)
		f |= CRMOD;

	if (n == 1) {		/* read mode flags */
		if (RW)
			f |= RAW;
		else
			f |= CBREAK;
		return (f);
	}

	if (!HT)
		f |= XTABS;
	if (n == 0)
		return (f);
	if (CB)
		f |= CRTBS;
	if (CE)
		f |= CRTERA;
	if (CK)
		f |= CRTKIL;
	if (PE)
		f |= PRTERA;
	if (EC)
		f |= ECHO;
	if (XC)
		f |= CTLECH;
	if (DX)
		f |= DECCTQ;
	return (f);
}

char	editedhost[32];

void
edithost(char *pat)
{
	register char *host = HN;
	register char *res = editedhost;

	if (!pat)
		pat = "";
	while (*pat) {
		switch (*pat) {

		case '#':
			if (*host)
				host++;
			break;

		case '@':
			if (*host)
				*res++ = *host++;
			break;

		default:
			*res++ = *pat;
			break;

		}
		if (res == &editedhost[sizeof editedhost - 1]) {
			*res = '\0';
			return;
		}
		pat++;
	}
	if (*host)
		strncpy(res, host, sizeof editedhost - (res - editedhost) - 1);
	else
		*res = '\0';
	editedhost[sizeof editedhost - 1] = '\0';
}

struct speedtab {
	int	speed;
	int	uxname;
} speedtab[] = {
	{ 50,	  B50 },
	{ 75,	  B75 },
	{ 110,	 B110 },
	{ 134,	 B134 },
	{ 150,	 B150 },
	{ 200,	 B200 },
	{ 300,	 B300 },
	{ 600,	 B600 },
	{ 1200,	B1200 },
	{ 1800,	B1800 },
	{ 2400,	B2400 },
	{ 4800,	B4800 },
	{ 9600,	B9600 },
	{ 19200, EXTA },
	{ 19,	 EXTA },	/* for people who say 19.2K */
	{ 38400, EXTB },
	{ 38,	 EXTB },
	{ 7200,	 EXTB },	/* alternative */
	{ 0 }
};

int
speed(long val)
{
	register struct speedtab *sp;

	if (val <= 15)
		return (val);

	for (sp = speedtab; sp->speed; sp++)
		if (sp->speed == val)
			return (sp->uxname);
	
	return (B300);		/* default in impossible cases */
}

void
makeenv(char *env[])
{
	static char termbuf[128] = "TERM=";
	register char *p, *q;
	register char **ep;

	ep = env;
	if (TT && *TT) {
		strcat(termbuf, TT);
		*ep++ = termbuf;
	}
	if ((p = EV)) {
		q = p;
		while ((q = index(q, ','))) {
			*q++ = '\0';
			*ep++ = p;
			p = q;
		}
		if (*p)
			*ep++ = p;
	}
	*ep = (char *)0;
}

/*
 * This speed select mechanism is written for the Develcon DATASWITCH.
 * The Develcon sends a string of the form "B{speed}\n" at a predefined
 * baud rate. This string indicates the user's actual speed.
 * The routine below returns the terminal type mapped from derived speed.
 */
struct	portselect {
	const char	*ps_baud;
	const char	*ps_type;
} portspeeds[] = {
	{ "B110",	"std.110" },
	{ "B134",	"std.134" },
	{ "B150",	"std.150" },
	{ "B300",	"std.300" },
	{ "B600",	"std.600" },
	{ "B1200",	"std.1200" },
	{ "B2400",	"std.2400" },
	{ "B4800",	"std.4800" },
	{ "B9600",	"std.9600" },
	{ "B19200",	"std.19200" },
	{ 0 }
};

const char *
portselector(void)
{
	char c, baud[20];
	const char *type = "default";
	register struct portselect *ps;
	int len;

	alarm(5*60);
	for (len = 0; len < sizeof (baud) - 1; len++) {
		if (read(0, &c, 1) <= 0)
			break;
		c &= 0177;
		if (c == '\n' || c == '\r')
			break;
		if (c == 'B')
			len = 0;	/* in case of leading garbage */
		baud[len] = c;
	}
	baud[len] = '\0';
	for (ps = portspeeds; ps->ps_baud; ps++)
		if (strcmp(ps->ps_baud, baud) == 0) {
			type = ps->ps_type;
			break;
		}
	sleep(2);	/* wait for connection to complete */
	return (type);
}

/*
 * This auto-baud speed select mechanism is written for the Micom 600
 * portselector. Selection is done by looking at how the character '\r'
 * is garbled at the different speeds.
 */
#include <sys/time.h>

const char *
autobaud(void)
{
	long rfds;
	struct timeval timeout;
	char c;
	const char *type = "1200-baud";
	int null = 0;

	ioctl(0, TIOCFLUSH, &null);
	rfds = 1 << 0;
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;
	if (select(32, &rfds, (int *)0, (int *)0, &timeout) <= 0)
		return (type);
	if (read(0, &c, sizeof(char)) != sizeof(char))
		return (type);
	timeout.tv_sec = 0;
	timeout.tv_usec = 20;
	(void) select(32, (int *)0, (int *)0, (int *)0, &timeout);
	ioctl(0, TIOCFLUSH, &null);
	switch (c & 0377) {

	case 0200:		/* 300-baud */
		type = "300-baud";
		break;

	case 0346:		/* 1200-baud */
		type = "1200-baud";
		break;

	case  015:		/* 2400-baud */
	case 0215:
		type = "2400-baud";
		break;

	default:		/* 4800-baud */
		type = "4800-baud";
		break;

	case 0377:		/* 9600-baud */
		type = "9600-baud";
		break;
	}
	return (type);
}
