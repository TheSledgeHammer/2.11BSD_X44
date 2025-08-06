/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)gettytab.c	5.1 (Berkeley) 4/29/85";
#endif
#endif /* not lint */

#include <sys/termios.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gettytab.h"
#include "pathnames.h"
#include "extern.h"

int
getent(char *bp, const char *name)
{
	const char *dba[2];
	dba[0] = _PATH_GETTYTAB;
	dba[1] = NULL;

	return (cgetent(&bp, dba, name));
}

long
getnum(char *bp, const char *id)
{
	long num;

	return ((long)cgetnum(&bp, id, &num));
}

char *
getstr(char *bp, const char *id, char **area)
{
	char *buf;
	int rval;

	rval = cgetstr(&bp, id, area);
	if (rval >= 0) {
		buf = bp;
	} else {
		buf = NULL;
	}
	return (buf);
}

int
getflag(char *bp, const char *id)
{
	char *buf;
	int rval;

	buf = cgetcap(&bp, id, ':');
	if (!buf || buf == ':') {
		rval = 1;
	} else if (buf == '!') {
		rval = 0;
	} else if (buf == '@') {
		rval = -1;
	}
	return (rval);
}

void
set_ttydefaults(int fd)
{
	struct termios term;

	tcgetattr(fd, &term);
	term.c_iflag = TTYDEF_IFLAG;
	term.c_oflag = TTYDEF_OFLAG;
	term.c_lflag = TTYDEF_LFLAG;
	term.c_cflag = TTYDEF_CFLAG;
	tcsetattr(fd, TCSAFLUSH, &term);
}

#ifdef OLD_TTYENT

#define	TABBUFSIZ	512

static	char *tbuf;
int	hopcount;	/* detect infinite loops in termcap, init 0 */

int nchktc(void);
int namatch(char *np);
static char *skip(char *bp);
static char *decode(char *str, char **area);

/*
 * Get an entry for terminal name in buffer bp,
 * from the termcap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
int
getent(char *bp, char *name)
{
	register char *cp;
	register int c;
	register int i = 0, cnt = 0;
	char ibuf[TABBUFSIZ];
	char *cp2;
	int tf;

	tbuf = bp;
	tf = open(_PATH_GETTYTAB, 0);
	if (tf < 0)
		return (-1);
	for (;;) {
		cp = bp;
		for (;;) {
			if (i == cnt) {
				cnt = read(tf, ibuf, TABBUFSIZ);
				if (cnt <= 0) {
					close(tf);
					return (0);
				}
				i = 0;
			}
			c = ibuf[i++];
			if (c == '\n') {
				if (cp > bp && cp[-1] == '\\'){
					cp--;
					continue;
				}
				break;
			}
			if (cp >= bp+TABBUFSIZ) {
				write(2,"Gettytab entry too long\n", 24);
				break;
			} else
				*cp++ = c;
		}
		*cp = 0;

		/*
		 * The real work for the match.
		 */
		if (namatch(name)) {
			close(tf);
			return (nchktc());
		}
	}
}

/*
 * tnchktc: check the last entry, see if it's tc=xxx. If so,
 * recursively find xxx and append that entry (minus the names)
 * to take the place of the tc=xxx entry. This allows termcap
 * entries to say "like an HP2621 but doesn't turn on the labels".
 * Note that this works because of the left to right scan.
 */
#define	MAXHOP	32

int
nchktc(void)
{
	register char *p, *q;
	char tcname[16];	/* name of similar terminal */
	char tcbuf[TABBUFSIZ];
	char *holdtbuf = tbuf;
	int l;

	p = tbuf + strlen(tbuf) - 2;	/* before the last colon */
	while (*--p != ':')
		if (p<tbuf) {
			write(2, "Bad gettytab entry\n", 19);
			return (0);
		}
	p++;
	/* p now points to beginning of last field */
	if (p[0] != 't' || p[1] != 'c')
		return(1);
	strcpy(tcname,p+3);
	q = tcname;
	while (q && *q != ':')
		q++;
	*q = 0;
	if (++hopcount > MAXHOP) {
		write(2, "Getty: infinite tc= loop\n", 25);
		return (0);
	}
	if (getent(tcbuf, tcname) != 1)
		return(0);
	for (q=tcbuf; *q != ':'; q++)
		;
	l = p - holdtbuf + strlen(q);
	if (l > TABBUFSIZ) {
		write(2, "Gettytab entry too long\n", 24);
		q[TABBUFSIZ - (p-tbuf)] = 0;
	}
	strcpy(p, q+1);
	tbuf = holdtbuf;
	return (1);
}

/*
 * Tnamatch deals with name matching.  The first field of the termcap
 * entry is a sequence of names separated by |'s, so we compare
 * against each such name.  The normal : terminator after the last
 * name (before the first field) stops us.
 */
int
namatch(char *np)
{
	register char *Np, *Bp;

	Bp = tbuf;
	if (*Bp == '#')
		return(0);
	for (;;) {
		for (Np = np; *Np && *Bp == *Np; Bp++, Np++)
			continue;
		if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0))
			return (1);
		while (*Bp && *Bp != ':' && *Bp != '|')
			Bp++;
		if (*Bp == 0 || *Bp == ':')
			return (0);
		Bp++;
	}
}

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the termcap file in octal.
 */
static char *
skip(char *bp)
{

	while (*bp && *bp != ':')
		bp++;
	while (*bp == ':')
		bp++;
	return (bp);
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
long
getnum(char *id)
{
	register long i, base;
	register char *bp = tbuf;

	for (;;) {
		bp = skip(bp);
		if (*bp == 0)
			return (-1);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return(-1);
		if (*bp != '#')
			continue;
		bp++;
		base = 10;
		if (*bp == '0')
			base = 8;
		i = 0;
		while (isdigit(*bp))
			i *= base, i += *bp++ - '0';
		return (i);
	}
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
int
getflag(char *id)
{
	register char *bp = tbuf;

	for (;;) {
		bp = skip(bp);
		if (!*bp)
			return (-1);
		if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
			if (!*bp || *bp == ':')
				return (1);
			else if (*bp == '!')
				return (0);
			else if (*bp == '@')
				return(-1);
		}
	}
}

/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
char *
getstr(char *id, char **area)
{
	register char *bp = tbuf;

	for (;;) {
		bp = skip(bp);
		if (!*bp)
			return (0);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return(0);
		if (*bp != '=')
			continue;
		bp++;
		return (decode(bp, area));
	}
}

/*
 * Tdecode does the grung work to decode the
 * string capability escapes.
 */
static char *
decode(char *str, char **area)
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = *area;
	while ((c = *str++) && c != ':') {
		switch (c) {

		case '^':
			c = *str++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (isdigit(c)) {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *str++ - '0';
				while (--i && isdigit(*str));
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return (str);
}
#endif
